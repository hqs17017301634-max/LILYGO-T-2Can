/*
    PlatformIO entry point.
    Shared build settings live in platformio_profile.h.
    Logic is in the shared headers under include/.
*/

#ifdef ESP_PLATFORM
#include "platform/espidf_runtime.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#include <Arduino.h>
#endif
#ifdef DRIVER_T2CAN_DUAL
// LILYGO T-2Can: native TWAI is the primary bus (FSD/speed pipeline, unchanged);
// an MCP2515 over SPI is a second bus monitored alongside it. Drive the primary
// through the existing DRIVER_TWAI path and bolt the secondary on top. DRIVER_TWAI
// must be defined before app.h so its CAN-task globals are declared.
#define DRIVER_TWAI
#ifndef T2CAN_SECONDARY_BUS
#define T2CAN_SECONDARY_BUS CAN_BUS_PARTY
#endif
#endif

#include "app.h"

#ifdef DRIVER_T2CAN_DUAL
#include "drivers/esp32_mcp2515_driver.h"
#endif

#ifdef DRIVER_MCP2515
#include <SPI.h>
#include "drivers/mcp2515_driver.h"
#elif defined(DRIVER_ESP32_EXT_MCP2515)
#ifndef ESP_PLATFORM
#include <SPI.h>
#endif
#include "drivers/esp32_mcp2515_driver.h"
#elif defined(DRIVER_SAME51)
#include "drivers/same51_driver.h"
#elif defined(DRIVER_TWAI)
#include "drivers/twai_driver.h"
#ifndef ESP_PLATFORM
#include <Preferences.h>
#endif
#ifndef TWAI_TX_PIN
#define TWAI_TX_PIN GPIO_NUM_5
#endif
#ifndef TWAI_RX_PIN
#define TWAI_RX_PIN GPIO_NUM_4
#endif
#else
#error "Define DRIVER_MCP2515, DRIVER_ESP32_EXT_MCP2515, DRIVER_SAME51, or DRIVER_TWAI in build_flags"
#endif

#if defined(ESP_PLATFORM) && defined(DRIVER_TWAI)
static bool appTwaiGpioReserved(gpio_num_t pin)
{
    int p = static_cast<int>(pin);
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    if (p >= 26 && p <= 32)
        return true; // embedded flash/PSRAM bus on ESP32-S3 modules
    if (p == 45 || p == 46)
        return true; // strapping/input-only pins
#elif defined(CONFIG_IDF_TARGET_ESP32)
#if !defined(DASH_ALLOW_CAN_GPIO_6_11) || !DASH_ALLOW_CAN_GPIO_6_11
    if (p >= 6 && p <= 11)
        return true; // SPI flash pins on common ESP32 modules
#endif
#endif
    return false;
}

static bool appTwaiGpioValid(gpio_num_t pin, bool tx)
{
    int p = static_cast<int>(pin);
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    constexpr int kMaxGpio = 48;
#else
    constexpr int kMaxGpio = 39;
#endif
    if (p < 0 || p > kMaxGpio || appTwaiGpioReserved(pin))
        return false;
#if defined(CONFIG_IDF_TARGET_ESP32)
    if (tx && p >= 34 && p <= 39)
        return false; // input-only pins cannot drive TWAI TX
#else
    (void)tx;
#endif
    return true;
}
#endif

#ifdef DRIVER_T2CAN_DUAL
static std::unique_ptr<ESP32_MCP2515Driver> appDriverSecondary;
static volatile uint32_t t2canSecondaryRxCount = 0;
static volatile uint32_t t2canSecondaryTxCount = 0;
static volatile uint32_t t2canSecondaryTxErrCount = 0;
static volatile uint8_t t2canSecondaryEflg = 0;

// Single counted TX path for the secondary (bus B / X197 9/10) so the dashboard
// can report CAN2 send + error totals. Every bus-B transmit goes through here.
static bool t2canTxSecondaryCounted(const CanFrame &f)
{
    bool ok = appDriverSecondary->send(f);
    t2canSecondaryTxCount = t2canSecondaryTxCount + 1;
    if (!ok)
        t2canSecondaryTxErrCount = t2canSecondaryTxErrCount + 1;
    return ok;
}

static void t2canSetupSecondary()
{
#ifdef MCP2515_RST_PIN
    gpio_set_direction((gpio_num_t)MCP2515_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)MCP2515_RST_PIN, 1);
    delay(20);
    gpio_set_level((gpio_num_t)MCP2515_RST_PIN, 0);
    delay(20);
    gpio_set_level((gpio_num_t)MCP2515_RST_PIN, 1);
    delay(20);
#endif
    appDriverSecondary = std::make_unique<ESP32_MCP2515Driver>(PIN_CAN_CS);
    if (!appDriverSecondary->init())
    {
        Serial.println("CAN B (MCP2515) init failed");
        return;
    }
    appDriverSecondary->mcp().setReceiveAllMode();
    Serial.println("CAN B (MCP2515) ready @ 500k");
}

// Transmit on the secondary bus. No automatic logic targets bus B yet
// (service-mode / lighting features land later); this is the TX entry point.
bool t2canSendSecondary(const CanFrame &frame)
{
    if (!appDriverSecondary)
        return false;
    CanFrame f = frame;
    f.bus = T2CAN_SECONDARY_BUS;
    return t2canTxSecondaryCounted(f);
}

// ── bus2 discovered-ID table (X197 9/10) — for serial log + dashboard /bus2_ids ──
struct T2canBus2Id
{
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint32_t count;
};
static constexpr uint16_t kT2canBus2MaxIds = 160;
static T2canBus2Id g_bus2Ids[kT2canBus2MaxIds];
static volatile uint16_t g_bus2IdCount = 0;

// Latest rolling counter seen on the real 0x249 SCCMLeftStalk stream, so an
// injected test frame can ride just after it with the next counter value.
static volatile uint8_t g_stalkLastCounter = 0;

// Forward decl — definition is alongside the stalk/burst injectors below.
static void t2canBurstCheckTrigger(uint8_t newStatus);

static void t2canRecordBus2(const CanFrame &f)
{
    if (f.id & 0x80000000UL)
        return; // standard 11-bit IDs only (Tesla lighting/stalk are standard)
    uint16_t sid = (uint16_t)(f.id & 0x7FF);
    if (sid == 0x249 && f.dlc >= 2)
    {
        g_stalkLastCounter = f.data[1] & 0x0F; // track real stalk counter
        t2canBurstCheckTrigger((uint8_t)((f.data[1] >> 4) & 0x07));
    }
    uint16_t n = g_bus2IdCount;
    for (uint16_t i = 0; i < n; i++)
    {
        if (g_bus2Ids[i].id == sid)
        {
            g_bus2Ids[i].dlc = f.dlc;
            memcpy(g_bus2Ids[i].data, f.data, 8);
            g_bus2Ids[i].count = g_bus2Ids[i].count + 1;
            return;
        }
    }
    if (n < kT2canBus2MaxIds)
    {
        g_bus2Ids[n].id = sid;
        g_bus2Ids[n].dlc = f.dlc;
        memcpy(g_bus2Ids[n].data, f.data, 8);
        g_bus2Ids[n].count = 1;
        g_bus2IdCount = n + 1; // publish count last so readers never see uninit slots
        Serial.printf("bus2 new id 0x%03X dlc=%u\n", sid, f.dlc);
    }
}

// Accessors used by the dashboard /bus2_ids handler.
uint16_t t2canBus2IdCount(void) { return g_bus2IdCount; }
bool t2canBus2IdAt(uint16_t i, uint16_t *id, uint8_t *dlc, uint8_t data[8], uint32_t *count)
{
    if (i >= g_bus2IdCount)
        return false;
    *id = g_bus2Ids[i].id;
    *dlc = g_bus2Ids[i].dlc;
    memcpy(data, g_bus2Ids[i].data, 8);
    *count = g_bus2Ids[i].count;
    return true;
}

// CAN2 (bus B / MCP2515) traffic counters for the dashboard status bar.
uint32_t t2canBus2RxCount(void) { return t2canSecondaryRxCount; }
uint32_t t2canBus2TxCount(void) { return t2canSecondaryTxCount; }
uint32_t t2canBus2TxErrCount(void) { return t2canSecondaryTxErrCount; }
uint8_t t2canBus2Eflg(void) { return t2canSecondaryEflg; }

static void t2canDrainSecondary()
{
    if (!appDriverSecondary)
        return;
    CanFrame f;
    for (uint8_t budget = 16; budget; budget--)
    {
        if (!appDriverSecondary->read(f))
            break;
        f.bus = T2CAN_SECONDARY_BUS;
        t2canSecondaryRxCount = t2canSecondaryRxCount + 1;
        t2canRecordBus2(f);
#ifdef ESP32_DASHBOARD
        // Also feed bus2 frames into the CSV recorder (with bus column) so a
        // timestamped sequential capture of X197 9/10 can be downloaded for
        // offline checksum/counter reverse-engineering (0x249/0x3E9/0x3F5).
        dashRecordCanFrame(f, 'R');
#endif
    }
}

// ── Service mode (BODY bus / bus B): VCSEC_serviceDiagnosticRequest 0x339 ──
// Spec 2.4.1: send 4 frames at 10ms spacing on the BODY bus. The signal lives at
// start bit 47 (Intel) = byte5 bit7: 1 = enter service mode, 0 = exit.
//   activate   -> 00 00 00 00 00 80 00 00
//   deactivate -> 00 00 00 00 00 00 00 00
// RAM-only flag, OFF on boot; each dashboard /service_mode toggle fires one burst.
static volatile bool g_t2canServiceMode = false;
static volatile uint8_t g_svcBurstRemaining = 0; // frames left in the 4-frame burst
static volatile uint8_t g_svcBurstValue = 0;     // byte5 value for this burst

void t2canSetServiceMode(bool on)
{
    g_t2canServiceMode = on;
    g_svcBurstValue = on ? 0x80 : 0x00; // byte5 bit7 = serviceDiagnosticRequest
    g_svcBurstRemaining = 4;            // spec: 4 frames @ 10ms
}
bool t2canGetServiceMode(void) { return g_t2canServiceMode; }

static void t2canServiceModeTick()
{
    if (g_svcBurstRemaining == 0 || !appDriverSecondary)
        return;
    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last < 10) // 10ms interval per spec
        return;
    last = now;
    CanFrame f = {};
    f.id = 0x339;
    f.dlc = 8;
    f.data[5] = g_svcBurstValue;
    f.bus = T2CAN_SECONDARY_BUS;
    t2canTxSecondaryCounted(f);
    g_svcBurstRemaining = g_svcBurstRemaining - 1;
}

// ── Stalk injection test (0x249 SCCMLeftStalk on bus B / X197 9/10) ──
// CRC reverse-engineered from a 2023.11 Model Y HW3 capture (846/846 verified):
//   b0 = BASE[counter] XOR OFFSET[status]   (valid while b2=b3=0)
// status: 1=PULL (flash/超车闪), 2=PUSH (high-beam toggle/远光). See
// docs/stalk_0x249_crc_solved_zh.md. Tests whether plain injection (no MITM)
// can drive the lights despite the genuine SCCM also transmitting 0x249.
static const uint8_t kStalkBase[16] = {
    0x9B, 0xE8, 0x2A, 0xD3, 0xD3, 0x83, 0x4C, 0x5E,
    0x3F, 0x5E, 0xE2, 0x28, 0x3A, 0x13, 0xAF, 0xCE};
static inline uint8_t t2canStalkCrc(uint8_t counter, uint8_t status)
{
    static const uint8_t off[8] = {0x00, 0x76, 0xEC, 0x00, 0xF7, 0x00, 0x00, 0x00};
    return kStalkBase[counter & 0x0F] ^ off[status & 0x07];
}

static volatile uint8_t g_stalkInjStatus = 0;  // 0=off, 1=PULL, 2=PUSH
static volatile uint32_t g_stalkInjUntil = 0;  // millis() deadline

void t2canStalkTest(uint8_t status, uint16_t durationMs)
{
    g_stalkInjStatus = status;
    g_stalkInjUntil = millis() + durationMs;
}

static void t2canStalkInjectTick()
{
    if (!appDriverSecondary || g_stalkInjStatus == 0)
        return;
    uint32_t now = millis();
    if ((int32_t)(now - g_stalkInjUntil) >= 0)
    {
        g_stalkInjStatus = 0;
        return;
    }
    static uint32_t last = 0;
    if (now - last < 50) // match real 0x249 cadence (~20Hz)
        return;
    last = now;
    uint8_t st = g_stalkInjStatus;
    uint8_t cnt = (g_stalkLastCounter + 1) & 0x0F; // ride after newest real frame
    CanFrame f = {};
    f.id = 0x249;
    f.dlc = 4;
    f.data[0] = t2canStalkCrc(cnt, st);
    f.data[1] = (uint8_t)((st << 4) | cnt);
    f.data[2] = 0;
    f.data[3] = 0;
    f.bus = T2CAN_SECONDARY_BUS;
    t2canTxSecondaryCounted(f);
#ifdef ESP32_DASHBOARD
    dashRecordCanFrame(f, 'T');
#endif
}

// ── Flash burst (双拨触发的爆闪) ──
// Trigger: while enabled, two real-stalk PULL events (idle→1) within 2 seconds
// fire a burst of N flashes. Each flash = PULL for onMs then idle for offMs.
// Implemented by driving the existing stalk injector (g_stalkInjStatus=1) in a
// timed on/off pattern so the burst rides the same 50ms cadence used elsewhere.
static volatile bool g_burstEnabled = false;
static volatile uint8_t g_burstCount = 3;    // flash count (1-20)
static volatile uint16_t g_burstOnMs = 180;  // PULL duration per flash (80-1000)
static volatile uint16_t g_burstOffMs = 180; // idle duration per flash (80-1000)
static volatile uint32_t g_lastPullStartMs = 0;
static volatile uint8_t g_prevStalkStatus = 0;
static volatile uint16_t g_burstPhasesLeft = 0; // 2× count (alternating on/off)
static volatile bool g_burstOnPhase = false;
static volatile uint32_t g_burstPhaseEnd = 0;

void t2canSetBurstEnabled(bool on)
{
    g_burstEnabled = on;
    if (!on)
    {
        // Disabling cancels any in-flight burst and clears the trigger window.
        g_burstPhasesLeft = 0;
        if (g_stalkInjStatus == 1)
            g_stalkInjStatus = 0;
        g_lastPullStartMs = 0;
    }
}
bool t2canGetBurstEnabled(void) { return g_burstEnabled; }

void t2canSetBurstParams(uint8_t cnt, uint16_t onMs, uint16_t offMs)
{
    if (cnt < 1)
        cnt = 1;
    if (cnt > 20)
        cnt = 20;
    if (onMs < 80)
        onMs = 80;
    if (onMs > 1000)
        onMs = 1000;
    if (offMs < 80)
        offMs = 80;
    if (offMs > 1000)
        offMs = 1000;
    g_burstCount = cnt;
    g_burstOnMs = onMs;
    g_burstOffMs = offMs;
}
uint8_t t2canGetBurstCount(void) { return g_burstCount; }
uint16_t t2canGetBurstOnMs(void) { return g_burstOnMs; }
uint16_t t2canGetBurstOffMs(void) { return g_burstOffMs; }

// Called from t2canRecordBus2 on every received 0x249 frame's status field.
// idle→PULL transition is the "tap"; two taps within 2s fires the burst.
static void t2canBurstCheckTrigger(uint8_t newStatus)
{
    bool pullStart = (g_prevStalkStatus != 1) && (newStatus == 1);
    g_prevStalkStatus = newStatus;
    if (!g_burstEnabled || !pullStart || g_burstPhasesLeft != 0)
        return;
    uint32_t now = millis();
    if (g_lastPullStartMs != 0 && (now - g_lastPullStartMs) <= 2000)
    {
        // Double-tap detected → arm a burst of g_burstCount flashes.
        g_burstPhasesLeft = (uint16_t)g_burstCount * 2;
        g_burstOnPhase = true;
        g_burstPhaseEnd = now + g_burstOnMs;
        g_stalkInjStatus = 1; // PULL
        g_stalkInjUntil = now + g_burstOnMs + 30;
        g_lastPullStartMs = 0; // consume the pair
    }
    else
    {
        g_lastPullStartMs = now;
    }
}

// Drives the burst phase machine. Each flash = ON phase (inject PULL) + OFF
// phase (release). When all phases done, clear the injector.
static void t2canBurstTick()
{
    if (g_burstPhasesLeft == 0)
        return;
    uint32_t now = millis();
    if ((int32_t)(now - g_burstPhaseEnd) < 0)
        return; // current phase still running
    g_burstPhasesLeft = g_burstPhasesLeft - 1;
    if (g_burstPhasesLeft == 0)
    {
        g_stalkInjStatus = 0;
        return;
    }
    g_burstOnPhase = !g_burstOnPhase;
    if (g_burstOnPhase)
    {
        g_stalkInjStatus = 1;
        g_stalkInjUntil = now + g_burstOnMs + 30;
        g_burstPhaseEnd = now + g_burstOnMs;
    }
    else
    {
        g_stalkInjStatus = 0;
        g_burstPhaseEnd = now + g_burstOffMs;
    }
}

// Periodically cache the MCP2515 error flags from inside the CAN task (the same
// task that owns the SPI bus, so no contention) so the dashboard can show CAN2
// bus health (bus-off / TX-error-passive => bad wiring/termination/arbitration).
static void t2canBus2HealthTick()
{
    if (!appDriverSecondary)
        return;
    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last < 500)
        return;
    last = now;
    t2canSecondaryEflg = appDriverSecondary->mcp().getErrorFlags();
}

// ── Bus2 acquisition filter ──
// Default = accept-all (needed for the sniffer / recorder discovery work).
// When enabled, narrow the MCP2515 hardware acceptance filters to only the IDs
// the lighting logic consumes (0x249 stalk, 0x3F5 lighting) so the chip drops
// everything else at the hardware level — eliminates RX overflow on a busy bus
// and cuts SPI/CPU load during normal operation. Applied from the CAN task to
// avoid SPI contention.
static const uint32_t kBus2FilterIds[] = {0x249, 0x3F5};
static volatile bool g_bus2FilterMode = false;    // requested (false=all)
static volatile bool g_bus2FilterApplied = false; // currently applied

void t2canSetBus2Filter(bool on) { g_bus2FilterMode = on; }
bool t2canGetBus2Filter(void) { return g_bus2FilterMode; }

static void t2canBus2FilterTick()
{
    if (!appDriverSecondary || g_bus2FilterMode == g_bus2FilterApplied)
        return;
    if (g_bus2FilterMode)
    {
        appDriverSecondary->setFilters(kBus2FilterIds,
                                       sizeof(kBus2FilterIds) / sizeof(kBus2FilterIds[0]));
        appDriverSecondary->mcp().setUseFiltersMode();
        Serial.println("bus2 filter: only 0x249/0x3F5");
    }
    else
    {
        appDriverSecondary->mcp().setReceiveAllMode();
        Serial.println("bus2 filter: accept-all");
    }
    g_bus2FilterApplied = g_bus2FilterMode;
}
#endif

static void app_main_setup()
{
#ifdef DRIVER_MCP2515
    appSetup<MCP2515Driver>(std::make_unique<MCP2515Driver>(PIN_CAN_CS), "MCP25625 ready @ 500k");
#ifdef ESP32_DASHBOARD
    mcpDashboardSetup(appHandler.get(), appDriver.get());
#endif
#elif defined(DRIVER_ESP32_EXT_MCP2515)
#ifndef ESP_PLATFORM
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, PIN_CAN_CS);
    SPI.setFrequency(8000000);
#endif
    auto drv = std::make_unique<ESP32_MCP2515Driver>(PIN_CAN_CS);
    MCP2515 *mcpPtr = &drv->mcp();
    appSetup<ESP32_MCP2515Driver>(std::move(drv), "ESP32 + MCP2515 ready @ 500k");
#ifdef ESP32_DASHBOARD
    mcpDashboardSetup(appHandler.get(), appDriver.get(), mcpPtr);
#endif
#elif defined(DRIVER_SAME51)
    appSetup<SAME51Driver>(std::make_unique<SAME51Driver>(), "SAME51 CAN ready @ 500k");
#elif defined(DRIVER_TWAI)
    // Load TWAI pins from NVS (survives OTA); fall back to compile-time defaults
    gpio_num_t twaiTx = TWAI_TX_PIN;
    gpio_num_t twaiRx = TWAI_RX_PIN;
    {
        Preferences canPrefs;
        if (canPrefs.begin("can", false))
        {
            int8_t tx = canPrefs.getChar("tx", -1);
            int8_t rx = canPrefs.getChar("rx", -1);
            canPrefs.end();
            if (appTwaiGpioValid((gpio_num_t)tx, true) && appTwaiGpioValid((gpio_num_t)rx, false) && tx != rx)
            {
                twaiTx = (gpio_num_t)tx;
                twaiRx = (gpio_num_t)rx;
            }
        }
    }
    appSetup<TWAIDriver>(std::make_unique<TWAIDriver>(twaiTx, twaiRx), "ESP32 TWAI ready @ 500k");
#ifdef ESP32_DASHBOARD
    mcpDashboardSetup(appHandler.get(), appDriver.get());
#endif
#endif
#ifdef DRIVER_T2CAN_DUAL
    t2canSetupSecondary();
#endif
}

static bool app_main_loop()
{
#ifdef DRIVER_MCP2515
    bool processed = appLoop<MCP2515Driver>();
#ifdef ESP32_DASHBOARD
    mcpDashboardLoop();
#endif
    return processed;
#elif defined(DRIVER_ESP32_EXT_MCP2515)
    bool processed = appLoop<ESP32_MCP2515Driver>();
#ifdef ESP32_DASHBOARD
    mcpDashboardLoop();
#endif
    return processed;
#elif defined(DRIVER_SAME51)
    return appLoop<SAME51Driver>();
#elif defined(DRIVER_TWAI)
    bool processed = appLoop<TWAIDriver>();
#ifdef ESP32_DASHBOARD
    mcpDashboardLoop();
#endif
    return processed;
#endif
}

#if defined(ESP_PLATFORM) && defined(DRIVER_TWAI)
#ifndef APP_CAN_TASK_STACK
#define APP_CAN_TASK_STACK 6144
#endif
#ifndef APP_CAN_TASK_PRIORITY
#define APP_CAN_TASK_PRIORITY 18
#endif
#ifndef APP_CAN_TASK_CORE
#define APP_CAN_TASK_CORE 0
#endif

static void app_can_task(void *)
{
    appCanTaskDedicated = true;
    for (;;)
    {
        bool processed = appLoop<TWAIDriver>();
#ifdef DRIVER_T2CAN_DUAL
        t2canDrainSecondary();
        t2canServiceModeTick();
        t2canBurstTick();          // drives the multi-flash burst phase machine
        t2canStalkInjectTick();
        t2canBus2HealthTick();
        t2canBus2FilterTick();     // apply pending accept-all/filtered switch
#endif
        appCanTaskLoops = appCanTaskLoops + 1;
        if (!processed)
        {
            appCanTaskIdleLoops = appCanTaskIdleLoops + 1;
            vTaskDelay(1);
        }
    }
}

static bool app_start_can_task()
{
    TaskHandle_t task = nullptr;
#if CONFIG_FREERTOS_UNICORE
    BaseType_t ok = xTaskCreate(app_can_task, "can_rt", APP_CAN_TASK_STACK, nullptr,
                                APP_CAN_TASK_PRIORITY, &task);
#else
    BaseType_t ok = xTaskCreatePinnedToCore(app_can_task, "can_rt", APP_CAN_TASK_STACK, nullptr,
                                            APP_CAN_TASK_PRIORITY, &task, APP_CAN_TASK_CORE);
#endif
    appCanTaskDedicated = ok == pdPASS;
    return ok == pdPASS;
}
#endif

#ifdef ESP_PLATFORM
extern "C" void app_main(void)
{
    esp_err_t nvsErr = nvs_flash_init();
    if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvsErr = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvsErr);

    app_main_setup();
#if defined(DRIVER_TWAI)
    bool canTaskStarted = app_start_can_task();
    while (true)
    {
        if (!canTaskStarted)
        {
            if (!app_main_loop())
                vTaskDelay(1);
            continue;
        }
#ifdef ESP32_DASHBOARD
        mcpDashboardLoop();
#endif
        vTaskDelay(1);
    }
#else
    while (true)
    {
        if (!app_main_loop())
            vTaskDelay(1);
    }
#endif
}
#else
void setup()
{
    app_main_setup();
}

void loop()
{
    app_main_loop();
}
#endif
