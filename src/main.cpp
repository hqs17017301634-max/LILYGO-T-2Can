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
    return appDriverSecondary->send(f);
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

static void t2canRecordBus2(const CanFrame &f)
{
    if (f.id & 0x80000000UL)
        return; // standard 11-bit IDs only (Tesla lighting/stalk are standard)
    uint16_t sid = (uint16_t)(f.id & 0x7FF);
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
    }
}

// ── Service mode (X197 pin 9/10 / bus B): periodic 0x339 injection ──
// Owner-provided frame {00,00,00,00,00,E0,00,00}. RAM-only flag, OFF on boot;
// injects only while explicitly enabled via the dashboard /service_mode toggle.
static volatile bool g_t2canServiceMode = false;

void t2canSetServiceMode(bool on) { g_t2canServiceMode = on; }
bool t2canGetServiceMode(void) { return g_t2canServiceMode; }

static void t2canServiceModeTick()
{
    if (!g_t2canServiceMode || !appDriverSecondary)
        return;
    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last < 10)
        return;
    last = now;
    CanFrame f = {};
    f.id = 0x339;
    f.dlc = 8;
    f.data[5] = 0xE0;
    f.bus = T2CAN_SECONDARY_BUS;
    appDriverSecondary->send(f);
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
