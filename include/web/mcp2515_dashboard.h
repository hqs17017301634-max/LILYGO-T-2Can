#pragma once

#if defined(ESP32_DASHBOARD) && !defined(NATIVE_BUILD)

#ifdef ESP_PLATFORM
#include "platform/espidf_runtime.h"
#else
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Update.h>
#endif
#include <esp_task_wdt.h>
#ifdef ESP_PLATFORM
#include <driver/temperature_sensor.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_image_format.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>
#include <esp_pm.h>
#include <esp_spiffs.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <esp_private/esp_clk.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif
#ifndef ESP_PLATFORM
#include <Preferences.h>
#include <SPIFFS.h>
#endif
#include "handlers.h"
#include "can_helpers.h"
#include <ArduinoJson.h>
#if defined(DRIVER_ESP32_EXT_MCP2515)
#include "drivers/esp32_mcp2515_driver.h"
#endif
#include "web/mcp2515_dashboard_ui.h"

#ifndef DASH_SSID
#error "Define -DDASH_SSID in build_flags (e.g. -DDASH_SSID=\\\"ADUnlock-1234\\\")"
#endif
#ifndef DASH_PASS
#error "Define -DDASH_PASS in build_flags (min 8 chars)"
#endif
#ifndef DASH_OTA_PASS
#error "Define -DDASH_OTA_PASS in build_flags"
#endif
#ifndef DASH_OTA_USER
#error "Define -DDASH_OTA_USER in build_flags"
#endif

static_assert(sizeof(DASH_SSID) > 1 && sizeof(DASH_SSID) <= 33, "DASH_SSID must be 1-32 bytes");
static_assert(sizeof(DASH_PASS) >= 9 && sizeof(DASH_PASS) <= 65, "DASH_PASS must be 8-64 bytes");

#ifndef DASH_DEFAULT_HW
#define DASH_DEFAULT_HW 1
#endif

#if defined(DASH_INJECTION_ON_BOOT)
static constexpr bool kDashInjectionDefaultEnabled = true;
#else
static constexpr bool kDashInjectionDefaultEnabled = false;
#endif

#if defined(DRIVER_TWAI)
#ifndef TWAI_TX_PIN
#define TWAI_TX_PIN GPIO_NUM_5
#endif
#ifndef TWAI_RX_PIN
#define TWAI_RX_PIN GPIO_NUM_4
#endif
#endif

#if DASH_DEFAULT_HW < 0 || DASH_DEFAULT_HW > 2
#error "DASH_DEFAULT_HW must be 0 (LEGACY), 1 (HW3), or 2 (HW4)"
#endif

#define PREFS_NS "ADunlock"
static constexpr uint8_t kDashUnsetU8 = 0xFF;

static Preferences prefs;

static CarManagerBase *dashHandler = nullptr;
static CanDriver *dashDriver = nullptr;
#if defined(DRIVER_ESP32_EXT_MCP2515)
static MCP2515 *dashMcp = nullptr;
#endif

static unsigned long rxCount = 0;
static unsigned long txCount = 0;
static unsigned long txErrCount = 0;
static unsigned long lastFrameMs = 0;
static unsigned long startMs = 0;
static bool canOnline = false;
static uint8_t followDist = 0;

static unsigned long fpsFrames = 0;
static unsigned long fpsLastMs = 0;
static float fps = 0.0f;

static unsigned long muxRx[4] = {};
static unsigned long muxTx[4] = {};
static unsigned long muxErr[4] = {};

#if defined(DRIVER_ESP32_EXT_MCP2515)
static uint8_t mcpEflg = 0;
#else
static const uint8_t mcpEflg = 0;
#endif

static uint8_t hwMode = DASH_DEFAULT_HW;
static bool canActive = kDashInjectionDefaultEnabled;
static bool forceActivate = false;
// AP Injection Gate — when false (default), 1021 mux0 bit46 注入与车辆状态解耦，
// 复刻 2.5.2 真车固件默认行为（kDashApGateDefaultEnabled=false）。
// 当 true 时回到 3.0 早期行为：必须 Parked||APActive||Summoning 才允许注入。
static bool apInjectionGate = false;
static bool apAutoRestore = false;
// 上一次 dashPostProcessFrame 实际发送成功的时间戳，便于 /status 区分"在持续发"与
// "发了几次就停"，与 framesSent 单调累计计数互补。跨 CAN 任务 / dashboard 任务读写。
static volatile uint32_t lastInjectMs = 0;
static bool dashSpeedProfileAuto = true;
static uint8_t dashManualSpeedProfile = 1;

// HW3 slew limiter constants/state moved to include/dash_hw3_speed.h so
// HW3Handler can call dashApplyHw3OffsetSlew directly.

// HW3 custom-speed config + helpers live in their own header so handlers.h
// (parsed before this file) can read fusedSpeedLimitRaw and call the
// encoders. See include/dash_hw3_speed.h.

#ifdef RGB_BRIGHTNESS
static constexpr uint8_t kDashLedBrightnessDefault = RGB_BRIGHTNESS;
#else
static constexpr uint8_t kDashLedBrightnessDefault = 32;
#endif
static constexpr uint8_t dashLedBrightness = kDashLedBrightnessDefault;

// WiFi AP (hotspot) — overridable at runtime
static char apSSID[33] = "";
static char apPass[65] = "";
static bool apHidden = false; // when true, SSID is not broadcast (hidden AP)
static constexpr size_t kDashMaxSsidLen = 32;
static constexpr size_t kDashMinApPassLen = 8;
static constexpr size_t kDashMaxPassLen = 64;
static constexpr int kDashApChannel = 1;
static constexpr int kDashApMaxConn = 4;
static uint8_t apRuntimeChannel = kDashApChannel;
static unsigned long apLastChannelSyncMs = 0;
static uint8_t apLastChannelSyncTarget = 0;
static bool apLastChannelSyncOk = false;

// WiFi STA (client) mode for internet access
static char staSSID[33] = "";
static char staPass[65] = "";
static bool staConnected = false;
static bool staConnectAttemptActive = false;
static bool staStaticIP = false;

// Multi-SSID storage
static constexpr uint8_t kDashMaxWifiNetworks = 4;
struct DashWifiNetwork
{
    char ssid[33];
    char pass[65];
    bool useStatic;
    char ip[16];
    char gw[16];
    char mask[16];
    char dns[16];
};
static DashWifiNetwork wifiNetworks[kDashMaxWifiNetworks] = {};
static uint8_t wifiNetworkCount = 0;
static int8_t wifiActiveSlot = -1;    // slot currently selected for STA attempt
static int8_t wifiNextRotateSlot = 0; // next slot to try when rotating
static bool updateBetaChannel = false;
static bool autoUpdateEnabled = false;
static bool autoUpdateDone = false;            // one-shot per boot
static unsigned long autoUpdateEligibleAt = 0; // millis() at which auto-check may fire
static unsigned long staConnectStartedAt = 0;
static unsigned long staRetryAt = 0;
static uint8_t staConsecutiveFailures = 0; // diagnostics only; retry interval is fixed
static constexpr unsigned long kDashStaBootDelayMs = 1000;
static constexpr unsigned long kDashStaSavedPollMs = 5000;
static constexpr unsigned long kDashStaConnectTimeoutMs = 10000;
// kDashStaRetryMs kept for backward compat with older references.
static constexpr unsigned long kDashStaRetryMs = kDashStaSavedPollMs;
static IPAddress staIP(0, 0, 0, 0);
static IPAddress staGW(0, 0, 0, 0);
static IPAddress staMask(255, 255, 255, 0);
static IPAddress staDNS(0, 0, 0, 0);

// Multi-SSID NVS helpers (key form: w0s, w0p, w0t, w0i, w0g, w0m, w0d)
static String dashWifiKey(uint8_t slot, const char *sub)
{
    String k = "w";
    k += slot;
    k += sub;
    return k;
}
static void dashClearWifiNetwork(DashWifiNetwork &n)
{
    n.ssid[0] = 0;
    n.pass[0] = 0;
    n.useStatic = false;
    n.ip[0] = 0;
    n.gw[0] = 0;
    n.mask[0] = 0;
    n.dns[0] = 0;
}
static void dashRotateAndConnect();
static void dashSwapHandler(uint8_t mode);
static void dashApplyFilters();
static void dashApplyRuntimeState();
static void dashClearLegacyOptionPrefs();
static void dashLog(const String &s);

// CAN recorder
#ifndef REC_CAP
#define REC_CAP 8000
#endif
static constexpr unsigned long kRecMaxDurationMs = 60000UL;
struct RecFrame
{
    unsigned long ts;
    char dir;
    uint32_t id;
    uint8_t dlc;
    uint8_t bus;
    uint8_t data[8];
};
static RecFrame *recBuf = nullptr;
static bool recBufInPsram = false;
static volatile bool recActive = false;
static volatile int recCount = 0;
static bool recSaved = false;
static unsigned long recStartMs = 0;

// Optional capture filter: when recFilterCount > 0, only frames whose ID is in
// recFilterIds are recorded. Empty (0) = record everything (default behaviour).
// Lets you capture just the lighting/stalk IDs (0x249/0x3E9/0x3F5) over a long
// window without the busy primary bus flooding the buffer.
static constexpr int kRecFilterMax = 16;
static uint32_t recFilterIds[kRecFilterMax];
static int recFilterCount = 0;
// Optional exclude filter (blacklist): when recExcludeCount > 0, frames whose ID
// is listed are NOT recorded. Lets a broad capture drop the high-rate noise (e.g.
// 0x118 ≈ 80% of frames) so the buffer holds a much longer useful window. Applied
// after the include filter; a frame must pass both to be recorded.
static uint32_t recExcludeIds[kRecFilterMax];
static int recExcludeCount = 0;

// CAN sniffer ring buffer
#define SNIFFER_CAP 30
struct SniffFrame
{
    unsigned long ts;
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
};
static SniffFrame sniffBuf[SNIFFER_CAP];
static int sniffHead = 0;
static int sniffCount = 0;

enum DashWriteProbeState : uint8_t
{
    kDashWriteProbeIdle = 0,
    kDashWriteProbePending = 1,
    kDashWriteProbeMatch = 2,
    kDashWriteProbeDifferent = 3,
    kDashWriteProbeFailed = 4,
};

struct DashWriteProbe
{
    bool active = false;
    bool hasRx = false;
    uint8_t state = kDashWriteProbeIdle;
    uint32_t id = 0;
    int8_t mux = -1;
    uint8_t txDlc = 0;
    uint8_t rxDlc = 0;
    uint8_t txData[8] = {};
    uint8_t rxData[8] = {};
    unsigned long txMs = 0;
    unsigned long rxMs = 0;
};
static DashWriteProbe dashWriteProbe;

struct DashApRestoreState
{
    bool gearSeen = false;
    uint8_t gearRaw = 0xFF;
    bool brakeSeen = false;
    uint8_t brakePedalRaw = 0xFF;
    bool chassisSeen = false;
    bool brakeTorqueActive = false;
    uint8_t anyVdcActive = 0xFF;
    bool tcActive = false;
    uint8_t vdcControlActive = 0xFF;
    bool steerSeen = false;
    uint8_t steerValidity = 0xFF;
    int16_t steerAngleX10 = 0;
    int16_t steerSpeedX10 = 0;
    unsigned long steerMs = 0;
    bool dasSettingsSeen = false;
    uint8_t dasSettingsData[8] = {};
    unsigned long dasSettingsMs = 0;
    uint8_t dasSettingsCounter = 0xFF;
    bool dasAccSeen = false;
    uint8_t dasAccState = 0xFF;
    unsigned long dasAccDropMs = 0;
    unsigned long lastDropHandledMs = 0;
    unsigned long lastTxMs = 0;
};
static DashApRestoreState apRestoreState;
static constexpr unsigned long kDashApRestoreTxCooldownMs = 1000;

static int8_t dashFrameMux(const CanFrame &frame)
{
    if ((frame.id == 1006 || frame.id == 1021) && frame.dlc > 0)
        return static_cast<int8_t>(readMuxID(frame));
    return -1;
}

static uint32_t dashReadBitsLE(const CanFrame &frame, uint8_t startBit, uint8_t bitCount)
{
    uint32_t value = 0;
    for (uint8_t i = 0; i < bitCount; i++)
    {
        uint8_t bit = startBit + i;
        if (bit >= frame.dlc * 8)
            break;
        if (frame.data[bit / 8] & (1U << (bit % 8)))
            value |= 1UL << i;
    }
    return value;
}

static bool dashReadBit(const CanFrame &frame, uint8_t bit)
{
    return dashReadBitsLE(frame, bit, 1) != 0;
}

static uint8_t dashCounterChecksumByte(const CanFrame &frame, uint8_t checksumByteIndex = 7)
{
    if (checksumByteIndex >= frame.dlc)
        return 0;
    uint16_t sum = static_cast<uint16_t>(frame.id & 0xFF) +
                   static_cast<uint16_t>((frame.id >> 8) & 0xFF);
    for (uint8_t i = 0; i < frame.dlc; i++)
    {
        if (i == checksumByteIndex)
            sum += frame.data[i] & 0x0F;
        else
            sum += frame.data[i];
    }
    uint8_t checksum = static_cast<uint8_t>((0x10 - (sum & 0x0F)) & 0x0F);
    return static_cast<uint8_t>((checksum << 4) | (frame.data[checksumByteIndex] & 0x0F));
}

static void dashResetWriteProbe()
{
    dashWriteProbe = {};
    dashWriteProbe.mux = -1;
    dashWriteProbe.state = kDashWriteProbeIdle;
}

static bool dashEnsureRecBuffer()
{
    if (recBuf)
        return true;
#if defined(CONFIG_SPIRAM) && CONFIG_SPIRAM
    recBuf = static_cast<RecFrame *>(heap_caps_calloc(REC_CAP, sizeof(RecFrame), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (recBuf)
    {
        recBufInPsram = true;
        Serial.println("[REC] Buffer allocated in PSRAM");
        return true;
    }
#endif
    recBuf = static_cast<RecFrame *>(heap_caps_calloc(REC_CAP, sizeof(RecFrame), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    recBufInPsram = false;
    if (!recBuf)
    {
        Serial.println("[REC] Buffer allocation failed");
        return false;
    }
    Serial.println("[REC] Buffer allocated in internal RAM");
    return true;
}

static void dashReleaseRecBuffer()
{
    if (!recBuf || recActive)
        return;
    heap_caps_free(recBuf);
    recBuf = nullptr;
    recBufInPsram = false;
}

static bool dashSaveRecordingToSpiffs(int n)
{
    if (!recBuf)
    {
        dashLog("[REC] Save failed: buffer unavailable");
        return false;
    }
    if (n < 0)
        n = 0;
    if (n > REC_CAP)
        n = REC_CAP;

    File f = SPIFFS.open("/rec.csv", "w");
    if (!f)
    {
        dashLog("[REC] SPIFFS write failed");
        return false;
    }

    // bus: 2 = secondary MCP2515 (X197 9/10), 1 = primary TWAI (X197 13/14)
    // Batch lines into an 8 KB buffer and flush in big chunks — one f.write per
    // ~200 frames instead of per frame, cutting SPIFFS call overhead ~200x.
    static char chunk[8192]; // not reentrant; only one save runs at a time
    size_t pos = 0;
    {
        const char *hdr = "ts_ms,dir,bus,id,dlc,b0,b1,b2,b3,b4,b5,b6,b7\n";
        size_t hlen = strlen(hdr);
        memcpy(chunk, hdr, hlen);
        pos = hlen;
    }
    char line[96];
    for (int i = 0; i < n; i++)
    {
        int len = snprintf(line, sizeof(line),
                           "%lu,%c,%u,%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                           recBuf[i].ts,
                           recBuf[i].dir ? recBuf[i].dir : 'R',
                           static_cast<unsigned>(recBuf[i].bus == CAN_BUS_PARTY ? 2 : 1),
                           static_cast<unsigned long>(recBuf[i].id),
                           static_cast<unsigned>(recBuf[i].dlc),
                           static_cast<unsigned>(recBuf[i].data[0]),
                           static_cast<unsigned>(recBuf[i].data[1]),
                           static_cast<unsigned>(recBuf[i].data[2]),
                           static_cast<unsigned>(recBuf[i].data[3]),
                           static_cast<unsigned>(recBuf[i].data[4]),
                           static_cast<unsigned>(recBuf[i].data[5]),
                           static_cast<unsigned>(recBuf[i].data[6]),
                           static_cast<unsigned>(recBuf[i].data[7]));
        if (len <= 0)
            continue;
        if (len > static_cast<int>(sizeof(line) - 1))
            len = sizeof(line) - 1;
        if (pos + static_cast<size_t>(len) > sizeof(chunk))
        {
            f.write(reinterpret_cast<const uint8_t *>(chunk), pos);
            pos = 0;
        }
        memcpy(chunk + pos, line, static_cast<size_t>(len));
        pos += static_cast<size_t>(len);
    }
    if (pos > 0)
        f.write(reinterpret_cast<const uint8_t *>(chunk), pos);
    f.close();
    recSaved = true;
    dashLog("[REC] Saved " + String(n) + " frames to SPIFFS");
    return true;
}

static bool dashStopRecordingAndSave(const char *reason = nullptr)
{
    if (!recActive && recSaved)
        return true;
    recActive = false;
    int n = recCount;
    bool ok = dashSaveRecordingToSpiffs(n);
    if (reason && *reason)
        dashLog(String("[REC] Stopped: ") + reason);
    return ok;
}

static void dashRecordCanFrame(const CanFrame &f, char dir)
{
    if (!recActive || !recBuf)
        return;
    uint32_t fid = f.id & 0x1FFFFFFFUL;
    if (recFilterCount > 0)
    {
        bool match = false;
        for (int k = 0; k < recFilterCount; k++)
        {
            if (recFilterIds[k] == fid)
            {
                match = true;
                break;
            }
        }
        if (!match)
            return;
    }
    if (recExcludeCount > 0)
    {
        for (int k = 0; k < recExcludeCount; k++)
        {
            if (recExcludeIds[k] == fid)
                return; // blacklisted ID — skip
        }
    }
    int idx = recCount;
    if (idx >= REC_CAP)
        return;
    uint8_t dlc = (f.dlc <= 8) ? f.dlc : 8;
    recBuf[idx].ts = millis();
    recBuf[idx].dir = dir;
    recBuf[idx].id = f.id;
    recBuf[idx].dlc = dlc;
    recBuf[idx].bus = f.bus;
    memset(recBuf[idx].data, 0, sizeof(recBuf[idx].data));
    memcpy(recBuf[idx].data, f.data, dlc);
    recCount = idx + 1;
    if (recCount >= REC_CAP)
        dashStopRecordingAndSave("frame limit");
}

static void dashRecordApRestoreFrame(const CanFrame &frame, unsigned long now)
{
    if (frame.id == 280 && frame.dlc >= 3)
    {
        apRestoreState.gearSeen = true;
        apRestoreState.gearRaw = readDIGear(frame);
        apRestoreState.brakeSeen = true;
        apRestoreState.brakePedalRaw = static_cast<uint8_t>(dashReadBitsLE(frame, 19, 2));
        return;
    }
    if (frame.id == 0x148 && frame.dlc >= 8)
    {
        apRestoreState.chassisSeen = true;
        apRestoreState.brakeTorqueActive = dashReadBit(frame, 15);
        apRestoreState.anyVdcActive = static_cast<uint8_t>(dashReadBitsLE(frame, 34, 2));
        apRestoreState.tcActive = dashReadBit(frame, 42);
        apRestoreState.vdcControlActive = static_cast<uint8_t>(dashReadBitsLE(frame, 60, 3));
        return;
    }
    if (frame.id == 0x129 && frame.dlc >= 6)
    {
        uint16_t angleRaw = static_cast<uint16_t>(dashReadBitsLE(frame, 16, 14));
        uint16_t speedRaw = static_cast<uint16_t>(dashReadBitsLE(frame, 32, 14));
        apRestoreState.steerSeen = true;
        apRestoreState.steerValidity = static_cast<uint8_t>(dashReadBitsLE(frame, 30, 2));
        apRestoreState.steerAngleX10 = angleRaw == 0x3FFF ? 0 : static_cast<int16_t>(angleRaw) - 8192;
        apRestoreState.steerSpeedX10 = static_cast<int16_t>(static_cast<int32_t>(speedRaw) * 5 - 40960);
        apRestoreState.steerMs = now;
        return;
    }
    if (frame.id == 0x293 && frame.dlc >= 8)
    {
        apRestoreState.dasSettingsSeen = true;
        apRestoreState.dasSettingsMs = now;
        memcpy(apRestoreState.dasSettingsData, frame.data, 8);
        apRestoreState.dasSettingsCounter = frame.data[7] & 0x0F;
        return;
    }
    if (frame.id == 0x389 && frame.dlc >= 4)
    {
        uint8_t accState = static_cast<uint8_t>((frame.data[3] >> 2) & 0x1F);
        if (apRestoreState.dasAccSeen && apRestoreState.dasAccState > 0 && accState == 0)
            apRestoreState.dasAccDropMs = now;
        apRestoreState.dasAccSeen = true;
        apRestoreState.dasAccState = accState;
    }
}

static bool dashWriteProbeMatches(const CanFrame &frame)
{
    if (!dashWriteProbe.active || dashWriteProbe.id != frame.id)
        return false;

    int8_t mux = dashFrameMux(frame);
    if (dashWriteProbe.mux < 0)
        return mux < 0;
    return mux == dashWriteProbe.mux;
}

static const char *decodeCanId(uint32_t id)
{
    switch (id)
    {
    case 0x045:
        return "STW_ACTN_RQ";
    case 0x129:
        return "Steering angle";
    case 0x175:
        return "Speed";
    case 0x186:
        return "Gear/Drive state";
    case 0x118:
        return "DI_systemStatus";
    case 0x233:
        return "UI_stalklessControl";
    case 0x257:
        return "State of charge";
    case 0x293:
        return "DAS control";
    case 0x321:
        return "Autopilot state";
    case 0x329:
        return "UI_autopilot";
    case 0x399:
        return "DAS_status";
    case 0x3E8:
        return "UI_driverAssistControl";
    case 0x3FD:
        return "UI_autopilotControl";
    case 0x678:
        return "GTW_gearControl";
    default:
        return "";
    }
}

static void sniffPush(const CanFrame &f)
{
    uint8_t dlc = (f.dlc <= 8) ? f.dlc : 8;
    sniffBuf[sniffHead] = {millis(), f.id, dlc, {}};
    memcpy(sniffBuf[sniffHead].data, f.data, dlc);
    sniffHead = (sniffHead + 1) % SNIFFER_CAP;
    if (sniffCount < SNIFFER_CAP)
        sniffCount++;
}

#define LOG_CAP 80
struct LogEntry
{
    String msg;
    unsigned long seq;
};
static LogEntry logBuf[LOG_CAP];
static int logHead = 0;
static int logCount = 0;
static unsigned long logSeq = 0;
// Cursor tracking how much of logRing we have copied into logBuf so far.
// logRing is filled by HW3/HW4 handlers when enablePrint is on (per-frame
// "AD: 1, Profile: 2, Offset: ..." diagnostics). We drain it here on demand
// so the WebUI /log endpoint surfaces those diagnostics in real time.
static uint32_t logRingDrainCursor = 0;

static void dashLog(const String &s)
{
    logBuf[logHead] = {String(millis() / 1000) + "s " + s, ++logSeq};
    logHead = (logHead + 1) % LOG_CAP;
    if (logCount < LOG_CAP)
        logCount++;
    Serial.println(s);
}

// Pull all new entries from the per-frame handler logRing (in handlers.h)
// into logBuf so /log returns them. Cheap: bounded by ring capacity (32).
static void dashDrainLogRing()
{
    uint32_t h = logRing.currentHead();
    if (h <= logRingDrainCursor)
    {
        logRingDrainCursor = h; // handle wrap / restart
        return;
    }
    LogRingBuffer::Entry tmp[LogRingBuffer::kCapacity];
    int n = logRing.readSince(logRingDrainCursor, tmp, LogRingBuffer::kCapacity);
    for (int i = 0; i < n; i++)
    {
        // Use the timestamp captured at push time, not now, so messages keep
        // their actual ordering. dashLog format prefixes seconds-since-boot.
        logBuf[logHead] = {String(tmp[i].timestamp_ms / 1000) + "s " + String(tmp[i].msg), ++logSeq};
        logHead = (logHead + 1) % LOG_CAP;
        if (logCount < LOG_CAP)
            logCount++;
    }
    logRingDrainCursor = h;
}

// Public hooks
static void mcpDashOnFrame(const CanFrame &f)
{
    unsigned long now = millis();
    rxCount++;
    lastFrameMs = now;
    canOnline = true;
    fpsFrames++;
    sniffPush(f);
    if (f.id == 1021 && f.dlc > 0)
    {
        uint8_t m = f.data[0] & 0x07;
        if (m < 4)
            muxRx[m]++;
    }
    if (f.id == 1016 && f.dlc > 5)
        followDist = (f.data[5] & 0xE0) >> 5;
    dashRecordApRestoreFrame(f, now);
    dashRecordCanFrame(f, 'R');
    if (dashWriteProbe.active && dashWriteProbe.state != kDashWriteProbeFailed && dashWriteProbeMatches(f))
    {
        dashWriteProbe.hasRx = true;
        dashWriteProbe.rxMs = now;
        dashWriteProbe.rxDlc = (f.dlc <= 8) ? f.dlc : 8;
        memset(dashWriteProbe.rxData, 0, sizeof(dashWriteProbe.rxData));
        memcpy(dashWriteProbe.rxData, f.data, dashWriteProbe.rxDlc);
        bool same = dashWriteProbe.txDlc == dashWriteProbe.rxDlc &&
                    memcmp(dashWriteProbe.txData, dashWriteProbe.rxData, dashWriteProbe.txDlc) == 0;
        dashWriteProbe.state = same ? kDashWriteProbeMatch : kDashWriteProbeDifferent;
    }
}

static void mcpDashOnTxFrame(const CanFrame &frame, bool ok)
{
    txCount++;
    int8_t mux = dashFrameMux(frame);
    if (!ok)
    {
        txErrCount++;
        if (mux >= 0 && mux < 4)
            muxErr[mux]++;
    }
    else if (mux >= 0 && mux < 4)
    {
        muxTx[mux]++;
    }
    if (ok)
        dashRecordCanFrame(frame, 'T');

    dashWriteProbe.active = true;
    dashWriteProbe.hasRx = false;
    dashWriteProbe.state = ok ? kDashWriteProbePending : kDashWriteProbeFailed;
    dashWriteProbe.id = frame.id;
    dashWriteProbe.mux = mux;
    dashWriteProbe.txMs = millis();
    dashWriteProbe.rxMs = 0;
    dashWriteProbe.txDlc = (frame.dlc <= 8) ? frame.dlc : 8;
    dashWriteProbe.rxDlc = 0;
    memset(dashWriteProbe.txData, 0, sizeof(dashWriteProbe.txData));
    memset(dashWriteProbe.rxData, 0, sizeof(dashWriteProbe.rxData));
    memcpy(dashWriteProbe.txData, frame.data, dashWriteProbe.txDlc);
}

// JSON escape for log strings
static String jsonEscape(const String &s)
{
    String out;
    out.reserve(s.length() + 8);
    for (unsigned int i = 0; i < s.length(); i++)
    {
        char c = s.charAt(i);
        if (c == '"')
            out += "\\\"";
        else if (c == '\\')
            out += "\\\\";
        else if (c == '\n')
            out += "\\n";
        else if (c == '\r')
            out += "\\r";
        else if (c < 0x20)
            out += ' ';
        else
            out += c;
    }
    return out;
}

static bool dashCheckADEnabled()
{
    return canActive;
}

static bool dashApInjectionAllowed()
{
    // 2.5.2 风格：apInjectionGate=false 时短路放行；=true 时回到 3.0 强制门控。
    return !apInjectionGate || (dashHandler && dashHandler->injectionGateOpen());
}

static bool dashInjectionActive()
{
    return canActive && dashApInjectionAllowed();
}

static bool dashApRestoreBraking()
{
    return (apRestoreState.brakeSeen && apRestoreState.brakePedalRaw == 1) ||
           (apRestoreState.chassisSeen && apRestoreState.brakeTorqueActive);
}

static bool dashApRestoreStabilityBlocked()
{
    return apRestoreState.chassisSeen &&
           (apRestoreState.anyVdcActive == 1 || apRestoreState.vdcControlActive > 0 ||
            apRestoreState.tcActive);
}

static void dashTryApAutoRestore(const CanFrame &trigger, CanDriver &driver)
{
    if (trigger.id != 0x389 || !apAutoRestore)
        return;

    unsigned long now = millis();
    if (!apRestoreState.dasAccDropMs ||
        apRestoreState.lastDropHandledMs == apRestoreState.dasAccDropMs ||
        now - apRestoreState.dasAccDropMs > 250)
        return;

    apRestoreState.lastDropHandledMs = apRestoreState.dasAccDropMs;
    if (!apRestoreState.dasSettingsSeen || now - apRestoreState.dasSettingsMs > 5000)
        return;
    if (!apRestoreState.gearSeen || apRestoreState.gearRaw != 4)
        return;
    if (dashApRestoreBraking() || dashApRestoreStabilityBlocked())
        return;
    if (apRestoreState.lastTxMs && now - apRestoreState.lastTxMs < kDashApRestoreTxCooldownMs)
        return;

    CanFrame modified{};
    modified.id = 0x293;
    modified.bus = CAN_BUS_DEFAULT;
    modified.dlc = 8;
    memcpy(modified.data, apRestoreState.dasSettingsData, 8);
    CanFrame original = modified;
    setBit(modified, 38, true);
    setBit(modified, 24, true);
    uint8_t counter = static_cast<uint8_t>(((apRestoreState.dasSettingsCounter == 0xFF ? 0 : apRestoreState.dasSettingsCounter) + 1) & 0x0F);
    modified.data[7] = static_cast<uint8_t>((modified.data[7] & 0xF0) | counter);
    modified.data[7] = dashCounterChecksumByte(modified);
    if (!framePayloadChanged(original, modified))
        return;

    bool ok = driver.send(modified);
    apRestoreState.lastTxMs = now;
    if (ok)
        lastInjectMs = now;
    dashRecordCanFrame(modified, ok ? 'T' : 'E');
    dashLog("[AP] Auto-restore " + String(ok ? "TX OK" : "TX FAIL"));
}

static void dashPostProcessFrame(const CanFrame &original, CanDriver &driver)
{
#if defined(DASH_FSD_252_COMPAT) && DASH_FSD_252_COMPAT
    dashTryApAutoRestore(original, driver);

    if ((hwMode != 0 && hwMode != 1) || !dashInjectionActive())
        return;
    const uint32_t activationId = hwMode == 0 ? 1006 : 1021;
    if (original.id != activationId || original.dlc < 8 || readMuxID(original) != 0)
        return;

    CanFrame modified = original;
    if (dashHandler && !(bool)dashHandler->speedProfileAuto)
        setSpeedProfileV12V13(modified, (int)dashHandler->speedProfile);
    setBit(modified, 46, true);
    if (!framePayloadChanged(original, modified))
        return;

    if (dashHandler)
        dashHandler->framesSent++;
    bool ok = driver.send(modified);
    if (ok)
        lastInjectMs = millis();
    if (dashHandler && dashHandler->onSend)
        dashHandler->onSend(0, ok);
#else
    (void)original;
    (void)driver;
#endif
}

static bool dashCheckNagDisabled()
{
    return false;
}

static bool dashStaSsidLooksCorrupt(const String &ssid)
{
    return ssid.indexOf("\"ssid\"") >= 0 || ssid.indexOf("{\"") >= 0 ||
           ssid.indexOf("\",\"") >= 0;
}

// dashClampHw3SlewRate / dashLoadHw3SlewRate now in dash_hw3_speed.h.

static uint8_t dashClampSpeedProfileForHw(uint8_t hw, int profile)
{
    int maxProfile = hw == 2 ? 4 : 2;
    if (profile < 0)
        return 0;
    if (profile > maxProfile)
        return static_cast<uint8_t>(maxProfile);
    return static_cast<uint8_t>(profile);
}

static void dashApplySpeedProfileState()
{
    if (!dashHandler)
        return;
    dashHandler->speedProfileAuto = dashSpeedProfileAuto;
    if (!dashSpeedProfileAuto)
        dashHandler->speedProfile = dashClampSpeedProfileForHw(hwMode, dashManualSpeedProfile);
}

// HW3 mux-2 codec + slew limiter live in include/dash_hw3_speed.h so they
// can be shared between HW3Handler (in handlers.h, parsed first) and the
// dashboard HW3 send path below.

static void dashApplyRuntimeState()
{
    forceActivateRuntime = canActive && forceActivate;
    emergencyVehicleDetectionRuntime = false;
    isaSpeedChimeSuppressRuntime = false;
    enhancedAutopilotRuntime = false;
    nagKillerRuntime = false;

    if (dashHandler)
    {
        dashHandler->checkAD = dashCheckADEnabled;
        dashHandler->checkNag = dashCheckNagDisabled;
        dashApplySpeedProfileState();
        if (!canActive)
        {
            dashHandler->ADEnabled = false;
            dashHandler->APActive = false;
        }
    }

#if defined(DASH_RGB_STATUS_LED)
    appRefreshStatusLed();
#endif
}

// Store config
static void dashSavePrefs()
{
    prefs.begin(PREFS_NS, false);
    prefs.putUChar("hw", hwMode);
    prefs.putUChar("hw_def", DASH_DEFAULT_HW);
    prefs.putBool("can", canActive);
    prefs.putBool("force_act", forceActivate);
    prefs.putBool("ap_gate", apInjectionGate);
    prefs.putBool("ap_rst", apAutoRestore);
    prefs.putBool("sp_auto", dashSpeedProfileAuto);
    prefs.putUChar("sp_sel", dashManualSpeedProfile);
    prefs.putBool("eprn", dashHandler ? (bool)dashHandler->enablePrint : true);
    prefs.putBool("h3_slw", hw3OffsetSlew);
    prefs.putUChar("h3_srt", hw3SlewRate);
    // HW3 custom speed-limit boost
    prefs.putBool("h3_cust", hw3CustomSpeed);
    prefs.putBool("h3_hse", hw3HighSpeedEnable);
    prefs.putUChar("h3_enc", hw3WireEncoding);
    char k[8];
    for (uint8_t i = 0; i < kHw3CustomTargetCount; i++)
    {
        snprintf(k, sizeof(k), "h3_ct%u", (unsigned)i);
        prefs.putUChar(k, hw3CustomTarget[i]);
    }
    for (uint8_t i = 0; i < kHw3HighSpeedBucketCount; i++)
    {
        snprintf(k, sizeof(k), "h3_ht%u", (unsigned)i);
        prefs.putUChar(k, hw3HighSpeedTarget[i]);
    }
    // Legacy MPP custom speed-limit override
    prefs.putBool("lg_mpp_en", legacyMppOverride);
    prefs.putBool("lg_mppc_en", legacyMppCustomEnable);
    prefs.putBool("lg_mpph_en", legacyMppHighSpeedEnable);
    for (uint8_t i = 0; i < kLegacyMppCustomTargetCount; i++)
    {
        snprintf(k, sizeof(k), "lg_ct%u", (unsigned)i);
        prefs.putUChar(k, legacyMppCustomTarget[i]);
    }
    for (uint8_t i = 0; i < kLegacyMppHighSpeedBucketCount; i++)
    {
        snprintf(k, sizeof(k), "lg_ht%u", (unsigned)i);
        prefs.putUChar(k, legacyMppHighSpeedTarget[i]);
    }
    prefs.end();
}

static void dashSetCanActive(bool active, const char *reason = nullptr)
{
    bool changed = (canActive != active) || (forceActivate != active);
    canActive = active;
    forceActivate = active;
    dashApplyRuntimeState();
    dashSavePrefs();
    if (changed)
    {
        String msg = String("[CFG] FSD master switch ") + (active ? "ON" : "OFF");
        if (reason && *reason)
            msg += String(" via ") + reason;
        dashLog(msg);
    }
}

[[maybe_unused]] static void dashToggleCanActive(const char *reason = nullptr)
{
    dashSetCanActive(!canActive, reason);
}

static bool dashApPasswordLengthValid(size_t len)
{
    return len >= kDashMinApPassLen && len <= kDashMaxPassLen;
}

static bool dashApConfigValid(const char *ssid, const char *pass)
{
    size_t ssidLen = strlen(ssid);
    size_t passLen = strlen(pass);
    return ssidLen > 0 && ssidLen <= kDashMaxSsidLen && dashApPasswordLengthValid(passLen);
}

static void dashUseDefaultApConfig()
{
    strlcpy(apSSID, DASH_SSID, sizeof(apSSID));
    strlcpy(apPass, DASH_PASS, sizeof(apPass));
    apHidden = false;
    apRuntimeChannel = kDashApChannel;
}

static uint8_t dashConfiguredApChannel()
{
    return kDashApChannel;
}

static bool dashStaConfigLengthValid(const String &ssid, const String &pass)
{
    return ssid.length() <= kDashMaxSsidLen && pass.length() <= kDashMaxPassLen;
}

static void dashClearLegacyOptionPrefs()
{
    static const char *const keys[] = {
        "fAD",
        "f_AD",
        "f_nag",
        "f_sum",
        "f_isa",
        "f_evd",
        "f_h4o",
        "sp",
        "sp_lock",
    };

    bool removed = false;
    for (const char *key : keys)
    {
        if (!prefs.isKey(key))
            continue;
        prefs.remove(key);
        removed = true;
    }

    if (removed)
        dashLog("[BOOT] Cleared legacy dashboard prefs from NVS");
}

static void dashLoadPrefs()
{
    prefs.begin(PREFS_NS, false);
    dashClearLegacyOptionPrefs();
    bool hasStoredHw = prefs.isKey("hw");
    uint8_t storedHw = prefs.getUChar("hw", DASH_DEFAULT_HW);
    uint8_t storedDefaultHw = prefs.getUChar("hw_def", kDashUnsetU8);
    bool migratedHw = false;

    hwMode = storedHw <= 2 ? storedHw : DASH_DEFAULT_HW;
    if (!hasStoredHw || storedHw > 2)
        migratedHw = true;

    // If the stored selection only mirrors the old firmware default, follow the
    // new build default after reflashing instead of staying pinned to stale NVS.
    if (storedDefaultHw <= 2 && storedDefaultHw != DASH_DEFAULT_HW && hwMode == storedDefaultHw)
    {
        hwMode = DASH_DEFAULT_HW;
        migratedHw = true;
    }

    if (migratedHw)
        prefs.putUChar("hw", hwMode);
    if (storedDefaultHw != DASH_DEFAULT_HW)
        prefs.putUChar("hw_def", DASH_DEFAULT_HW);
    canActive = prefs.getBool("can", kDashInjectionDefaultEnabled);
    forceActivate = canActive;
    if (prefs.getBool("force_act", canActive) != forceActivate)
        prefs.putBool("force_act", forceActivate);
    // 默认 false：复刻 2.5.2 真车固件行为（apInjectionGate=false 注入无条件放行）。
    apInjectionGate = prefs.getBool("ap_gate", false);
    apAutoRestore = prefs.getBool("ap_rst", false);
    dashSpeedProfileAuto = prefs.getBool("sp_auto", true);
    dashManualSpeedProfile = dashClampSpeedProfileForHw(hwMode, prefs.getUChar("sp_sel", 1));
    hw3OffsetSlew = prefs.getBool("h3_slw", false);
    hw3SlewRate = dashLoadHw3SlewRate(prefs.getUChar("h3_srt", kHw3SlewRateDefault));
    // HW3 custom speed-limit boost
    hw3CustomSpeed = prefs.getBool("h3_cust", false);
    hw3HighSpeedEnable = prefs.getBool("h3_hse", false);
    {
        uint8_t enc = prefs.getUChar("h3_enc", kHw3WireEncDefault);
        hw3WireEncoding = (enc == kHw3WireEncKph5) ? kHw3WireEncKph5 : kHw3WireEncPct4;
    }
    {
        char k[8];
        static const uint8_t defCt[kHw3CustomTargetCount] = {45, 60, 75, 90, 105};
        static const uint8_t defHs[kHw3HighSpeedBucketCount] = {90, 110, 130};
        for (uint8_t i = 0; i < kHw3CustomTargetCount; i++)
        {
            snprintf(k, sizeof(k), "h3_ct%u", (unsigned)i);
            hw3CustomTarget[i] = dashClampHw3CustomTargetForBucket(i, prefs.getUChar(k, defCt[i]));
        }
        for (uint8_t i = 0; i < kHw3HighSpeedBucketCount; i++)
        {
            snprintf(k, sizeof(k), "h3_ht%u", (unsigned)i);
            hw3HighSpeedTarget[i] = dashClampHw3HighSpeedTargetForBucket(i,
                prefs.getUChar(k, defHs[i]));
        }
    }
    // Legacy MPP custom speed-limit override
    legacyMppOverride = prefs.getBool("lg_mpp_en", false);
    legacyMppCustomEnable = prefs.getBool("lg_mppc_en", false);
    legacyMppHighSpeedEnable = prefs.getBool("lg_mpph_en", false);
    {
        char k[8];
        static const uint8_t defLgCt[kLegacyMppCustomTargetCount] = {45, 60, 75, 90, 105};
        static const uint8_t defLgHt[kLegacyMppHighSpeedBucketCount] = {90, 110, 130};
        for (uint8_t i = 0; i < kLegacyMppCustomTargetCount; i++)
        {
            snprintf(k, sizeof(k), "lg_ct%u", (unsigned)i);
            legacyMppCustomTarget[i] = dashClampLegacyMppCustomTargetForBucket(i, prefs.getUChar(k, defLgCt[i]));
        }
        for (uint8_t i = 0; i < kLegacyMppHighSpeedBucketCount; i++)
        {
            snprintf(k, sizeof(k), "lg_ht%u", (unsigned)i);
            legacyMppHighSpeedTarget[i] = dashClampLegacyMppHighSpeedTargetForBucket(i, prefs.getUChar(k, defLgHt[i]));
        }
    }
    bool ep = prefs.getBool("eprn", true);

    dashApplyRuntimeState();
    if (dashHandler)
        dashHandler->enablePrint = ep;
    // Load WiFi AP overrides (hotspot name/password)
    String apSsidPref = prefs.isKey("ap_ssid") ? prefs.getString("ap_ssid", "") : "";
    String apPassPref = prefs.isKey("ap_pass") ? prefs.getString("ap_pass", "") : "";
    bool hasApOverride = apSsidPref.length() > 0 || apPassPref.length() > 0 || prefs.isKey("ap_hidden");
    bool invalidApOverride = apSsidPref.length() > kDashMaxSsidLen ||
                             (apPassPref.length() > 0 && !dashApPasswordLengthValid(apPassPref.length()));
    if (apSsidPref.length() > 0)
        strlcpy(apSSID, apSsidPref.c_str(), sizeof(apSSID));
    else
        strlcpy(apSSID, DASH_SSID, sizeof(apSSID));
    if (apPassPref.length() > 0)
        strlcpy(apPass, apPassPref.c_str(), sizeof(apPass));
    else
        strlcpy(apPass, DASH_PASS, sizeof(apPass));
    apHidden = prefs.getBool("ap_hidden", false);
    if (invalidApOverride || !dashApConfigValid(apSSID, apPass))
    {
        if (hasApOverride)
        {
            prefs.remove("ap_ssid");
            prefs.remove("ap_pass");
            prefs.remove("ap_hidden");
            dashLog("[WIFI] Invalid saved AP config ignored");
        }
        dashUseDefaultApConfig();
    }
    apRuntimeChannel = dashConfiguredApChannel();

    // Load WiFi STA networks (multi-SSID slot array)
    wifiNetworkCount = 0;
    for (uint8_t i = 0; i < kDashMaxWifiNetworks; i++)
        dashClearWifiNetwork(wifiNetworks[i]);

    uint8_t storedCount = prefs.getUChar("wn_cnt", 0);
    if (storedCount > kDashMaxWifiNetworks)
        storedCount = kDashMaxWifiNetworks;

    for (uint8_t i = 0; i < storedCount; i++)
    {
        DashWifiNetwork &n = wifiNetworks[wifiNetworkCount];
        String s = prefs.getString(dashWifiKey(i, "s").c_str(), "");
        String p = prefs.getString(dashWifiKey(i, "p").c_str(), "");
        if (!dashStaConfigLengthValid(s, p) || dashStaSsidLooksCorrupt(s) || s.length() == 0)
            continue;
        strlcpy(n.ssid, s.c_str(), sizeof(n.ssid));
        strlcpy(n.pass, p.c_str(), sizeof(n.pass));
        n.useStatic = prefs.getBool(dashWifiKey(i, "t").c_str(), false);
        if (n.useStatic)
        {
            String ip = prefs.getString(dashWifiKey(i, "i").c_str(), "0.0.0.0");
            String gw = prefs.getString(dashWifiKey(i, "g").c_str(), "0.0.0.0");
            String mk = prefs.getString(dashWifiKey(i, "m").c_str(), "255.255.255.0");
            String dn = prefs.getString(dashWifiKey(i, "d").c_str(), "0.0.0.0");
            strlcpy(n.ip, ip.c_str(), sizeof(n.ip));
            strlcpy(n.gw, gw.c_str(), sizeof(n.gw));
            strlcpy(n.mask, mk.c_str(), sizeof(n.mask));
            strlcpy(n.dns, dn.c_str(), sizeof(n.dns));
        }
        wifiNetworkCount++;
    }

    // One-shot migration from legacy single-SSID keys
    if (wifiNetworkCount == 0 && prefs.isKey("wifi_ssid"))
    {
        String s = prefs.getString("wifi_ssid", "");
        String p = prefs.getString("wifi_pass", "");
        if (dashStaConfigLengthValid(s, p) && !dashStaSsidLooksCorrupt(s) && s.length() > 0)
        {
            DashWifiNetwork &n = wifiNetworks[0];
            strlcpy(n.ssid, s.c_str(), sizeof(n.ssid));
            strlcpy(n.pass, p.c_str(), sizeof(n.pass));
            n.useStatic = prefs.getBool("wifi_static", false);
            if (n.useStatic)
            {
                strlcpy(n.ip, prefs.getString("wifi_ip", "0.0.0.0").c_str(), sizeof(n.ip));
                strlcpy(n.gw, prefs.getString("wifi_gw", "0.0.0.0").c_str(), sizeof(n.gw));
                strlcpy(n.mask, prefs.getString("wifi_mask", "255.255.255.0").c_str(), sizeof(n.mask));
                strlcpy(n.dns, prefs.getString("wifi_dns", "0.0.0.0").c_str(), sizeof(n.dns));
            }
            wifiNetworkCount = 1;
            prefs.putUChar("wn_cnt", 1);
            prefs.putString(dashWifiKey(0, "s").c_str(), s);
            prefs.putString(dashWifiKey(0, "p").c_str(), p);
            prefs.putBool(dashWifiKey(0, "t").c_str(), n.useStatic);
            if (n.useStatic)
            {
                prefs.putString(dashWifiKey(0, "i").c_str(), String(n.ip));
                prefs.putString(dashWifiKey(0, "g").c_str(), String(n.gw));
                prefs.putString(dashWifiKey(0, "m").c_str(), String(n.mask));
                prefs.putString(dashWifiKey(0, "d").c_str(), String(n.dns));
            }
            dashLog("[WIFI] Migrated legacy STA config to slot 0");
        }
        prefs.remove("wifi_ssid");
        prefs.remove("wifi_pass");
        prefs.remove("wifi_static");
        prefs.remove("wifi_ip");
        prefs.remove("wifi_gw");
        prefs.remove("wifi_mask");
        prefs.remove("wifi_dns");
    }

    // Seed staSSID/staPass with the preferred slot (last network that
    // successfully connected) if it's still valid, otherwise fall back to
    // slot 0. Connecting to the last-known-good network first is much faster
    // than always rotating from slot 0 across reboots.
    if (wifiNetworkCount > 0)
    {
        uint8_t preferred = prefs.getUChar("wn_pref", 0);
        if (preferred >= wifiNetworkCount)
            preferred = 0;
        const DashWifiNetwork &n = wifiNetworks[preferred];
        strlcpy(staSSID, n.ssid, sizeof(staSSID));
        strlcpy(staPass, n.pass, sizeof(staPass));
        staStaticIP = n.useStatic;
        if (n.useStatic)
        {
            staIP.fromString(n.ip);
            staGW.fromString(n.gw);
            staMask.fromString(n.mask);
            staDNS.fromString(n.dns);
        }
        wifiActiveSlot = static_cast<int8_t>(preferred);
        wifiNextRotateSlot = preferred;
    }
    else
    {
        staSSID[0] = 0;
        staPass[0] = 0;
        staStaticIP = false;
        wifiActiveSlot = -1;
        wifiNextRotateSlot = 0;
    }

    updateBetaChannel = prefs.getBool("update_beta", false);
    autoUpdateEnabled = prefs.getBool("auto_upd", false);
    prefs.end();

    if (migratedHw)
        dashLog("[BOOT] HW default synced to " + String(hwMode == 0 ? "LEGACY" : hwMode == 1 ? "HW3"
                                                                                             : "HW4"));
    dashLog("[BOOT] Prefs loaded HW=" + String(hwMode));
    dashLog("[BOOT] canActive=" + String(canActive ? "YES" : "NO"));
}

// MCP2515-only: fine-grained filter register reload on HW mode switch.
// Other builds use dashDriver->setFilters() in dashSwapHandler instead.
static void dashApplyFilters()
{
#if defined(DRIVER_ESP32_EXT_MCP2515)
    if (!dashMcp)
        return;
    dashMcp->setConfigMode();
    if (hwMode == 0)
    {
        dashMcp->setFilterMask(MCP2515::MASK0, false, 0x7FF);
        dashMcp->setFilter(MCP2515::RXF0, false, 69);
        dashMcp->setFilter(MCP2515::RXF1, false, 280);
        dashMcp->setFilterMask(MCP2515::MASK1, false, 0x7FF);
        dashMcp->setFilter(MCP2515::RXF2, false, 390);
        dashMcp->setFilter(MCP2515::RXF3, false, 760);
        dashMcp->setFilter(MCP2515::RXF4, false, 921);
        dashMcp->setFilter(MCP2515::RXF5, false, 1006);
    }
    else if (hwMode == 2)
    {
        dashMcp->setFilterMask(MCP2515::MASK0, false, 0x7FF);
        dashMcp->setFilter(MCP2515::RXF0, false, 921);
        dashMcp->setFilter(MCP2515::RXF1, false, 1021);
        dashMcp->setFilterMask(MCP2515::MASK1, false, 0x7FF);
        dashMcp->setFilter(MCP2515::RXF2, false, 1016);
        dashMcp->setFilter(MCP2515::RXF3, false, 280);
        dashMcp->setFilter(MCP2515::RXF4, false, 1016);
        dashMcp->setFilter(MCP2515::RXF5, false, 921);
    }
    else
    {
        dashMcp->setFilterMask(MCP2515::MASK0, false, 0x7FF);
        dashMcp->setFilter(MCP2515::RXF0, false, 1016);
        dashMcp->setFilter(MCP2515::RXF1, false, 1021);
        dashMcp->setFilterMask(MCP2515::MASK1, false, 0x7FF);
        dashMcp->setFilter(MCP2515::RXF2, false, 1016);
        dashMcp->setFilter(MCP2515::RXF3, false, 280);
        dashMcp->setFilter(MCP2515::RXF4, false, 1016);
        dashMcp->setFilter(MCP2515::RXF5, false, 1021);
    }
    dashMcp->setNormalMode();
    dashLog("[CFG] Filters set for " + String(hwMode == 0 ? "LEGACY" : hwMode == 1 ? "HW3"
                                                                                   : "HW4"));
#endif
}

// Bus-off recovery (MCP2515 only — TWAI driver handles its own bus-off internally)
#if defined(DRIVER_ESP32_EXT_MCP2515)
static unsigned long lastEflgCheckMs = 0;
static void dashCheckBusHealth()
{
    if (!dashMcp)
        return;
    if (millis() - lastEflgCheckMs < 5000)
        return;
    lastEflgCheckMs = millis();
    uint8_t eflg = dashMcp->getErrorFlags();
    mcpEflg = eflg;
    if (eflg & 0x20)
    {
        dashLog("[ERR] MCP2515 BUS-OFF -- recovering");
        dashMcp->reset();
        delay(10);
        dashMcp->setBitrate(CAN_500KBPS, MCP_CRYSTAL_FREQ);
        dashApplyFilters();
        dashLog("[OK] MCP2515 recovered");
    }
}
#else
static void dashCheckBusHealth()
{
}
#endif
static WebServer server(80);

#include "web/dash_gateway.h"

static void handleRoot()
{
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
#ifdef ESP_PLATFORM
    server.sendRaw(200, "text/html",
                   reinterpret_cast<const char *>(DASH_HTML_GZ),
                   DASH_HTML_GZ_LEN);
#else
    server.send_P(200, "text/html", reinterpret_cast<const char *>(DASH_HTML_GZ), DASH_HTML_GZ_LEN);
#endif
}

#ifdef DRIVER_T2CAN_DUAL
// CAN2 (bus B / MCP2515, X197 pin 9/10) traffic counters — defined in main.cpp.
uint32_t t2canBus2RxCount(void);
uint32_t t2canBus2TxCount(void);
uint32_t t2canBus2TxErrCount(void);
uint8_t t2canBus2Eflg(void);
void t2canSetBurstEnabled(bool on);
bool t2canGetBurstEnabled(void);
void t2canSetBurstParams(uint8_t cnt, uint16_t onMs, uint16_t offMs);
uint8_t t2canGetBurstCount(void);
uint16_t t2canGetBurstOnMs(void);
uint16_t t2canGetBurstOffMs(void);
void t2canSetBus2Filter(bool on); // true = MCP2515 HW-filter to only 0x249/0x3F5
bool t2canGetBus2Filter(void);
#endif

static void handleStatus()
{
    if (canOnline && millis() - lastFrameMs > 10000)
    {
        canOnline = false;
        dashLog("[CAN] Bus OFFLINE (timeout)");
    }
    unsigned long now = millis();
    if (now - fpsLastMs >= 2000)
    {
        fps = fpsFrames * 1000.0f / max(1UL, now - fpsLastMs);
        fpsFrames = 0;
        fpsLastMs = now;
    }

    bool APActive = dashHandler ? (bool)dashHandler->APActive : false;
    bool ADEnabled = dashHandler ? (bool)dashHandler->ADEnabled : false;
    int sp = dashHandler ? (int)dashHandler->speedProfile : 0;
    bool spAuto = dashHandler ? (bool)dashHandler->speedProfileAuto : true;
    int soff = dashHandler ? (int)dashHandler->speedOffset : 0;
    int gtwAp = dashHandler ? (int)dashHandler->gatewayAutopilot : -1;
    bool ep = dashHandler ? (bool)dashHandler->enablePrint : true;
    bool apGateOpen = dashApInjectionAllowed();

    String j = "{\"hw\":";
    j += hwMode;
    j += ",\"sp\":";
    j += sp;
    j += ",\"spAuto\":";
    j += spAuto ? "true" : "false";
    j += ",\"soff\":";
    j += soff;
    j += ",\"gtwap\":";
    j += gtwAp;
    j += ",\"AD\":";
    j += APActive ? "true" : "false";
    j += ",\"apActive\":";
    j += APActive ? "true" : "false";
    j += ",\"adEnabled\":";
    j += ADEnabled ? "true" : "false";
    j += ",\"eprn\":";
    j += ep ? "true" : "false";
    j += ",\"force\":";
    j += forceActivate ? "true" : "false";
    j += ",\"apGate\":";
    j += (canActive && !apGateOpen) ? "true" : "false";
    j += ",\"apGateOpen\":";
    j += apGateOpen ? "true" : "false";
    j += ",\"apGateEnabled\":";
    j += apInjectionGate ? "true" : "false";
    j += ",\"apAutoRestore\":";
    j += apAutoRestore ? "true" : "false";
    j += ",\"ia\":";
    j += dashInjectionActive() ? "true" : "false";
    j += ",\"lastInjectMs\":";
    j += (uint32_t)lastInjectMs;
    j += ",\"hw3OffsetSlew\":";
    j += hw3OffsetSlew ? "true" : "false";
    j += ",\"hw3SlewRate\":";
    j += hw3SlewRate;
    j += ",\"hw3OffsetTarget\":";
    j += hw3OffsetTargetRaw;
    j += ",\"hw3OffsetLast\":";
    j += hw3OffsetLastRaw;
    j += ",\"hw3SlewCount\":";
    j += hw3OffsetSlewCount;
    // HW3 custom speed-limit boost
    j += ",\"hw3CustomSpeed\":";
    j += hw3CustomSpeed ? "true" : "false";
    j += ",\"hw3CustomTarget\":[";
    for (uint8_t i = 0; i < kHw3CustomTargetCount; i++)
    {
        if (i) j += ",";
        j += hw3CustomTarget[i];
    }
    j += "],\"hw3HighSpeedEnable\":";
    j += hw3HighSpeedEnable ? "true" : "false";
    j += ",\"hw3HighSpeedTarget\":[";
    for (uint8_t i = 0; i < kHw3HighSpeedBucketCount; i++)
    {
        if (i) j += ",";
        j += hw3HighSpeedTarget[i];
    }
    j += "],\"hw3WireEncoding\":";
    j += hw3WireEncoding;
    j += ",\"fusedSpeedLimitRaw\":";
    j += fusedSpeedLimitRaw;
    j += ",\"fusedSpeedLimitKph\":";
    j += (fusedSpeedLimitRaw == 0 || fusedSpeedLimitRaw == 31)
             ? 0
             : (uint16_t)fusedSpeedLimitRaw * 5;
    j += ",\"hw3StockOffset\":";
    j += hw3StockOffsetKph;
    // Legacy MPP custom speed-limit override
    j += ",\"legacyMppOverride\":";
    j += legacyMppOverride ? "true" : "false";
    j += ",\"legacyMppCustomEnable\":";
    j += legacyMppCustomEnable ? "true" : "false";
    j += ",\"legacyMppCustomTarget\":[";
    for (uint8_t i = 0; i < kLegacyMppCustomTargetCount; i++)
    {
        if (i) j += ",";
        j += legacyMppCustomTarget[i];
    }
    j += "],\"legacyMppHighSpeedEnable\":";
    j += legacyMppHighSpeedEnable ? "true" : "false";
    j += ",\"legacyMppHighSpeedTarget\":[";
    for (uint8_t i = 0; i < kLegacyMppHighSpeedBucketCount; i++)
    {
        if (i) j += ",";
        j += legacyMppHighSpeedTarget[i];
    }
    j += "],\"legacyMppLastRaw\":";
    j += legacyMppLastRaw;
    j += ",\"legacyMppLastSentRaw\":";
    j += legacyMppLastSentRaw;
    j += ",\"can\":";
    j += canOnline ? "true" : "false";
    j += ",\"ci\":";
    j += canActive ? "true" : "false";
    j += ",\"rx\":";
    j += rxCount;
    j += ",\"tx\":";
    j += txCount;
    j += ",\"txerr\":";
    j += txErrCount;
    j += ",\"fd\":";
    j += followDist;
    j += ",\"fps\":";
    {
        unsigned long fpsX10 = static_cast<unsigned long>(fps * 10.0f + 0.5f);
        j += String(fpsX10 / 10);
        j += ".";
        j += String(fpsX10 % 10);
    }
    j += ",\"eflg\":";
    j += mcpEflg;
    j += ",\"up\":";
    j += (millis() - startMs) / 1000;
    j += ",\"probe\":{\"active\":";
    j += dashWriteProbe.active ? "true" : "false";
    j += ",\"state\":";
    j += dashWriteProbe.state;
    j += ",\"id\":";
    j += dashWriteProbe.id;
    j += ",\"mux\":";
    j += dashWriteProbe.mux;
    j += ",\"txa\":";
    j += dashWriteProbe.active ? String(now - dashWriteProbe.txMs) : String(0);
    j += ",\"rxa\":";
    j += dashWriteProbe.hasRx ? String(now - dashWriteProbe.rxMs) : String(0);
    j += ",\"txdlc\":";
    j += dashWriteProbe.txDlc;
    j += ",\"rxdlc\":";
    j += dashWriteProbe.rxDlc;
    j += ",\"hasrx\":";
    j += dashWriteProbe.hasRx ? "true" : "false";
    j += ",\"tx\":[";
    for (uint8_t i = 0; i < dashWriteProbe.txDlc; i++)
    {
        if (i)
            j += ",";
        j += String(dashWriteProbe.txData[i]);
    }
    j += "],\"rx\":[";
    for (uint8_t i = 0; i < dashWriteProbe.rxDlc; i++)
    {
        if (i)
            j += ",";
        j += String(dashWriteProbe.rxData[i]);
    }
    j += "]},\"mux\":[";
    for (int i = 0; i < 3; i++)
    {
        if (i)
            j += ",";
        j += "{\"rx\":" + String(muxRx[i]) +
             ",\"tx\":" + String(muxTx[i]) +
             ",\"err\":" + String(muxErr[i]) + "}";
    }
    j += "]";
#ifdef DRIVER_T2CAN_DUAL
    j += ",\"can2\":{\"rx\":";
    j += t2canBus2RxCount();
    j += ",\"tx\":";
    j += t2canBus2TxCount();
    j += ",\"txerr\":";
    j += t2canBus2TxErrCount();
    j += ",\"eflg\":";
    j += t2canBus2Eflg();
    j += ",\"filter\":";
    j += t2canGetBus2Filter() ? "true" : "false";
    j += ",\"burst\":{\"en\":";
    j += t2canGetBurstEnabled() ? "true" : "false";
    j += ",\"cnt\":";
    j += t2canGetBurstCount();
    j += ",\"on\":";
    j += t2canGetBurstOnMs();
    j += ",\"off\":";
    j += t2canGetBurstOffMs();
    j += "}}";
#endif
    j += "}";
    server.send(200, "application/json", j);
}

static void handleConfig()
{
    bool hwChanged = false;
    if (server.hasArg("hw"))
    {
        uint8_t v = server.arg("hw").toInt();
        if (v <= 2 && v != hwMode)
        {
            hwMode = v;
            hwChanged = true;
            dashLog("[CFG] HW=" + String(v == 0 ? "LEGACY" : v == 1 ? "HW3"
                                                                    : "HW4"));
        }
    }
    bool requestedFsdSwitch = canActive;
    bool hasFsdSwitchArg = false;
    if (server.hasArg("can"))
    {
        requestedFsdSwitch = server.arg("can") == "1";
        hasFsdSwitchArg = true;
    }
    if (server.hasArg("force"))
    {
        requestedFsdSwitch = server.arg("force") == "1";
        hasFsdSwitchArg = true;
    }
    if (hasFsdSwitchArg && ((requestedFsdSwitch != canActive) || (requestedFsdSwitch != forceActivate)))
    {
        canActive = requestedFsdSwitch;
        forceActivate = requestedFsdSwitch;
        dashLog("[CFG] FSD master switch " + String(requestedFsdSwitch ? "ON" : "OFF"));
    }
    bool profileAutoRequested = server.hasArg("spa") && server.arg("spa") == "1";
    if (server.hasArg("sp"))
    {
        uint8_t v = dashClampSpeedProfileForHw(hwMode, server.arg("sp").toInt());
        if (!profileAutoRequested && (v != dashManualSpeedProfile || dashSpeedProfileAuto))
            dashLog("[CFG] Speed profile manual " + String(v));
        dashManualSpeedProfile = v;
        if (!profileAutoRequested)
            dashSpeedProfileAuto = false;
    }
    if (server.hasArg("spa"))
    {
        bool v = server.arg("spa") == "1";
        if (v != dashSpeedProfileAuto)
            dashLog("[CFG] Speed profile " + String(v ? "AUTO" : "MANUAL"));
        dashSpeedProfileAuto = v;
    }
    if (server.hasArg("apRestore"))
    {
        bool v = server.arg("apRestore") == "1";
        if (v != apAutoRestore)
        {
            apAutoRestore = v;
            dashLog("[CFG] AP/EAP auto-restore " + String(v ? "ON" : "OFF"));
        }
    }
    if (server.hasArg("hw3OffsetSlew"))
    {
        bool v = server.arg("hw3OffsetSlew") == "1";
        if (v != hw3OffsetSlew)
        {
            hw3OffsetSlew = v;
            dashLog("[CFG] HW3 offset slew " + String(v ? "ON" : "OFF"));
        }
    }
    if (server.hasArg("hw3SlewRate"))
    {
        uint8_t v = dashClampHw3SlewRate(server.arg("hw3SlewRate").toInt());
        if (v != hw3SlewRate)
        {
            hw3SlewRate = v;
            dashLog("[CFG] HW3 slew rate " + String(hw3SlewRate) + "%/s");
        }
    }
    // ─── HW3 custom speed-limit boost ────────────────────────────────────────
    if (server.hasArg("hw3CustomSpeed"))
    {
        bool v = server.arg("hw3CustomSpeed") == "1";
        if (v != hw3CustomSpeed)
        {
            hw3CustomSpeed = v;
            dashLog("[CFG] HW3 custom speed " + String(v ? "ON" : "OFF"));
        }
    }
    if (server.hasArg("hw3HighSpeedEnable"))
    {
        bool v = server.arg("hw3HighSpeedEnable") == "1";
        if (v != hw3HighSpeedEnable)
        {
            hw3HighSpeedEnable = v;
            dashLog("[CFG] HW3 high-speed " + String(v ? "ON" : "OFF"));
        }
    }
    if (server.hasArg("hw3WireEncoding"))
    {
        int v = server.arg("hw3WireEncoding").toInt();
        uint8_t enc = (v == kHw3WireEncKph5) ? kHw3WireEncKph5 : kHw3WireEncPct4;
        if (enc != hw3WireEncoding)
        {
            hw3WireEncoding = enc;
            dashLog(String("[CFG] HW3 wire enc ") +
                    (enc == kHw3WireEncPct4 ? "PCT4" : "KPH5"));
        }
    }
    {
        char arg[16];
        for (uint8_t i = 0; i < kHw3CustomTargetCount; i++)
        {
            snprintf(arg, sizeof(arg), "hw3CustomT%u", (unsigned)i);
            if (server.hasArg(arg))
                hw3CustomTarget[i] = dashClampHw3CustomTargetForBucket(i,
                    server.arg(arg).toInt());
        }
        for (uint8_t i = 0; i < kHw3HighSpeedBucketCount; i++)
        {
            snprintf(arg, sizeof(arg), "hw3HighTarget%u", (unsigned)i);
            if (server.hasArg(arg))
                hw3HighSpeedTarget[i] = dashClampHw3HighSpeedTargetForBucket(i,
                    server.arg(arg).toInt());
        }
    }
    // ─── Legacy MPP custom speed-limit override ──────────────────────────────
    if (server.hasArg("legacyMppOverride"))
    {
        bool v = server.arg("legacyMppOverride") == "1";
        if (v != legacyMppOverride)
        {
            legacyMppOverride = v;
            dashLog("[CFG] Legacy MPP override " + String(v ? "ON" : "OFF"));
        }
    }
    if (server.hasArg("legacyMppCustomEnable"))
    {
        bool v = server.arg("legacyMppCustomEnable") == "1";
        if (v != legacyMppCustomEnable)
        {
            legacyMppCustomEnable = v;
            dashLog("[CFG] Legacy MPP custom " + String(v ? "ON" : "OFF"));
        }
    }
    if (server.hasArg("legacyMppHighSpeedEnable"))
    {
        bool v = server.arg("legacyMppHighSpeedEnable") == "1";
        if (v != legacyMppHighSpeedEnable)
        {
            legacyMppHighSpeedEnable = v;
            dashLog("[CFG] Legacy MPP high-speed " + String(v ? "ON" : "OFF"));
        }
    }
    {
        char arg[28];
        for (uint8_t i = 0; i < kLegacyMppCustomTargetCount; i++)
        {
            snprintf(arg, sizeof(arg), "legacyMppCustomT%u", (unsigned)i);
            if (server.hasArg(arg))
                legacyMppCustomTarget[i] = dashClampLegacyMppCustomTargetForBucket(i,
                    server.arg(arg).toInt());
        }
        for (uint8_t i = 0; i < kLegacyMppHighSpeedBucketCount; i++)
        {
            snprintf(arg, sizeof(arg), "legacyMppHighTarget%u", (unsigned)i);
            if (server.hasArg(arg))
                legacyMppHighSpeedTarget[i] = dashClampLegacyMppHighSpeedTargetForBucket(i,
                    server.arg(arg).toInt());
        }
    }
    if (hwChanged)
    {
        dashSwapHandler(hwMode);
        dashApplyFilters();
    }
    dashApplyRuntimeState();
    dashSavePrefs();
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleLoggingConfig()
{
    if (server.hasArg("eprn") && dashHandler)
    {
        bool ep = server.arg("eprn") == "1";
        dashHandler->enablePrint = ep;
        dashLog("[CFG] Logging " + String(ep ? "ON" : "OFF"));
    }
    dashApplyRuntimeState();
    dashSavePrefs();
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleFrames()
{
    String j = "{\"frames\":[";
    int start = (sniffCount < SNIFFER_CAP) ? 0 : sniffHead;
    int count = min(sniffCount, SNIFFER_CAP);
    for (int i = 0; i < count; i++)
    {
        int idx = (start + i) % SNIFFER_CAP;
        SniffFrame &f = sniffBuf[idx];
        if (i)
            j += ",";
        j += "{\"ts\":" + String(f.ts) +
             ",\"id\":" + String(f.id) +
             ",\"dlc\":" + String(f.dlc) +
             ",\"data\":[";
        for (int b = 0; b < f.dlc; b++)
        {
            if (b)
                j += ",";
            j += String(f.data[b]);
        }
        j += "],\"name\":\"" + jsonEscape(decodeCanId(f.id)) + "\"}";
    }
    j += "]}";
    server.send(200, "application/json", j);
}

static void handleLog()
{
    // Pick up any new per-frame handler diagnostics first.
    dashDrainLogRing();
    unsigned long since = 0;
    if (server.hasArg("since"))
        since = strtoul(server.arg("since").c_str(), nullptr, 10);
    String j = "{\"seq\":";
    j += logSeq;
    j += ",\"lines\":[";
    int start = (logCount < LOG_CAP) ? 0 : logHead;
    int count = min(logCount, LOG_CAP);
    bool first = true;
    for (int i = 0; i < count; i++)
    {
        int idx = (start + i) % LOG_CAP;
        if (logBuf[idx].seq <= since)
            continue;
        if (!first)
            j += ",";
        first = false;
        j += "\"" + jsonEscape(logBuf[idx].msg) + "\"";
    }
    j += "]}";
    server.send(200, "application/json", j);
}

static void handleResetStats()
{
    rxCount = 0;
    txCount = 0;
    txErrCount = 0;
    memset(muxRx, 0, sizeof(muxRx));
    memset(muxTx, 0, sizeof(muxTx));
    memset(muxErr, 0, sizeof(muxErr));
    dashResetWriteProbe();
    dashLog("[CFG] Stats reset");
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleRecStart()
{
    if (!dashEnsureRecBuffer())
    {
        server.send(500, "application/json", "{\"ok\":false,\"error\":\"recorder buffer allocation failed\"}");
        return;
    }
    // Optional ID filter: /rec/start?ids=249,3E9,3F5 (hex, comma-separated).
    // Records only those IDs (on any bus) so the busy primary bus does not flood
    // the buffer while you capture lighting/stalk frames for checksum analysis.
    recFilterCount = 0;
    if (server.hasArg("ids"))
    {
        String s = server.arg("ids");
        const char *p = s.c_str();
        while (*p && recFilterCount < kRecFilterMax)
        {
            while (*p == ',' || *p == ' ')
                p++;
            if (!*p)
                break;
            char *end = nullptr;
            uint32_t v = static_cast<uint32_t>(strtoul(p, &end, 16));
            if (end == p)
                break; // no progress: avoid infinite loop on junk input
            recFilterIds[recFilterCount++] = v;
            p = end;
        }
    }
    // Optional exclude/blacklist: /rec_start?exclude=118,3FD (hex, comma-sep).
    // Frames with these IDs are dropped — broad capture without the noisy frames.
    recExcludeCount = 0;
    if (server.hasArg("exclude"))
    {
        String s = server.arg("exclude");
        const char *p = s.c_str();
        while (*p && recExcludeCount < kRecFilterMax)
        {
            while (*p == ',' || *p == ' ')
                p++;
            if (!*p)
                break;
            char *end = nullptr;
            uint32_t v = static_cast<uint32_t>(strtoul(p, &end, 16));
            if (end == p)
                break;
            recExcludeIds[recExcludeCount++] = v;
            p = end;
        }
    }
    recCount = 0;
    recSaved = false;
    recStartMs = millis();
    recActive = true;
    dashLog(String("[REC] Recording started")
            + (recFilterCount > 0 ? String(" (filter ") + recFilterCount + " ids)" : String())
            + (recExcludeCount > 0 ? String(" (exclude ") + recExcludeCount + " ids)" : String()));
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleRecStop()
{
    bool ok = dashStopRecordingAndSave("manual");
    server.send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

static void handleRecStatus()
{
    if (recActive && (millis() - recStartMs >= kRecMaxDurationMs))
        dashStopRecordingAndSave("time limit");
    String j = "{\"active\":";
    j += recActive ? "true" : "false";
    j += ",\"count\":";
    j += recCount;
    j += ",\"cap\":";
    j += REC_CAP;
    j += ",\"saved\":";
    j += recSaved ? "true" : "false";
    j += ",\"psram\":";
    j += recBufInPsram ? "true" : "false";
    j += ",\"filter\":";
    j += recFilterCount;
    j += "}";
    server.send(200, "application/json", j);
}

#ifdef DRIVER_T2CAN_DUAL
// Defined in main.cpp — controls the bus B (X197 pin 9/10) service-mode injector
// and exposes the discovered-ID table sniffed on bus B.
void t2canSetServiceMode(bool on);
bool t2canGetServiceMode(void);
uint16_t t2canBus2IdCount(void);
bool t2canBus2IdAt(uint16_t i, uint16_t *id, uint8_t *dlc, uint8_t *data, uint32_t *count);
void t2canStalkTest(uint8_t status, uint16_t durationMs); // status 1=PULL flash, 2=PUSH high beam

static void handleServiceMode()
{
    if (server.hasArg("on"))
        t2canSetServiceMode(server.arg("on") == "1");
    server.send(200, "application/json",
                String("{\"service_mode\":") + (t2canGetServiceMode() ? "true" : "false") + "}");
}

// Flash-burst trigger (爆闪): two real stalk PULL events within 2s fire a
// configured pattern of N PULL flashes. Accepts ?on=0/1 and/or
// ?count=N&on_ms=X&off_ms=Y. Limits enforced firmware-side (count 1-20,
// on/off 80-1000ms). Returns the latched enable + current params.
static void handleBurst()
{
    if (server.hasArg("on"))
        t2canSetBurstEnabled(server.arg("on") == "1");
    if (server.hasArg("count") && server.hasArg("on_ms") && server.hasArg("off_ms"))
    {
        uint8_t cnt = (uint8_t)server.arg("count").toInt();
        uint16_t onMs = (uint16_t)server.arg("on_ms").toInt();
        uint16_t offMs = (uint16_t)server.arg("off_ms").toInt();
        t2canSetBurstParams(cnt, onMs, offMs);
    }
    String j = "{\"enabled\":";
    j += t2canGetBurstEnabled() ? "true" : "false";
    j += ",\"count\":";
    j += t2canGetBurstCount();
    j += ",\"on_ms\":";
    j += t2canGetBurstOnMs();
    j += ",\"off_ms\":";
    j += t2canGetBurstOffMs();
    j += "}";
    server.send(200, "application/json", j);
}

// Bus2 acquisition filter: ?on=1 narrows the MCP2515 hardware filters to only
// the IDs the lighting logic uses (0x249/0x3F5); ?on=0 = accept-all (default,
// for sniffer/recorder discovery). Returns the latched state.
static void handleBus2Filter()
{
    if (server.hasArg("on"))
        t2canSetBus2Filter(server.arg("on") == "1");
    server.send(200, "application/json",
                String("{\"bus2_filter\":") + (t2canGetBus2Filter() ? "true" : "false") + "}");
}

// Inject a short 0x249 stalk burst on bus B to test manual lighting:
//   /stalk_test?mode=flash     -> PULL (超车闪)  ~400ms
//   /stalk_test?mode=highbeam  -> PUSH (远光toggle) ~1000ms
static void handleStalkTest()
{
    String m = server.hasArg("mode") ? server.arg("mode") : "";
    uint8_t status = 0;
    uint16_t dur = 0;
    if (m == "highbeam")
    {
        status = 2;
        dur = 1000;
    }
    else if (m == "flash")
    {
        status = 1;
        dur = 400;
    }
    if (status)
        t2canStalkTest(status, dur);
    server.send(200, "application/json",
                String("{\"ok\":") + (status ? "true" : "false") + ",\"mode\":\"" + m + "\"}");
}

static void handleBus2Ids()
{
    uint16_t n = t2canBus2IdCount();
    String j = "{\"count\":";
    j += n;
    j += ",\"ids\":[";
    for (uint16_t i = 0; i < n; i++)
    {
        uint16_t id = 0;
        uint8_t dlc = 0;
        uint8_t data[8] = {0};
        uint32_t cnt = 0;
        if (!t2canBus2IdAt(i, &id, &dlc, data, &cnt))
            break;
        if (i)
            j += ",";
        char idbuf[8];
        snprintf(idbuf, sizeof(idbuf), "%03X", id);
        j += "{\"id\":\"";
        j += idbuf;
        j += "\",\"dlc\":";
        j += dlc;
        j += ",\"count\":";
        j += cnt;
        j += ",\"data\":\"";
        for (uint8_t b = 0; b < dlc && b < 8; b++)
        {
            char hb[4];
            snprintf(hb, sizeof(hb), "%02X", data[b]);
            if (b)
                j += " ";
            j += hb;
        }
        j += "\"}";
    }
    j += "]}";
    server.send(200, "application/json", j);
}
#endif

static void handleRecDownload()
{
    if (!SPIFFS.exists("/rec.csv"))
    {
        server.send(404, "text/plain", "No recording saved yet");
        return;
    }
    File f = SPIFFS.open("/rec.csv", "r");
    server.sendHeader("Content-Disposition", "attachment; filename=\"can_recording.csv\"");
    server.streamFile(f, "text/csv");
    f.close();
}

static void handleDisable()
{
    dashSetCanActive(false, "dashboard");
    server.send(200, "text/plain", "Injection stopped.");
}

static void handleReboot()
{
    server.send(200, "text/plain", "Rebooting...");
    delay(200);
    ESP.restart();
}

static void handleOtaResult()
{
    if (!server.authenticate(DASH_OTA_USER, DASH_OTA_PASS))
    {
        server.requestAuthentication();
        return;
    }
    bool ok = Update.isFinished() && !Update.hasError();
    server.sendHeader("Connection", "close");
    server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : Update.errorString());
    if (ok)
    {
        dashLog("[OTA] Upload complete -- rebooting");
        delay(300);
        ESP.restart();
    }
    else
    {
        dashLog("[OTA] Upload FAILED: " + String(Update.errorString()));
        Update.abort();
    }
}

static void handleOtaUpload()
{
    if (!server.authenticate(DASH_OTA_USER, DASH_OTA_PASS))
        return;
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        dashLog("[OTA] Receiving: " + String(upload.filename.c_str()));
        esp_task_wdt_deinit();
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
            dashLog("[OTA] Begin failed: " + String(Update.errorString()));
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            dashLog("[OTA] Write error: " + String(Update.errorString()));
            Update.abort();
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (upload.totalSize > 0 && Update.end(true) && Update.isFinished())
            dashLog("[OTA] Done: " + String(upload.totalSize) + " bytes");
        else
        {
            dashLog("[OTA] End failed: " + String(Update.errorString()));
            Update.abort();
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        dashLog("[OTA] Upload aborted");
        Update.abort();
    }
}

// CAN RUNTIME MANAGEMENT

static String dashFrameDataJson(const CanFrame &frame)
{
    String j = "[";
    for (uint8_t i = 0; i < 8; i++)
    {
        if (i)
            j += ",";
        j += String(frame.data[i]);
    }
    j += "]";
    return j;
}

static String dashFrameDataHex(const CanFrame &frame)
{
    String out;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (i)
            out += " ";
        if (frame.data[i] < 16)
            out += "0";
        out += String(frame.data[i], HEX);
    }
    out.toUpperCase();
    return out;
}

// ── WIFI STA ────────────────────────────────────────────────────

static bool dashStartAccessPoint(bool withSta)
{
    WiFi.persistent(false);
    WiFi.mode(withSta ? WIFI_AP_STA : WIFI_AP);
    WiFi.setSleep(false);

    IPAddress apIp(100, 100, 1, 1);
    IPAddress apMask(255, 255, 255, 0);
    WiFi.softAPConfig(apIp, apIp, apMask);

    if (!dashApConfigValid(apSSID, apPass))
        dashUseDefaultApConfig();

    apRuntimeChannel = dashConfiguredApChannel();
    bool ok = WiFi.softAP(apSSID, apPass, apRuntimeChannel, apHidden ? 1 : 0, kDashApMaxConn);
    if (!ok)
    {
        dashUseDefaultApConfig();
        ok = WiFi.softAP(apSSID, apPass, apRuntimeChannel, 0, kDashApMaxConn);
    }
    if (!ok)
        dashLog("[WIFI] AP start failed");
    else
        dashGatewayOnApStarted(WiFi.apNetif());
    return ok;
}

static void dashBeginSTA()
{
    if (strlen(staSSID) == 0)
        return;

    // Ensure STA interface is enabled (AP+STA) before initiating a STA connect.
    // Without this, esp_wifi_set_config(WIFI_IF_STA, ...) inside WiFi.begin()
    // can fail silently when the device is in AP-only mode.
    if (WiFi.getMode() != WIFI_AP_STA)
        WiFi.mode(WIFI_AP_STA);

    if (staStaticIP && (uint32_t)staIP != 0)
    {
        WiFi.config(staIP, staGW, staMask, staDNS);
        dashLog("[WIFI] Static IP: " + staIP.toString());
    }
    else
    {
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }
    // Disconnect any prior STA association before issuing a fresh begin().
    // Back-to-back WiFi.begin() without disconnect can leave esp_wifi in an
    // intermediate state and trigger an extra channel switch on the shared
    // AP+STA radio, which the AP beacon picks up as jitter (clients on
    // 100.100.1.1 momentarily see the AP go away). eraseAP=false keeps the
    // soft-AP up; wifioff=false keeps the radio on.
    WiFi.disconnect(false, false);
    WiFi.begin(staSSID, staPass);
    staConnectAttemptActive = true;
    staConnectStartedAt = millis();
    staRetryAt = 0;
    dashLog("[WIFI] Connecting to " + String(staSSID) + "...");
}

static void dashPrepareStaReconnect()
{
    if (staConnectAttemptActive || staConnected || WiFi.status() == WL_CONNECTED)
        WiFi.disconnect(false, false);
    dashGatewayOnStaDisconnected(WiFi.apNetif());
    staConnected = false;
    staConnectAttemptActive = false;
    staRetryAt = 0;
    staConsecutiveFailures = 0; // user-initiated reconnect resets diagnostics
    autoUpdateEligibleAt = 0;
}

static void dashApplyWifiSlot(uint8_t slot)
{
    if (slot >= wifiNetworkCount)
        return;
    const DashWifiNetwork &n = wifiNetworks[slot];
    strlcpy(staSSID, n.ssid, sizeof(staSSID));
    strlcpy(staPass, n.pass, sizeof(staPass));
    staStaticIP = n.useStatic;
    if (n.useStatic)
    {
        staIP.fromString(n.ip);
        staGW.fromString(n.gw);
        staMask.fromString(n.mask);
        staDNS.fromString(n.dns);
    }
    else
    {
        staIP = IPAddress(0, 0, 0, 0);
    }
    wifiActiveSlot = static_cast<int8_t>(slot);
}

static void dashRotateAndConnect()
{
    if (wifiNetworkCount == 0)
        return;
    // Rotate through saved slots, skipping any slot whose SSID matches our own AP
    // (connecting to ourselves would bring down the AP and disconnect all clients).
    for (uint8_t tries = 0; tries < wifiNetworkCount; tries++)
    {
        uint8_t next = wifiNextRotateSlot % wifiNetworkCount;
        wifiNextRotateSlot = (next + 1) % wifiNetworkCount;
        dashApplyWifiSlot(next);
        if (strlen(apSSID) > 0 && strcmp(staSSID, apSSID) == 0)
        {
            dashLog("[WIFI] Skipping slot " + String(next) + " (matches own AP SSID)");
            continue;
        }
        dashLog("[WIFI] Trying slot " + String(next) + ": " + String(staSSID));
        // AP is already running in AP+STA mode since boot; don't restart it.
        // Only re-assert AP_STA mode if it was actually changed, to avoid
        // unnecessary esp_wifi_set_mode() calls that can briefly disturb the
        // shared AP+STA radio. dashBeginSTA() also sets AP_STA defensively.
        if (WiFi.getMode() != WIFI_AP_STA)
            WiFi.mode(WIFI_AP_STA);
        dashBeginSTA();
        return;
    }
    dashLog("[WIFI] No connectable STA slots (all match own AP SSID?)");
}

static void dashScheduleSTAConnect(unsigned long delayMs)
{
    if (strlen(staSSID) == 0)
        return;
    staConnectAttemptActive = false;
    staRetryAt = millis() + delayMs;
}

static void dashPrepareWifiScan()
{
    if (WiFi.getMode() != WIFI_AP_STA)
        WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
}

static uint8_t dashCurrentApChannel()
{
#ifdef ESP_PLATFORM
    wifi_config_t cfg = {};
    if (esp_wifi_get_config(WIFI_IF_AP, &cfg) == ESP_OK && cfg.ap.channel > 0)
        return cfg.ap.channel;
#endif
    return apRuntimeChannel;
}

static bool dashSyncApChannelToSta()
{
#ifdef ESP_PLATFORM
    wifi_ap_record_t staInfo = {};
    if (esp_wifi_sta_get_ap_info(&staInfo) != ESP_OK || staInfo.primary == 0)
        return false;

    uint8_t staChannel = staInfo.primary;
    uint8_t apChannel = dashCurrentApChannel();
    apLastChannelSyncTarget = staChannel;
    apLastChannelSyncMs = millis();

    if (apChannel == staChannel)
    {
        apRuntimeChannel = staChannel;
        apLastChannelSyncOk = true;
        dashLog("[WIFI] AP channel already matches STA CH" + String(staChannel));
        return true;
    }

    wifi_config_t cfg = {};
    esp_err_t err = esp_wifi_get_config(WIFI_IF_AP, &cfg);
    if (err == ESP_OK)
    {
        cfg.ap.channel = staChannel;
        err = esp_wifi_set_config(WIFI_IF_AP, &cfg);
    }

    if (err == ESP_OK)
    {
        apRuntimeChannel = staChannel;
        apLastChannelSyncOk = true;
        dashLog("[WIFI] AP channel auto matched: AP CH" + String(apChannel) +
                " -> STA CH" + String(staChannel));
        return true;
    }

    apLastChannelSyncOk = false;
    dashLog("[WIFI] AP channel auto match failed: AP CH" + String(apChannel) +
            " STA CH" + String(staChannel) + " err=" + String(esp_err_to_name(err)));
#endif
    return false;
}

static const char *dashWifiStatusName(int status)
{
    switch (status)
    {
    case WL_IDLE_STATUS:
        return "IDLE";
    case WL_NO_SSID_AVAIL:
        return "NO_SSID";
    case WL_SCAN_COMPLETED:
        return "SCAN_DONE";
    case WL_CONNECTED:
        return "CONNECTED";
    case WL_CONNECT_FAILED:
        return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
        return "CONNECTION_LOST";
    case WL_DISCONNECTED:
        return "DISCONNECTED";
    default:
        return "UNKNOWN";
    }
}

static void performAutoUpdate(); // forward decl, defined below

static void dashCheckWifi()
{
    static unsigned long lastCheck = 0;
    if (wifiNetworkCount == 0)
        return;
    unsigned long now = millis();
    if (!staConnected && !staConnectAttemptActive && staRetryAt > 0 && (long)(now - staRetryAt) >= 0)
    {
        staRetryAt = 0;
        dashRotateAndConnect();
        return; // let the fresh attempt age from its own start timestamp
    }

    unsigned long checkInterval = staConnectAttemptActive ? 250UL : 1000UL;
    if (now - lastCheck < checkInterval)
        return;
    lastCheck = now;

    int wifiStatus = WiFi.status();
    bool connected = wifiStatus == WL_CONNECTED;
    bool timedOut = !connected && staConnectAttemptActive &&
                    now - staConnectStartedAt >= kDashStaConnectTimeoutMs;
    if (timedOut)
    {
        uint8_t reason = WiFi.lastDisconnectReason();
        const char *reasonName = WiFi.lastDisconnectReasonName();
        staConnectAttemptActive = false;
        WiFi.disconnect(false, false);
        dashGatewayOnStaDisconnected(WiFi.apNetif());
        if (staConsecutiveFailures < 255)
            staConsecutiveFailures++;
        staRetryAt = now + kDashStaSavedPollMs;
        dashLog("[WIFI] STA connect timed out; status=" + String(dashWifiStatusName(wifiStatus)) +
                " reason=" + String(reasonName) + "(" + String(reason) + ")" +
                " retry saved networks in " + String(kDashStaSavedPollMs / 1000) +
                "s, AP+STA stays up (fail#" + String(staConsecutiveFailures) + ")");
        connected = false;
    }

    if (connected != staConnected)
    {
        staConnected = connected;
        if (connected)
        {
            staConnectAttemptActive = false;
            staRetryAt = 0;
            staConsecutiveFailures = 0; // reset diagnostics on successful connect
            dashLog("[WIFI] Connected to " + String(staSSID) + " IP: " + WiFi.localIP().toString());
            dashSyncApChannelToSta();
            dashGatewayOnStaConnected(WiFi.staNetif(), WiFi.apNetif());
            // Remember which slot just succeeded so the next reboot tries it
            // first. Avoids rotating through stale/dead networks on every boot.
            if (wifiActiveSlot >= 0 && wifiActiveSlot < (int8_t)wifiNetworkCount)
            {
                prefs.begin(PREFS_NS, false);
                if ((int8_t)prefs.getUChar("wn_pref", 0xFF) != wifiActiveSlot)
                    prefs.putUChar("wn_pref", static_cast<uint8_t>(wifiActiveSlot));
                prefs.end();
            }
            // Schedule auto-update check 15 s after STA comes up (grace period for other boot work)
            if (autoUpdateEnabled && !autoUpdateDone)
                autoUpdateEligibleAt = millis() + 15000;
        }
        else
        {
            if (staConsecutiveFailures < 255)
                staConsecutiveFailures++;
            dashLog("[WIFI] Disconnected from " + String(staSSID) +
                    "; retry saved networks in " + String(kDashStaSavedPollMs / 1000) + "s (fail#" +
                    String(staConsecutiveFailures) + ")");
            dashGatewayOnStaDisconnected(WiFi.apNetif());
            staConnectAttemptActive = false;
            staRetryAt = now + kDashStaSavedPollMs;
        }
    }

    // Fire one-shot auto-update check once eligible
    if (autoUpdateEnabled && !autoUpdateDone && staConnected && autoUpdateEligibleAt > 0 && millis() >= autoUpdateEligibleAt)
    {
        autoUpdateDone = true;
        performAutoUpdate();
    }

}

// Cached scan results — a full-channel scan in APSTA mode briefly drops the
// AP beacon, so we do NOT scan more often than kDashScanMinIntervalMs even if
// the WebUI keeps polling. Cached JSON is returned for repeat calls inside the
// window, and a 429 with retry-after is returned if the cache is empty.
static String dashCachedScanJson;
static unsigned long dashLastScanAt = 0;
static constexpr unsigned long kDashScanMinIntervalMs = 30000;

static void handleWifiScan()
{
    unsigned long now = millis();
    bool force = server.hasArg("force") && server.arg("force") == "1";
    if (!force && dashLastScanAt != 0 && (now - dashLastScanAt) < kDashScanMinIntervalMs)
    {
        if (dashCachedScanJson.length() > 0)
        {
            server.send(200, "application/json", dashCachedScanJson);
        }
        else
        {
            unsigned long retryMs = kDashScanMinIntervalMs - (now - dashLastScanAt);
            server.sendHeader("Retry-After", String((retryMs + 999) / 1000).c_str());
            server.send(429, "application/json",
                        String("{\"ok\":false,\"error\":\"scan-throttled\",\"retry_ms\":") +
                            String(retryMs) + "}");
        }
        return;
    }
    // Delete any lingering async scan result before triggering a new blocking scan.
    // In APSTA mode a full-channel scan briefly pauses AP beacon delivery.
    // This is user-triggered only; we do NOT scan automatically on reconnect.
    WiFi.scanDelete();
    dashPrepareWifiScan();
    int n = WiFi.scanNetworks(false, false, false, 300);
    String j = "{\"networks\":[";
    for (int i = 0; i < n && i < 20; i++)
    {
        if (i)
            j += ",";
        j += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i).c_str()) + "\"";
        j += ",\"rssi\":" + String(WiFi.RSSI(i));
        wifi_auth_mode_t auth = WiFi.encryptionType(i);
        j += ",\"enc\":" + String(auth != WIFI_AUTH_OPEN ? "true" : "false");
        j += ",\"auth\":" + String(static_cast<int>(auth));
        j += ",\"ch\":" + String(WiFi.channel(i));
        j += "}";
    }
    j += "]}";
    WiFi.scanDelete();
    dashCachedScanJson = j;
    dashLastScanAt = millis();
    server.send(200, "application/json", j);
}

static void dashPersistWifiSlot(uint8_t slot)
{
    if (slot >= wifiNetworkCount)
        return;
    const DashWifiNetwork &n = wifiNetworks[slot];
    prefs.putString(dashWifiKey(slot, "s").c_str(), String(n.ssid));
    prefs.putString(dashWifiKey(slot, "p").c_str(), String(n.pass));
    prefs.putBool(dashWifiKey(slot, "t").c_str(), n.useStatic);
    if (n.useStatic)
    {
        prefs.putString(dashWifiKey(slot, "i").c_str(), String(n.ip));
        prefs.putString(dashWifiKey(slot, "g").c_str(), String(n.gw));
        prefs.putString(dashWifiKey(slot, "m").c_str(), String(n.mask));
        prefs.putString(dashWifiKey(slot, "d").c_str(), String(n.dns));
    }
    else
    {
        prefs.remove(dashWifiKey(slot, "i").c_str());
        prefs.remove(dashWifiKey(slot, "g").c_str());
        prefs.remove(dashWifiKey(slot, "m").c_str());
        prefs.remove(dashWifiKey(slot, "d").c_str());
    }
}

static void dashRemoveWifiSlotKeys(uint8_t slot)
{
    prefs.remove(dashWifiKey(slot, "s").c_str());
    prefs.remove(dashWifiKey(slot, "p").c_str());
    prefs.remove(dashWifiKey(slot, "t").c_str());
    prefs.remove(dashWifiKey(slot, "i").c_str());
    prefs.remove(dashWifiKey(slot, "g").c_str());
    prefs.remove(dashWifiKey(slot, "m").c_str());
    prefs.remove(dashWifiKey(slot, "d").c_str());
}

// Save to slot N (0..count). idx == count means append (new). Reconnect on save.
static void handleWifiConfig()
{
    if (!server.hasArg("ssid"))
    {
        server.send(200, "application/json", "{\"ok\":true}");
        return;
    }

    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    if (ssid.length() == 0 || ssid.length() > kDashMaxSsidLen || dashStaSsidLooksCorrupt(ssid))
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid SSID or password\"}");
        return;
    }

    int idx = -1;
    if (server.hasArg("idx"))
        idx = server.arg("idx").toInt();
    if (idx < 0 || idx > wifiNetworkCount)
        idx = wifiNetworkCount; // append

    if (idx == kDashMaxWifiNetworks)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Max networks reached\"}");
        return;
    }

    String effectivePass = pass;
    if (idx >= 0 && idx < wifiNetworkCount && pass.length() == 0 && strlen(wifiNetworks[idx].pass) > 0)
        effectivePass = wifiNetworks[idx].pass;
    if (!dashStaConfigLengthValid(ssid, effectivePass))
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid SSID or password\"}");
        return;
    }

    DashWifiNetwork &n = wifiNetworks[idx];
    dashClearWifiNetwork(n);
    strlcpy(n.ssid, ssid.c_str(), sizeof(n.ssid));
    strlcpy(n.pass, effectivePass.c_str(), sizeof(n.pass));
    n.useStatic = server.hasArg("static") && server.arg("static") == "1";
    if (n.useStatic)
    {
        strlcpy(n.ip, server.arg("ip").c_str(), sizeof(n.ip));
        strlcpy(n.gw, server.arg("gw").c_str(), sizeof(n.gw));
        strlcpy(n.mask, server.arg("mask").c_str(), sizeof(n.mask));
        strlcpy(n.dns, server.arg("dns").c_str(), sizeof(n.dns));
    }

    if (idx == wifiNetworkCount)
        wifiNetworkCount++;

    prefs.begin(PREFS_NS, false);
    prefs.putUChar("wn_cnt", wifiNetworkCount);
    dashPersistWifiSlot(idx);
    prefs.end();

    dashLog("[WIFI] Saved slot " + String(idx) + ": " + ssid);

    // Switch to newly saved slot and connect
    wifiNextRotateSlot = idx;
    dashApplyWifiSlot(idx);
    dashPrepareStaReconnect();

    server.send(200, "application/json", "{\"ok\":true,\"idx\":" + String(idx) + "}");
    dashScheduleSTAConnect(1000);
}

static void handleWifiConnect()
{
    if (!server.hasArg("idx"))
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing idx\"}");
        return;
    }
    int idx = server.arg("idx").toInt();
    if (idx < 0 || idx >= wifiNetworkCount)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad idx\"}");
        return;
    }

    dashApplyWifiSlot(static_cast<uint8_t>(idx));
    if (strlen(apSSID) > 0 && strcmp(staSSID, apSSID) == 0)
    {
        server.send(409, "application/json", "{\"ok\":false,\"error\":\"SSID matches AP hotspot\"}");
        return;
    }

    wifiNextRotateSlot = static_cast<uint8_t>(idx);
    dashPrepareStaReconnect();
    dashLog("[WIFI] Manual connect slot " + String(idx) + ": " + String(staSSID));

    server.send(200, "application/json",
                "{\"ok\":true,\"idx\":" + String(idx) +
                    ",\"ssid\":\"" + jsonEscape(staSSID) + "\"}");
    dashScheduleSTAConnect(100);
}

static void handleWifiDelete()
{
    if (!server.hasArg("idx"))
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing idx\"}");
        return;
    }
    int idx = server.arg("idx").toInt();
    if (idx < 0 || idx >= wifiNetworkCount)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad idx\"}");
        return;
    }

    String removedSsid = wifiNetworks[idx].ssid;
    // Shift slots down
    for (uint8_t i = idx; i + 1 < wifiNetworkCount; i++)
        wifiNetworks[i] = wifiNetworks[i + 1];
    wifiNetworkCount--;
    dashClearWifiNetwork(wifiNetworks[wifiNetworkCount]);

    // Rewrite all slot keys
    prefs.begin(PREFS_NS, false);
    prefs.putUChar("wn_cnt", wifiNetworkCount);
    for (uint8_t i = 0; i < wifiNetworkCount; i++)
        dashPersistWifiSlot(i);
    for (uint8_t i = wifiNetworkCount; i < kDashMaxWifiNetworks; i++)
        dashRemoveWifiSlotKeys(i);
    // Adjust persisted preferred slot to stay within bounds after deletion.
    // If the deleted slot WAS the preferred one, reset to 0 so the next boot
    // doesn't try an empty/stale slot first.
    {
        uint8_t pref = prefs.getUChar("wn_pref", 0);
        if ((int)idx == (int)pref || pref >= wifiNetworkCount)
            prefs.putUChar("wn_pref", 0);
        else if (pref > idx)
            prefs.putUChar("wn_pref", pref - 1);
    }
    prefs.end();

    dashLog("[WIFI] Deleted slot " + String(idx) + ": " + removedSsid);

    // Adjust active slot if needed
    if (wifiActiveSlot == idx)
    {
        wifiActiveSlot = -1;
        if (staConnectAttemptActive || staConnected)
        {
            WiFi.disconnect(false, false);
            dashGatewayOnStaDisconnected(WiFi.apNetif());
            staConnectAttemptActive = false;
            staConnected = false;
        }
        if (wifiNetworkCount > 0)
        {
            wifiNextRotateSlot = 0;
            dashRotateAndConnect();
        }
        else
        {
            staSSID[0] = 0;
            staPass[0] = 0;
            // Keep AP+STA mode active even when no networks are saved.
            if (WiFi.getMode() != WIFI_AP_STA)
                WiFi.mode(WIFI_AP_STA);
        }
    }
    else if (wifiActiveSlot > idx)
    {
        wifiActiveSlot--;
    }
    if (wifiNextRotateSlot >= wifiNetworkCount)
        wifiNextRotateSlot = 0;

    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleWifiNetworks()
{
    String j = "{\"max\":";
    j += kDashMaxWifiNetworks;
    j += ",\"count\":";
    j += wifiNetworkCount;
    j += ",\"active\":";
    j += wifiActiveSlot;
    j += ",\"networks\":[";
    for (uint8_t i = 0; i < wifiNetworkCount; i++)
    {
        if (i)
            j += ",";
        const DashWifiNetwork &n = wifiNetworks[i];
        j += "{\"idx\":";
        j += i;
        j += ",\"ssid\":\"" + jsonEscape(n.ssid) + "\"";
        j += ",\"hasPass\":" + String(strlen(n.pass) > 0 ? "true" : "false");
        j += ",\"static\":" + String(n.useStatic ? "true" : "false");
        if (n.useStatic)
        {
            j += ",\"ip\":\"" + String(n.ip) + "\"";
            j += ",\"gw\":\"" + String(n.gw) + "\"";
            j += ",\"mask\":\"" + String(n.mask) + "\"";
            j += ",\"dns\":\"" + String(n.dns) + "\"";
        }
        j += "}";
    }
    j += "]}";
    server.send(200, "application/json", j);
}

static void handleWifiStatus()
{
    bool stored = wifiNetworkCount > 0;
    bool connectedNow = WiFi.status() == WL_CONNECTED;
    IPAddress staIp = WiFi.localIP();
    bool connected = connectedNow;
    String activeSsid = connectedNow ? WiFi.SSID() : String(staSSID);
    if (dashStaSsidLooksCorrupt(activeSsid))
        activeSsid = "";
    String j = "{\"connected\":";
    j += connected ? "true" : "false";
    j += ",\"ssid\":\"" + jsonEscape(activeSsid) + "\"";
    j += ",\"stored\":" + String(stored ? "true" : "false");
    j += ",\"count\":" + String(wifiNetworkCount);
    j += ",\"active\":" + String(wifiActiveSlot);
    int wifiStatus = WiFi.status();
    j += ",\"wifi_status\":" + String(wifiStatus);
    j += ",\"wifi_status_name\":\"";
    j += dashWifiStatusName(wifiStatus);
    j += "\"";
    j += ",\"disconnect_reason\":";
    j += String(static_cast<unsigned>(WiFi.lastDisconnectReason()));
    j += ",\"disconnect_reason_name\":\"";
    j += WiFi.lastDisconnectReasonName();
    j += "\"";
    if (staConnectAttemptActive)
        j += ",\"attempt_age_s\":" + String((millis() - staConnectStartedAt) / 1000);
    if (connected)
        j += ",\"ip\":\"" + staIp.toString() + "\"";
    j += ",\"static\":" + String(staStaticIP ? "true" : "false");
    if (staStaticIP)
    {
        j += ",\"cfg_ip\":\"" + staIP.toString() + "\"";
        j += ",\"cfg_gw\":\"" + staGW.toString() + "\"";
        j += ",\"cfg_mask\":\"" + staMask.toString() + "\"";
        j += ",\"cfg_dns\":\"" + staDNS.toString() + "\"";
    }
    if (!connected)
        j += ",\"connecting\":" + String(staConnectAttemptActive ? "true" : "false");
    j += ",\"fail_count\":" + String(staConsecutiveFailures);
    if (!connected && staRetryAt > 0)
    {
        unsigned long now = millis();
        long retryInMs = (long)(staRetryAt - now);
        j += ",\"retry_in_s\":" + String(retryInMs > 0 ? (retryInMs / 1000) : 0);
    }
    j += "}";
    server.send(200, "application/json", j);
}

// ── AP Config (hotspot name/password) ───────────────────────────

#ifdef ESP_PLATFORM
static const char *dashResetReasonName(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_POWERON:
        return "poweron";
    case ESP_RST_EXT:
        return "external";
    case ESP_RST_SW:
        return "software";
    case ESP_RST_PANIC:
        return "panic";
    case ESP_RST_INT_WDT:
        return "interrupt_wdt";
    case ESP_RST_TASK_WDT:
        return "task_wdt";
    case ESP_RST_WDT:
        return "other_wdt";
    case ESP_RST_DEEPSLEEP:
        return "deepsleep";
    case ESP_RST_BROWNOUT:
        return "brownout";
    case ESP_RST_SDIO:
        return "sdio";
    default:
        return "unknown";
    }
}

static bool dashReadTemperature(float &celsius)
{
    static temperature_sensor_handle_t tempHandle = nullptr;
    static bool tempReady = false;
    static bool tempTried = false;
    if (!tempTried)
    {
        tempTried = true;
        temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
        if (temperature_sensor_install(&cfg, &tempHandle) == ESP_OK &&
            temperature_sensor_enable(tempHandle) == ESP_OK)
        {
            tempReady = true;
        }
    }
    return tempReady && temperature_sensor_get_celsius(tempHandle, &celsius) == ESP_OK;
}
#endif

#if defined(ESP_PLATFORM) && CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
static void dashReadCpuLoad(uint8_t &core0Load, uint8_t &core1Load, bool &valid)
{
    static uint32_t prevIdle[2] = {0, 0};
    static int64_t prevWallUs = 0;
    static bool havePrev = false;

    core0Load = 0;
    core1Load = 0;
    valid = false;

    constexpr UBaseType_t kMaxTasksForCpuStats = 48;
    TaskStatus_t tasks[kMaxTasksForCpuStats];
    configRUN_TIME_COUNTER_TYPE totalRunTime = 0;
    UBaseType_t count = uxTaskGetSystemState(tasks, kMaxTasksForCpuStats, &totalRunTime);
    if (count == 0)
        return;

    uint32_t idle[2] = {0, 0};
    bool seenIdle[2] = {false, false};
    for (UBaseType_t i = 0; i < count; i++)
    {
        const char *name = tasks[i].pcTaskName ? tasks[i].pcTaskName : "";
        if (strncmp(name, "IDLE", 4) != 0)
            continue;
        BaseType_t core = -1;
        size_t len = strlen(name);
        if (len > 0 && name[len - 1] >= '0' && name[len - 1] <= '1')
            core = name[len - 1] - '0';
        if (core >= 0 && core <= 1)
        {
            idle[core] = static_cast<uint32_t>(tasks[i].ulRunTimeCounter);
            seenIdle[core] = true;
        }
    }
    if (!seenIdle[0] || !seenIdle[1])
        return;

    int64_t nowUs = esp_timer_get_time();
    if (!havePrev)
    {
        prevIdle[0] = idle[0];
        prevIdle[1] = idle[1];
        prevWallUs = nowUs;
        havePrev = true;
        return;
    }

    uint32_t wallDelta = static_cast<uint32_t>(nowUs - prevWallUs);
    if (wallDelta < 100000)
        return;

    uint32_t idleDelta0 = idle[0] - prevIdle[0];
    uint32_t idleDelta1 = idle[1] - prevIdle[1];
    auto loadFromIdle = [](uint32_t idleDelta, uint32_t elapsedUs) -> uint8_t {
        uint32_t idlePct = elapsedUs ? (idleDelta * 100UL + elapsedUs / 2) / elapsedUs : 0;
        if (idlePct > 100)
            idlePct = 100;
        return static_cast<uint8_t>(100 - idlePct);
    };

    core0Load = loadFromIdle(idleDelta0, wallDelta);
    core1Load = loadFromIdle(idleDelta1, wallDelta);
    valid = true;
    prevIdle[0] = idle[0];
    prevIdle[1] = idle[1];
    prevWallUs = nowUs;
}
#else
static void dashReadCpuLoad(uint8_t &core0Load, uint8_t &core1Load, bool &valid)
{
    core0Load = 0;
    core1Load = 0;
    valid = false;
}
#endif

#ifndef DASH_BOARD_NAME
#define DASH_BOARD_NAME "ESP32-S3R8"
#endif

static void handleSystemStatus()
{
#ifdef ESP_PLATFORM
    esp_chip_info_t chip;
    esp_chip_info(&chip);

    uint32_t flashSize = 0;
    esp_flash_get_size(NULL, &flashSize);

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macText[18];
    snprintf(macText, sizeof(macText), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    wifi_ap_record_t apInfo;
    int rssi = 0;
    bool hasRssi = esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK;
    if (hasRssi)
        rssi = apInfo.rssi;

    size_t spiffsTotal = 0, spiffsUsed = 0;
    bool spiffsOk = esp_spiffs_info(NULL, &spiffsTotal, &spiffsUsed) == ESP_OK;

    const esp_partition_t *running = esp_ota_get_running_partition();
    uint32_t appUsed = 0;
    if (running)
    {
        esp_partition_pos_t runningPos = {};
        runningPos.offset = running->address;
        runningPos.size = running->size;
        esp_image_metadata_t imageMeta = {};
        if (esp_image_get_metadata(&runningPos, &imageMeta) == ESP_OK)
            appUsed = imageMeta.image_len;
    }
    float tempC = 0.0f;
    bool hasTemp = dashReadTemperature(tempC);
    uint32_t cpuHz = static_cast<uint32_t>(esp_clk_cpu_freq());
    uint32_t cpuMhz = (cpuHz + 500000UL) / 1000000UL;
    uint32_t apbMhz = (static_cast<uint32_t>(esp_clk_apb_freq()) + 500000UL) / 1000000UL;
    uint32_t xtalMhz = (static_cast<uint32_t>(esp_clk_xtal_freq()) + 500000UL) / 1000000UL;
    bool pmDynamic = false;
    int pmMinMhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
    int pmMaxMhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
#if CONFIG_PM_ENABLE
    esp_pm_config_t pmConfig;
    if (esp_pm_get_configuration(&pmConfig) == ESP_OK)
    {
        pmDynamic = pmConfig.min_freq_mhz != pmConfig.max_freq_mhz;
        pmMinMhz = pmConfig.min_freq_mhz;
        pmMaxMhz = pmConfig.max_freq_mhz;
    }
#endif
    wifi_mode_t wifiMode = WIFI_MODE_NULL;
    esp_wifi_get_mode(&wifiMode);
    wifi_ps_type_t wifiPs = WIFI_PS_NONE;
    esp_wifi_get_ps(&wifiPs);
    const char *wifiModeText = "off";
    switch (wifiMode)
    {
    case WIFI_MODE_STA:
        wifiModeText = "STA";
        break;
    case WIFI_MODE_AP:
        wifiModeText = "AP";
        break;
    case WIFI_MODE_APSTA:
        wifiModeText = "AP+STA";
        break;
    default:
        wifiModeText = "off";
        break;
    }
    uint8_t cpu0Load = 0, cpu1Load = 0;
    bool hasCpuLoad = false;
    dashReadCpuLoad(cpu0Load, cpu1Load, hasCpuLoad);

    String j = "{\"chip\":\"ESP32-S3\"";
    j += ",\"module\":\"" DASH_BOARD_NAME "\"";
    j += ",\"board\":\"" DASH_BOARD_NAME "\"";
    j += ",\"target\":\"" CONFIG_IDF_TARGET "\"";
    j += ",\"cores\":" + String(chip.cores);
    j += ",\"revision\":" + String(chip.revision);
    j += ",\"cpu_mhz\":" + String(cpuMhz);
    j += ",\"cpu_default_mhz\":" + String(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    j += ",\"cpu_max_mhz\":240";
    j += ",\"cpu_core0_mhz\":" + String(cpuMhz);
    j += ",\"cpu_core1_mhz\":" + String(chip.cores > 1 ? cpuMhz : 0);
    j += ",\"cpu_policy\":\"" + String(pmDynamic ? "dynamic" : "fixed") + "\"";
    j += ",\"cpu_pm_min_mhz\":" + String(pmMinMhz);
    j += ",\"cpu_pm_max_mhz\":" + String(pmMaxMhz);
    j += ",\"cpu_overclock\":" + String(cpuMhz > 240 ? "true" : "false");
    j += ",\"cpu_load_valid\":" + String(hasCpuLoad ? "true" : "false");
    j += ",\"cpu0_load\":" + String(cpu0Load);
    j += ",\"cpu1_load\":" + String(cpu1Load);
    j += ",\"apb_mhz\":" + String(apbMhz);
    j += ",\"xtal_mhz\":" + String(xtalMhz);
    j += ",\"sram_bytes\":524288";
    j += ",\"rtc_sram_bytes\":16384";
    j += ",\"rom_bytes\":393216";
    j += ",\"idf\":\"" IDF_VER "\"";
    j += ",\"firmware\":\"" FIRMWARE_VERSION "\"";
    j += ",\"mac\":\"" + String(macText) + "\"";
    j += ",\"reset\":\"" + String(dashResetReasonName(esp_reset_reason())) + "\"";
    j += ",\"uptime\":" + String((millis() - startMs) / 1000);
    j += ",\"tasks\":" + String(uxTaskGetNumberOfTasks());
    j += ",\"core\":" + String(xPortGetCoreID());
    j += ",\"heap_total\":" + String(heap_caps_get_total_size(MALLOC_CAP_8BIT));
    j += ",\"heap_free\":" + String(heap_caps_get_free_size(MALLOC_CAP_8BIT));
    j += ",\"heap_min\":" + String(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
    j += ",\"heap_largest\":" + String(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    j += ",\"internal_free\":" + String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    j += ",\"psram_total\":" + String(heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    j += ",\"psram_free\":" + String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    j += ",\"flash_size\":" + String(flashSize);
    j += ",\"flash_speed\":" + String(80000000UL);
    j += ",\"app_addr\":" + String(running ? running->address : 0);
    j += ",\"app_size\":" + String(running ? running->size : 0);
    j += ",\"app_used\":" + String(appUsed);
    j += ",\"app_label\":\"" + String(running ? running->label : "") + "\"";
    j += ",\"spiffs_ok\":" + String(spiffsOk ? "true" : "false");
    j += ",\"spiffs_total\":" + String(spiffsTotal);
    j += ",\"spiffs_used\":" + String(spiffsUsed);
    j += ",\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
    j += ",\"wifi_mode\":\"" + String(wifiModeText) + "\"";
    j += ",\"wifi_sleep\":" + String(wifiPs == WIFI_PS_NONE ? "false" : "true");
    j += ",\"wifi_standard\":\"2.4GHz 802.11 b/g/n\"";
    j += ",\"wifi_max_mbps\":150";
    j += ",\"ble_supported\":" + String((chip.features & CHIP_FEATURE_BLE) ? "true" : "false");
#ifdef CONFIG_BT_ENABLED
    j += ",\"ble_enabled\":true";
    j += ",\"ble_status\":\"compiled\"";
#else
    j += ",\"ble_enabled\":false";
    j += ",\"ble_status\":\"firmware disabled\"";
#endif
    j += ",\"wifi_rssi\":";
    j += hasRssi ? String(rssi) : String("null");
    j += ",\"ap_clients\":" + String(WiFi.softAPgetStationNum());
    j += ",\"temp_c\":";
    if (hasTemp)
    {
        int tempX10 = (int)(tempC * 10.0f + (tempC >= 0 ? 0.5f : -0.5f));
        j += String(tempX10 / 10);
        j += ".";
        j += String(abs(tempX10 % 10));
    }
    else
    {
        j += "null";
    }
    j += "}";
    server.send(200, "application/json", j);
#else
    server.send(200, "application/json", "{\"chip\":\"native\",\"cores\":1}");
#endif
}

#ifdef ESP_PLATFORM
static const char *dashTaskStateName(eTaskState state)
{
    switch (state)
    {
    case eRunning:
        return "RUN";
    case eReady:
        return "READY";
    case eBlocked:
        return "BLOCK";
    case eSuspended:
        return "SUSP";
    case eDeleted:
        return "DEL";
    default:
        return "UNK";
    }
}

static String dashTaskCoreName(BaseType_t core)
{
    if (core == tskNO_AFFINITY)
        return "any";
    return String((int)core);
}

static BaseType_t dashTaskCore(TaskHandle_t handle)
{
    if (!handle)
        return tskNO_AFFINITY;
    return xTaskGetCoreID(handle);
}

#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
static constexpr UBaseType_t kMaxTasksForStats = 64;

struct DashTaskSample
{
    const TaskStatus_t *task;
    uint32_t delta;
};

static String dashBuildTaskStatsText(const TaskStatus_t *before,
                                     UBaseType_t beforeCount,
                                     const TaskStatus_t *after,
                                     UBaseType_t afterCount,
                                     uint32_t elapsedMs)
{
    DashTaskSample samples[kMaxTasksForStats];
    UBaseType_t sampleCount = 0;
    uint64_t totalDelta = 0;
    uint32_t idleDelta[2] = {0, 0};

    for (UBaseType_t i = 0; i < afterCount && i < kMaxTasksForStats; i++)
    {
        uint32_t prevCounter = static_cast<uint32_t>(after[i].ulRunTimeCounter);
        bool found = false;
        for (UBaseType_t j = 0; j < beforeCount && j < kMaxTasksForStats; j++)
        {
            if (before[j].xHandle == after[i].xHandle)
            {
                prevCounter = static_cast<uint32_t>(before[j].ulRunTimeCounter);
                found = true;
                break;
            }
        }
        uint32_t nowCounter = static_cast<uint32_t>(after[i].ulRunTimeCounter);
        uint32_t delta = found ? (nowCounter - prevCounter) : 0;
        samples[sampleCount++] = {&after[i], delta};
        totalDelta += delta;

        const char *name = after[i].pcTaskName ? after[i].pcTaskName : "";
        BaseType_t core = dashTaskCore(after[i].xHandle);
        if (strncmp(name, "IDLE", 4) == 0 && core >= 0 && core <= 1)
            idleDelta[core] = delta;
    }

    for (UBaseType_t i = 0; i < sampleCount; i++)
    {
        for (UBaseType_t j = i + 1; j < sampleCount; j++)
        {
            if (samples[j].delta > samples[i].delta)
            {
                DashTaskSample tmp = samples[i];
                samples[i] = samples[j];
                samples[j] = tmp;
            }
        }
    }

    auto pct = [totalDelta](uint32_t delta) -> String {
        if (totalDelta == 0)
            return "0.0";
        uint32_t tenths = static_cast<uint32_t>((static_cast<uint64_t>(delta) * 1000ULL + totalDelta / 2) / totalDelta);
        return String(tenths / 10) + "." + String(tenths % 10);
    };
    auto loadFromIdle = [totalDelta](uint32_t idle) -> String {
        if (totalDelta == 0)
            return "n/a";
        uint32_t perCoreTotal = static_cast<uint32_t>(totalDelta / 2);
        if (perCoreTotal == 0)
            return "n/a";
        uint32_t idlePct = static_cast<uint32_t>((static_cast<uint64_t>(idle) * 100ULL + perCoreTotal / 2) / perCoreTotal);
        if (idlePct > 100)
            idlePct = 100;
        return String(100 - idlePct);
    };

    String out;
    out.reserve(4096);
    out += "ev-open-can-tools task stats\n";
    out += "sample_ms: " + String(elapsedMs) + "\n";
    out += "tasks: " + String(afterCount) + "\n";
    out += "cpu0_load_from_idle: " + loadFromIdle(idleDelta[0]) + "%\n";
    out += "cpu1_load_from_idle: " + loadFromIdle(idleDelta[1]) + "%\n";
    out += "heap_free: " + String(heap_caps_get_free_size(MALLOC_CAP_8BIT)) + " bytes\n";
    out += "heap_largest: " + String(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)) + " bytes\n\n";
    out += "CPU%   core prio stack state  task\n";
    out += "-----  ---- ---- ----- ------ ----------------\n";

    for (UBaseType_t i = 0; i < sampleCount; i++)
    {
        const TaskStatus_t &t = *samples[i].task;
        String p = pct(samples[i].delta);
        while (p.length() < 5)
            p = " " + p;
        String core = dashTaskCoreName(dashTaskCore(t.xHandle));
        while (core.length() < 4)
            core = " " + core;
        String prio = String((unsigned)t.uxCurrentPriority);
        while (prio.length() < 4)
            prio = " " + prio;
        String stack = String((unsigned)t.usStackHighWaterMark);
        while (stack.length() < 5)
            stack = " " + stack;

        out += p + "  " + core + " " + prio + " " + stack + " ";
        out += dashTaskStateName(t.eCurrentState);
        out += "  ";
        out += t.pcTaskName ? t.pcTaskName : "?";
        out += "\n";
    }

    return out;
}

static String dashBuildTaskStatsTextBlocking()
{
    TaskStatus_t before[kMaxTasksForStats];
    TaskStatus_t after[kMaxTasksForStats];
    configRUN_TIME_COUNTER_TYPE totalBefore = 0;
    configRUN_TIME_COUNTER_TYPE totalAfter = 0;

    UBaseType_t beforeCount = uxTaskGetSystemState(before, kMaxTasksForStats, &totalBefore);
    int64_t startUs = esp_timer_get_time();
    vTaskDelay(pdMS_TO_TICKS(1000));
    int64_t endUs = esp_timer_get_time();
    UBaseType_t afterCount = uxTaskGetSystemState(after, kMaxTasksForStats, &totalAfter);
    return dashBuildTaskStatsText(before, beforeCount, after, afterCount,
                                  static_cast<uint32_t>((endUs - startUs) / 1000));
}
#endif

static void handleTaskStats()
{
#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
    server.send(200, "text/plain; charset=utf-8", dashBuildTaskStatsTextBlocking());
#else
    server.send(200, "text/plain; charset=utf-8", "FreeRTOS runtime stats are not enabled.\n");
#endif
}
#else
static void handleTaskStats()
{
    server.send(200, "text/plain; charset=utf-8", "Task stats are only available on ESP-IDF builds.\n");
}
#endif

#ifdef ESP_PLATFORM
static void dashSerialPrintHelp()
{
    Serial.println();
    Serial.println("ev-open-can-tools serial diagnostics");
    Serial.println("Commands:");
    Serial.println("  help           show this help");
    Serial.println("  system_status  print CPU/heap/WiFi summary");
    Serial.println("  can_status     print CAN/injection summary");
    Serial.println("  task_stats     sample FreeRTOS tasks for 1s asynchronously");
    Serial.println();
}

static void dashSerialPrintSystemStatus()
{
    uint8_t cpu0Load = 0, cpu1Load = 0;
    bool hasCpuLoad = false;
    dashReadCpuLoad(cpu0Load, cpu1Load, hasCpuLoad);

    float tempC = 0.0f;
    bool hasTemp = dashReadTemperature(tempC);
    uint32_t cpuMhz = (static_cast<uint32_t>(esp_clk_cpu_freq()) + 500000UL) / 1000000UL;
    wifi_mode_t wifiMode = WIFI_MODE_NULL;
    esp_wifi_get_mode(&wifiMode);

    const char *wifiModeText = "off";
    switch (wifiMode)
    {
    case WIFI_MODE_STA:
        wifiModeText = "STA";
        break;
    case WIFI_MODE_AP:
        wifiModeText = "AP";
        break;
    case WIFI_MODE_APSTA:
        wifiModeText = "AP+STA";
        break;
    default:
        break;
    }

    wifi_ap_record_t apInfo;
    bool hasRssi = esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK;

    Serial.println();
    Serial.println("[system_status]");
    Serial.printf("uptime=%lus firmware=%s idf=%s\n", (millis() - startMs) / 1000, FIRMWARE_VERSION, IDF_VER);
    Serial.printf("cpu=%luMHz load=", (unsigned long)cpuMhz);
    if (hasCpuLoad)
        Serial.printf("CPU0 %u%% CPU1 %u%%\n", cpu0Load, cpu1Load);
    else
        Serial.println("sampling");
    Serial.printf("heap_free=%u heap_largest=%u heap_min=%u tasks=%u\n",
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                  (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
                  (unsigned)uxTaskGetNumberOfTasks());
    Serial.printf("wifi=%s connected=%s ap_clients=%u",
                  wifiModeText,
                  WiFi.status() == WL_CONNECTED ? "yes" : "no",
                  (unsigned)WiFi.softAPgetStationNum());
    if (hasRssi)
        Serial.printf(" rssi=%d", apInfo.rssi);
    Serial.println();
    if (hasTemp)
        Serial.printf("temp=%.1fC\n", tempC);
    Serial.println();
}

static void dashSerialPrintCanStatus()
{
    unsigned long fpsX10 = static_cast<unsigned long>(fps * 10.0f + 0.5f);
    bool apActive = dashHandler ? (bool)dashHandler->APActive : false;
    bool adEnabled = dashHandler ? (bool)dashHandler->ADEnabled : false;
    int sp = dashHandler ? (int)dashHandler->speedProfile : 0;
    bool spAuto = dashHandler ? (bool)dashHandler->speedProfileAuto : true;
    int gtwAp = dashHandler ? (int)dashHandler->gatewayAutopilot : -1;
    Serial.println();
    Serial.println("[can_status]");
    Serial.printf("can=%s fsd_switch=%s injection_active=%s hw=%u profile=%s/%d\n",
                  canOnline ? "online" : "offline",
                  canActive ? "ON" : "OFF",
                  dashInjectionActive() ? "ON" : "OFF",
                  (unsigned)hwMode,
                  spAuto ? "auto" : "manual",
                  sp);
    Serial.printf("rx=%lu tx=%lu txerr=%lu fps=%lu.%lu follow_dist=%u eflg=0x%02X\n",
                  rxCount, txCount, txErrCount, fpsX10 / 10, fpsX10 % 10,
                  (unsigned)followDist, (unsigned)mcpEflg);
    Serial.printf("APActive=%s ADEnabled=%s GTW_autopilot=%d\n",
                  apActive ? "yes" : "no",
                  adEnabled ? "yes" : "no",
                  gtwAp);    Serial.println();
}

#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
static TaskStatus_t dashSerialTaskBefore[kMaxTasksForStats];
static UBaseType_t dashSerialTaskBeforeCount = 0;
static uint32_t dashSerialTaskStartMs = 0;
static bool dashSerialTaskSampling = false;

static void dashSerialStartTaskStats()
{
    configRUN_TIME_COUNTER_TYPE totalBefore = 0;
    dashSerialTaskBeforeCount = uxTaskGetSystemState(dashSerialTaskBefore, kMaxTasksForStats, &totalBefore);
    dashSerialTaskStartMs = millis();
    dashSerialTaskSampling = true;
    Serial.println("[task_stats] sampling 1000 ms...");
}

static void dashSerialTaskStatsTick()
{
    if (!dashSerialTaskSampling || millis() - dashSerialTaskStartMs < 1000)
        return;

    TaskStatus_t after[kMaxTasksForStats];
    configRUN_TIME_COUNTER_TYPE totalAfter = 0;
    UBaseType_t afterCount = uxTaskGetSystemState(after, kMaxTasksForStats, &totalAfter);
    uint32_t elapsedMs = millis() - dashSerialTaskStartMs;
    dashSerialTaskSampling = false;
    Serial.print(dashBuildTaskStatsText(dashSerialTaskBefore, dashSerialTaskBeforeCount, after, afterCount, elapsedMs));
}
#else
static void dashSerialStartTaskStats()
{
    Serial.println("FreeRTOS runtime stats are not enabled.");
}

static void dashSerialTaskStatsTick() {}
#endif

static void dashSerialRunCommand(char *cmd)
{
    char *start = cmd;
    while (*start == ' ' || *start == '\t')
        start++;
    char *end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t'))
        *--end = '\0';
    for (char *p = start; *p; p++)
    {
        if (*p >= 'A' && *p <= 'Z')
            *p = *p - 'A' + 'a';
    }

    if (strcmp(start, "help") == 0 || strcmp(start, "?") == 0)
        dashSerialPrintHelp();
    else if (strcmp(start, "system_status") == 0 || strcmp(start, "sys") == 0)
        dashSerialPrintSystemStatus();
    else if (strcmp(start, "can_status") == 0 || strcmp(start, "can") == 0)
        dashSerialPrintCanStatus();
    else if (strcmp(start, "task_stats") == 0 || strcmp(start, "tasks") == 0)
        dashSerialStartTaskStats();
    else if (*start)
        Serial.println("Unknown command. Type help.");
}

static void dashSerialDiagnosticsPoll()
{
    static char cmd[96];
    static uint8_t len = 0;
    static bool announced = false;

    if (!announced && millis() > 3000)
    {
        announced = true;
        Serial.println("[DIAG] Serial commands ready. Type help.");
    }

    dashSerialTaskStatsTick();

    int budget = 24;
    while (budget-- > 0 && Serial.available() > 0)
    {
        int ch = Serial.read();
        if (ch < 0)
            break;
        if (ch == '\r' || ch == '\n')
        {
            if (len > 0)
            {
                cmd[len] = '\0';
                dashSerialRunCommand(cmd);
                len = 0;
            }
            continue;
        }
        if (ch == 8 || ch == 127)
        {
            if (len > 0)
                len--;
            continue;
        }
        if (ch < 32 || ch > 126)
            continue;
        if (len < sizeof(cmd) - 1)
            cmd[len++] = static_cast<char>(ch);
    }
}
#else
static void dashSerialDiagnosticsPoll() {}
#endif

static bool dashCanGpioReserved(int pin)
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    if (pin >= 26 && pin <= 32)
        return true; // embedded flash/PSRAM bus on ESP32-S3 modules
    if (pin == 45 || pin == 46)
        return true; // strapping/input-only pins
#elif defined(CONFIG_IDF_TARGET_ESP32)
#ifndef DASH_ALLOW_CAN_GPIO_6_11
#define DASH_ALLOW_CAN_GPIO_6_11 0
#endif
#if !DASH_ALLOW_CAN_GPIO_6_11
    if (pin >= 6 && pin <= 11)
        return true; // SPI flash pins on common ESP32 modules
#endif
#endif
    return false;
}

static bool dashCanGpioValid(int pin, bool tx)
{
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    constexpr int kMaxGpio = 48;
#else
    constexpr int kMaxGpio = 39;
#endif
    if (pin < 0 || pin > kMaxGpio || dashCanGpioReserved(pin))
        return false;
#if defined(CONFIG_IDF_TARGET_ESP32)
    if (tx && pin >= 34 && pin <= 39)
        return false; // input-only pins cannot drive TWAI TX
#else
    (void)tx;
#endif
    return true;
}

static void handleCanPins()
{
    Preferences canPrefs;
    bool customized = false;
    int tx = -1, rx = -1;
#if defined(DRIVER_TWAI)
    tx = (int)TWAI_TX_PIN;
    rx = (int)TWAI_RX_PIN;
#endif
    if (canPrefs.begin("can", false))
    {
        int storedTx = canPrefs.getChar("tx", -1);
        int storedRx = canPrefs.getChar("rx", -1);
        canPrefs.end();
        if (dashCanGpioValid(storedTx, true) && dashCanGpioValid(storedRx, false) && storedTx != storedRx)
        {
            tx = storedTx;
            rx = storedRx;
            customized = true;
        }
    }
    String j = "{\"tx\":" + String(tx);
    j += ",\"rx\":" + String(rx);
    j += ",\"customized\":" + String(customized ? "true" : "false");
    j += "}";
    server.send(200, "application/json", j);
}

static void handleCanPinsSave()
{
    int tx = server.arg("tx").toInt();
    int rx = server.arg("rx").toInt();

    if (!dashCanGpioValid(tx, true) || !dashCanGpioValid(rx, false))
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid or reserved GPIO for CAN\"}");
        return;
    }
    if (tx == rx)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"TX and RX must differ\"}");
        return;
    }

    Preferences canPrefs;
    if (!canPrefs.begin("can", false))
    {
        server.send(500, "application/json", "{\"ok\":false,\"error\":\"NVS open failed\"}");
        return;
    }
    canPrefs.putChar("tx", (int8_t)tx);
    canPrefs.putChar("rx", (int8_t)rx);
    canPrefs.end();

    dashLog("[CAN] Pins saved: TX=" + String(tx) + " RX=" + String(rx) + " (reboot required)");
    server.send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
}

// ── Settings Backup / Restore ───────────────────────────────────

static void handleSettingsExport()
{
    Preferences p;
    String apSsid = "", apPass = "", wSsid = "", wPass = "";
    String wIp = "", wGw = "", wMask = "", wDns = "";
    bool wStatic = false, beta = false, autoUpdate = false, apHid = false;
    bool h3Slew = false, eprn = true;
    uint8_t h3SlewRate = kHw3SlewRateDefault;
    uint8_t storedHw = hwMode;
    bool storedCan = canActive;
    bool spAuto = dashSpeedProfileAuto;
    uint8_t spSel = dashManualSpeedProfile;
    bool h3Custom = hw3CustomSpeed;
    bool h3HighSpeed = hw3HighSpeedEnable;
    uint8_t h3Enc = hw3WireEncoding;
    uint8_t h3CustomTargets[kHw3CustomTargetCount];
    uint8_t h3HighSpeedTargets[kHw3HighSpeedBucketCount];
    int canTx = -1, canRx = -1;

    for (uint8_t i = 0; i < kHw3CustomTargetCount; i++)
        h3CustomTargets[i] = hw3CustomTarget[i];
    for (uint8_t i = 0; i < kHw3HighSpeedBucketCount; i++)
        h3HighSpeedTargets[i] = hw3HighSpeedTarget[i];

    if (p.begin(PREFS_NS, false))
    {
        storedHw = p.getUChar("hw", hwMode);
        storedCan = p.getBool("can", canActive);
        spAuto = p.getBool("sp_auto", dashSpeedProfileAuto);
        spSel = p.getUChar("sp_sel", dashManualSpeedProfile);
        eprn = p.getBool("eprn", true);
        if (p.isKey("ap_ssid"))
            apSsid = p.getString("ap_ssid", "");
        if (p.isKey("ap_pass"))
            apPass = p.getString("ap_pass", "");
        apHid = p.getBool("ap_hidden", false);
        if (p.isKey("wifi_ssid"))
            wSsid = p.getString("wifi_ssid", "");
        if (p.isKey("wifi_pass"))
            wPass = p.getString("wifi_pass", "");
        wStatic = p.getBool("wifi_static", false);
        if (p.isKey("wifi_ip"))
            wIp = p.getString("wifi_ip", "");
        if (p.isKey("wifi_gw"))
            wGw = p.getString("wifi_gw", "");
        if (p.isKey("wifi_mask"))
            wMask = p.getString("wifi_mask", "");
        if (p.isKey("wifi_dns"))
            wDns = p.getString("wifi_dns", "");
        beta = p.getBool("update_beta", p.getBool("upd_beta", false));
        autoUpdate = p.getBool("auto_upd", false);
        h3Slew = p.getBool("h3_slw", false);
        h3SlewRate = dashLoadHw3SlewRate(p.getUChar("h3_srt", kHw3SlewRateDefault));
        h3Custom = p.getBool("h3_cust", hw3CustomSpeed);
        h3HighSpeed = p.getBool("h3_hse", hw3HighSpeedEnable);
        h3Enc = p.getUChar("h3_enc", hw3WireEncoding);
        char k[8];
        for (uint8_t i = 0; i < kHw3CustomTargetCount; i++)
        {
            snprintf(k, sizeof(k), "h3_ct%u", (unsigned)i);
            h3CustomTargets[i] = p.getUChar(k, h3CustomTargets[i]);
        }
        for (uint8_t i = 0; i < kHw3HighSpeedBucketCount; i++)
        {
            snprintf(k, sizeof(k), "h3_ht%u", (unsigned)i);
            h3HighSpeedTargets[i] = p.getUChar(k, h3HighSpeedTargets[i]);
        }
        p.end();
    }
    Preferences cp;
    if (cp.begin("can", true))
    {
        canTx = cp.getChar("tx", -1);
        canRx = cp.getChar("rx", -1);
        cp.end();
    }

    String j = "{\"version\":\"" FIRMWARE_VERSION "\"";
    j += ",\"device\":{\"hw\":" + String(storedHw) + ",\"can\":" + String(storedCan ? "true" : "false");
    j += ",\"speedProfileAuto\":" + String(spAuto ? "true" : "false") + ",\"speedProfile\":" + String(spSel);
    j += ",\"dashboardLog\":" + String(eprn ? "true" : "false") + "}";
    j += ",\"ap\":{\"ssid\":\"" + jsonEscape(apSsid) + "\",\"pass\":\"" + jsonEscape(apPass) + "\",\"hidden\":" + String(apHid ? "true" : "false") + "}";
    j += ",\"wifi\":{\"ssid\":\"" + jsonEscape(wSsid) + "\",\"pass\":\"" + jsonEscape(wPass) + "\"";
    j += ",\"static\":" + String(wStatic ? "true" : "false");
    j += ",\"ip\":\"" + jsonEscape(wIp) + "\",\"gw\":\"" + jsonEscape(wGw) + "\"";
    j += ",\"mask\":\"" + jsonEscape(wMask) + "\",\"dns\":\"" + jsonEscape(wDns) + "\"}";
    j += ",\"wifiNetworks\":[";
    for (uint8_t i = 0; i < wifiNetworkCount; i++)
    {
        if (i)
            j += ",";
        const DashWifiNetwork &n = wifiNetworks[i];
        j += "{\"ssid\":\"" + jsonEscape(n.ssid) + "\",\"pass\":\"" + jsonEscape(n.pass) + "\"";
        j += ",\"static\":" + String(n.useStatic ? "true" : "false");
        j += ",\"ip\":\"" + jsonEscape(n.ip) + "\",\"gw\":\"" + jsonEscape(n.gw) + "\"";
        j += ",\"mask\":\"" + jsonEscape(n.mask) + "\",\"dns\":\"" + jsonEscape(n.dns) + "\"}";
    }
    j += "]";
    j += ",\"wifiPreferred\":" + String(wifiActiveSlot >= 0 ? wifiActiveSlot : 0);
    j += ",\"hw3\":{\"offsetSlew\":" + String(h3Slew ? "true" : "false") + ",\"slewRate\":" + String(h3SlewRate);
    j += ",\"custom\":" + String(h3Custom ? "true" : "false");
    j += ",\"highSpeed\":" + String(h3HighSpeed ? "true" : "false") + ",\"encoding\":" + String(h3Enc);
    j += ",\"customTargets\":[";
    for (uint8_t i = 0; i < kHw3CustomTargetCount; i++)
    {
        if (i)
            j += ",";
        j += String(h3CustomTargets[i]);
    }
    j += "],\"highSpeedTargets\":[";
    for (uint8_t i = 0; i < kHw3HighSpeedBucketCount; i++)
    {
        if (i)
            j += ",";
        j += String(h3HighSpeedTargets[i]);
    }
    j += "]}";
    j += ",\"can\":{\"tx\":" + String(canTx) + ",\"rx\":" + String(canRx) + "}";
    j += ",\"updates\":{\"beta\":" + String(beta ? "true" : "false") + ",\"auto\":" + String(autoUpdate ? "true" : "false") + "}";
    j += ",\"beta\":" + String(beta ? "true" : "false");
#if defined(ESP_PLATFORM) && defined(DASH_STA_AP_GATEWAY)
    j += ",\"gateway\":{\"enabled\":" + String(gatewayEnabled ? "true" : "false");
    j += ",\"blacklist\":\"" + jsonEscape(gatewayDnsBlacklist.c_str()) + "\"";
    j += ",\"whitelist\":\"" + jsonEscape(gatewayDnsWhitelist.c_str()) + "\"}";
#endif
    j += "}";

    server.sendHeader("Content-Disposition", "attachment; filename=\"evtools-backup.json\"");
    server.send(200, "application/json", j);
}

static void handleSettingsImport()
{
    String body = server.arg("plain");
    if (body.length() == 0)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Empty body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    Preferences p;
    if (!p.begin(PREFS_NS, false))
    {
        server.send(500, "application/json", "{\"ok\":false,\"error\":\"NVS open failed\"}");
        return;
    }

    if (doc["device"].is<JsonObject>())
    {
        if (doc["device"]["hw"].is<int>())
        {
            int hw = doc["device"]["hw"].as<int>();
            if (hw >= 0 && hw <= 2)
                p.putUChar("hw", static_cast<uint8_t>(hw));
        }
        if (doc["device"]["can"].is<bool>())
        {
            bool fsdSwitch = doc["device"]["can"].as<bool>();
            p.putBool("can", fsdSwitch);
            p.putBool("force_act", fsdSwitch);
        }
        if (doc["device"]["speedProfileAuto"].is<bool>())
            p.putBool("sp_auto", doc["device"]["speedProfileAuto"].as<bool>());
        if (doc["device"]["speedProfile"].is<int>())
        {
            uint8_t targetHw = p.getUChar("hw", hwMode);
            p.putUChar("sp_sel", dashClampSpeedProfileForHw(targetHw, doc["device"]["speedProfile"].as<int>()));
        }
        if (doc["device"]["dashboardLog"].is<bool>())
            p.putBool("eprn", doc["device"]["dashboardLog"].as<bool>());
    }
    if (doc["ap"].is<JsonObject>())
    {
        const char *s = doc["ap"]["ssid"] | "";
        const char *pw = doc["ap"]["pass"] | "";
        size_t ssidLen = strlen(s);
        size_t passLen = strlen(pw);
        if (ssidLen > 0 && ssidLen <= kDashMaxSsidLen)
            p.putString("ap_ssid", s);
        if (dashApPasswordLengthValid(passLen))
            p.putString("ap_pass", pw);
        if (doc["ap"]["hidden"].is<bool>())
            p.putBool("ap_hidden", doc["ap"]["hidden"].as<bool>());
    }
    if (doc["wifi"].is<JsonObject>())
    {
        const char *s = doc["wifi"]["ssid"] | "";
        const char *pw = doc["wifi"]["pass"] | "";
        if (strlen(s) <= kDashMaxSsidLen && strlen(pw) <= kDashMaxPassLen)
        {
            p.putString("wifi_ssid", s);
            p.putString("wifi_pass", pw);
        }
        p.putBool("wifi_static", doc["wifi"]["static"] | false);
        p.putString("wifi_ip", (const char *)(doc["wifi"]["ip"] | ""));
        p.putString("wifi_gw", (const char *)(doc["wifi"]["gw"] | ""));
        p.putString("wifi_mask", (const char *)(doc["wifi"]["mask"] | ""));
        p.putString("wifi_dns", (const char *)(doc["wifi"]["dns"] | ""));
    }
    if (doc["wifiNetworks"].is<JsonArray>())
    {
        JsonArray nets = doc["wifiNetworks"].as<JsonArray>();
        uint8_t count = 0;
        for (uint8_t i = 0; i < kDashMaxWifiNetworks; i++)
            dashRemoveWifiSlotKeys(i);
        for (JsonVariant v : nets)
        {
            if (count >= kDashMaxWifiNetworks || !v.is<JsonObject>())
                break;
            const char *s = v["ssid"] | "";
            const char *pw = v["pass"] | "";
            if (strlen(s) == 0 || !dashStaConfigLengthValid(String(s), String(pw)) || dashStaSsidLooksCorrupt(String(s)))
                continue;
            p.putString(dashWifiKey(count, "s").c_str(), s);
            p.putString(dashWifiKey(count, "p").c_str(), pw);
            bool st = v["static"] | false;
            p.putBool(dashWifiKey(count, "t").c_str(), st);
            if (st)
            {
                p.putString(dashWifiKey(count, "i").c_str(), (const char *)(v["ip"] | "0.0.0.0"));
                p.putString(dashWifiKey(count, "g").c_str(), (const char *)(v["gw"] | "0.0.0.0"));
                p.putString(dashWifiKey(count, "m").c_str(), (const char *)(v["mask"] | "255.255.255.0"));
                p.putString(dashWifiKey(count, "d").c_str(), (const char *)(v["dns"] | "0.0.0.0"));
            }
            count++;
        }
        p.putUChar("wn_cnt", count);
        uint8_t pref = doc["wifiPreferred"] | 0;
        p.putUChar("wn_pref", count > 0 && pref < count ? pref : 0);
    }
    if (doc["updates"].is<JsonObject>())
    {
        if (doc["updates"]["beta"].is<bool>())
        {
            p.putBool("update_beta", doc["updates"]["beta"].as<bool>());
            p.putBool("upd_beta", doc["updates"]["beta"].as<bool>());
        }
        if (doc["updates"]["auto"].is<bool>())
            p.putBool("auto_upd", doc["updates"]["auto"].as<bool>());
    }
    else if (doc["beta"].is<bool>())
    {
        p.putBool("update_beta", doc["beta"].as<bool>());
        p.putBool("upd_beta", doc["beta"].as<bool>());
    }    if (doc["hw3"].is<JsonObject>())
    {
        if (doc["hw3"]["offsetSlew"].is<bool>())
            p.putBool("h3_slw", doc["hw3"]["offsetSlew"].as<bool>());
        if (doc["hw3"]["slewRate"].is<int>())
            p.putUChar("h3_srt", dashClampHw3SlewRate(doc["hw3"]["slewRate"].as<int>()));
        if (doc["hw3"]["custom"].is<bool>())
            p.putBool("h3_cust", doc["hw3"]["custom"].as<bool>());
        if (doc["hw3"]["highSpeed"].is<bool>())
            p.putBool("h3_hse", doc["hw3"]["highSpeed"].as<bool>());
        if (doc["hw3"]["encoding"].is<int>())
        {
            uint8_t enc = doc["hw3"]["encoding"].as<int>() == kHw3WireEncKph5 ? kHw3WireEncKph5 : kHw3WireEncPct4;
            p.putUChar("h3_enc", enc);
        }
        if (doc["hw3"]["customTargets"].is<JsonArray>())
        {
            JsonArray arr = doc["hw3"]["customTargets"].as<JsonArray>();
            char k[8];
            for (uint8_t i = 0; i < kHw3HighSpeedBucketCount && i < arr.size(); i++)
            {
                snprintf(k, sizeof(k), "h3_ct%u", (unsigned)i);
                p.putUChar(k, dashClampHw3CustomTargetForBucket(i, arr[i].as<int>()));
            }
        }
        if (doc["hw3"]["highSpeedTargets"].is<JsonArray>())
        {
            JsonArray arr = doc["hw3"]["highSpeedTargets"].as<JsonArray>();
            char k[8];
            for (uint8_t i = 0; i < 5 && i < arr.size(); i++)
            {
                snprintf(k, sizeof(k), "h3_ht%u", (unsigned)i);
                p.putUChar(k, dashClampHw3HighSpeedTargetForBucket(i, arr[i].as<int>()));
            }
        }
    }
    p.end();

    if (doc["can"].is<JsonObject>())
    {
        int tx = doc["can"]["tx"] | -1;
        int rx = doc["can"]["rx"] | -1;
        Preferences cp;
        if (cp.begin("can", false))
        {
            if (tx >= 0 && tx <= 39 && rx >= 0 && rx <= 39 && tx != rx &&
                !((tx >= 6 && tx <= 11) || (rx >= 6 && rx <= 11)))
            {
                cp.putChar("tx", (int8_t)tx);
                cp.putChar("rx", (int8_t)rx);
            }
            cp.end();
        }
    }

#if defined(ESP_PLATFORM) && defined(DASH_STA_AP_GATEWAY)
    if (doc["gateway"].is<JsonObject>())
    {
        JsonObject gw = doc["gateway"].as<JsonObject>();
        if (gw["enabled"].is<bool>())
            gatewayEnabled = gw["enabled"].as<bool>();
        if (gw["blacklist"].is<const char *>())
            gatewayDnsBlacklist = dashGatewaySanitizeBlacklist((const char *)(gw["blacklist"] | ""));
        if (gw["whitelist"].is<const char *>())
            gatewayDnsWhitelist = dashGatewaySanitizeWhitelist((const char *)(gw["whitelist"] | ""));
        dashGatewaySave();
    }
#endif

    dashLog("[BACKUP] Settings imported (reboot required)");
    server.send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
}

static void handleApConfig()
{
    String newSsid = server.arg("ssid");
    String newPass = server.arg("pass");
    bool hasHidden = server.hasArg("hidden");
    bool newHidden = hasHidden && (server.arg("hidden") == "1" || server.arg("hidden") == "true");

    if (newSsid.length() == 0)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"SSID required\"}");
        return;
    }
    if (newSsid.length() > kDashMaxSsidLen)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"SSID must be 32 bytes or less\"}");
        return;
    }
    if (newPass.length() > 0 && !dashApPasswordLengthValid(newPass.length()))
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Password must be 8-64 characters\"}");
        return;
    }

    strlcpy(apSSID, newSsid.c_str(), sizeof(apSSID));
    if (newPass.length() > 0)
        strlcpy(apPass, newPass.c_str(), sizeof(apPass));
    if (hasHidden)
        apHidden = newHidden;

    prefs.begin(PREFS_NS, false);
    prefs.putString("ap_ssid", newSsid);
    if (newPass.length() > 0)
        prefs.putString("ap_pass", newPass);
    if (hasHidden)
        prefs.putBool("ap_hidden", newHidden);
    prefs.end();

    dashLog("[WIFI] AP config updated: SSID=" + newSsid + (apHidden ? " (hidden)" : "") +
            " channel=auto match STA");
    server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Saved. AP starts on CH1 and auto matches STA after WiFi connects.\"}");
}

static void handleApStatus()
{
    Preferences p;
    bool stored = false;
    if (p.begin(PREFS_NS, false))
    {
        stored = p.isKey("ap_ssid") && p.getString("ap_ssid", "").length() > 0;
        p.end();
    }
    String j = "{\"ssid\":\"" + jsonEscape(apSSID) + "\"";
    j += ",\"ip\":\"" + WiFi.softAPIP().toString() + "\"";
    j += ",\"clients\":" + String(WiFi.softAPgetStationNum());
    j += ",\"channel\":" + String(dashCurrentApChannel());
    j += ",\"channel_auto\":true";
    j += ",\"last_channel_sync_ms\":" + String(apLastChannelSyncMs);
    j += ",\"last_channel_sync_target\":" + String(apLastChannelSyncTarget);
    j += ",\"last_channel_sync_ok\":" + String(apLastChannelSyncOk ? "true" : "false");
    j += ",\"stored\":" + String(stored ? "true" : "false");
    j += ",\"hidden\":" + String(apHidden ? "true" : "false");
    j += "}";
    server.send(200, "application/json", j);
}

// ── OTA GitHub Update ───────────────────────────────────────────

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "unknown"
#endif

static const char *GITHUB_REPO = "ev-open-can-tools/ev-open-can-tools";

// Map driver type to release artifact filename
static const char *getFirmwareArtifact()
{
#if defined(DRIVER_ESP32_EXT_MCP2515)
    return "firmware-esp32-ext-mcp2515.bin";
#else
    return "firmware-esp32.bin";
#endif
}

// Parse a semver-ish version string into (major, minor, patch, preRank, preNum).
// Pre-release rank: 0 = stable (no suffix, sorts highest among same M.m.p),
//                  1 = -alpha.N, 2 = -beta.N, 3 = -rc.N (higher rank = closer to stable).
// Unknown suffix → treated as stable (rank 0).
static void parseVersion(const String &v, int &maj, int &min, int &pat, int &preRank, int &preNum)
{
    maj = min = pat = 0;
    preRank = 0;
    preNum = 0;
    int i = 0;
    int len = v.length();
    auto readInt = [&](int &out)
    {
        int val = 0;
        bool any = false;
        while (i < len && v[i] >= '0' && v[i] <= '9')
        {
            val = val * 10 + (v[i] - '0');
            i++;
            any = true;
        }
        if (any)
            out = val;
    };
    readInt(maj);
    if (i < len && v[i] == '.')
    {
        i++;
        readInt(min);
    }
    if (i < len && v[i] == '.')
    {
        i++;
        readInt(pat);
    }
    if (i < len && v[i] == '-')
    {
        i++;
        String tail = v.substring(i);
        tail.toLowerCase();
        if (tail.startsWith("alpha"))
            preRank = 1;
        else if (tail.startsWith("beta"))
            preRank = 2;
        else if (tail.startsWith("rc"))
            preRank = 3;
        else
            preRank = 0; // unknown → treat as stable
        int dot = tail.indexOf('.');
        if (dot >= 0)
            preNum = tail.substring(dot + 1).toInt();
    }
}

// Returns true iff `candidate` is strictly newer than `current`.
static bool isVersionNewer(const String &candidate, const String &current)
{
    int cM, cm, cp, cR, cN;
    int uM, um, up, uR, uN;
    parseVersion(candidate, cM, cm, cp, cR, cN);
    parseVersion(current, uM, um, up, uR, uN);
    if (cM != uM)
        return cM > uM;
    if (cm != um)
        return cm > um;
    if (cp != up)
        return cp > up;
    // Same M.m.p — stable (rank 0) beats any prerelease (rank 1-3)
    // For two prereleases: higher rank beats lower (rc > beta > alpha)
    int cEff = (cR == 0) ? 1000 : cR; // stable → very high
    int uEff = (uR == 0) ? 1000 : uR;
    if (cEff != uEff)
        return cEff > uEff;
    return cN > uN;
}

static void handleUpdateCheck()
{
    if (!staConnected)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"WiFi not connected\"}");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url;
    if (updateBetaChannel)
        url = "https://api.github.com/repos/" + String(GITHUB_REPO) + "/releases?per_page=1";
    else
        url = "https://api.github.com/repos/" + String(GITHUB_REPO) + "/releases/latest";

    http.begin(client, url);
    http.setTimeout(20000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("User-Agent", "ESP32-OTA");
    int code = http.GET();

    if (code != 200)
    {
        http.end();
        String msg = code <= 0
                         ? "GitHub unreachable from ESP32. Use manual firmware upload."
                         : "GitHub API error " + String(code);
        server.send(502, "application/json", "{\"ok\":false,\"error\":\"" + jsonEscape(msg.c_str()) + "\"}");
        return;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err)
    {
        server.send(500, "application/json", "{\"ok\":false,\"error\":\"JSON parse error\"}");
        return;
    }

    // Find the right release
    JsonObject release;
    if (updateBetaChannel)
    {
        JsonArray arr = doc.as<JsonArray>();
        for (JsonObject r : arr)
        {
            release = r;
            break; // first (newest) release
        }
    }
    else
    {
        release = doc.as<JsonObject>();
    }

    if (release.isNull())
    {
        server.send(404, "application/json", "{\"ok\":false,\"error\":\"No release found\"}");
        return;
    }

    String tagName = release["tag_name"] | "";
    bool prerelease = release["prerelease"] | false;
    String version = tagName;
    if (version.startsWith("v"))
        version = version.substring(1);

    // Find the matching firmware asset
    String downloadUrl = "";
    const char *artifact = getFirmwareArtifact();
    JsonArray assets = release["assets"];
    for (JsonObject asset : assets)
    {
        String name = asset["name"] | "";
        if (name == artifact)
        {
            downloadUrl = String(asset["browser_download_url"] | "");
            break;
        }
    }

    String j = "{\"ok\":true";
    j += ",\"current\":\"" + jsonEscape(FIRMWARE_VERSION) + "\"";
    j += ",\"latest\":\"" + jsonEscape(version.c_str()) + "\"";
    j += ",\"tag\":\"" + jsonEscape(tagName.c_str()) + "\"";
    j += ",\"prerelease\":" + String(prerelease ? "true" : "false");
    j += ",\"artifact\":\"" + jsonEscape(artifact) + "\"";
    j += ",\"url\":\"" + jsonEscape(downloadUrl.c_str()) + "\"";
    bool isNewer = isVersionNewer(version, String(FIRMWARE_VERSION));
    j += ",\"update\":" + String(isNewer && downloadUrl.length() > 0 ? "true" : "false");
    j += ",\"beta\":" + String(updateBetaChannel ? "true" : "false");
    j += "}";
    server.send(200, "application/json", j);
}

static void handleUpdateInstall()
{
    if (!staConnected)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"WiFi not connected\"}");
        return;
    }

    String url = server.arg("url");
    if (url.length() == 0)
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"No URL provided\"}");
        return;
    }

    dashLog("[OTA] Starting GitHub update from: " + url);
    server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Downloading and installing... Device will reboot.\"}");
    delay(500);

    WiFiClientSecure client;
    client.setInsecure();

    // Follow redirects — GitHub release assets redirect to S3
    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.begin(client, url);
    http.setTimeout(30000);
    http.addHeader("Accept", "application/octet-stream");
    int code = http.GET();

    if (code != 200)
    {
        dashLog("[OTA] Download failed: HTTP " + String(code));
        http.end();
        return;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0)
    {
        dashLog("[OTA] Invalid content length: " + String(contentLength));
        http.end();
        return;
    }

    dashLog("[OTA] Downloading " + String(contentLength) + " bytes...");

    if (!Update.begin(contentLength))
    {
        dashLog("[OTA] Update.begin failed: " + String(Update.errorString()));
        http.end();
        return;
    }

    WiFiClient *stream = http.getStreamPtr();
    size_t written = Update.writeStream(*stream);
    http.end();

    if (written != (size_t)contentLength)
    {
        dashLog("[OTA] Written " + String(written) + " of " + String(contentLength) + " bytes: " + String(Update.errorString()));
        Update.abort();
        return;
    }

    if (!Update.end(true))
    {
        dashLog("[OTA] Update finalize failed: " + String(Update.errorString()));
        return;
    }

    if (!Update.isFinished())
    {
        dashLog("[OTA] Update not finished");
        return;
    }

    dashLog("[OTA] Update successful! Rebooting...");
    delay(1000);
    ESP.restart();
}

// Check GitHub for a newer release and, if found, download + install it.
// Blocking; on success calls ESP.restart() and never returns.
static void performAutoUpdate()
{
    if (!staConnected)
        return;

    dashLog("[AUTO-OTA] Checking for updates...");

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url;
    if (updateBetaChannel)
        url = "https://api.github.com/repos/" + String(GITHUB_REPO) + "/releases?per_page=1";
    else
        url = "https://api.github.com/repos/" + String(GITHUB_REPO) + "/releases/latest";

    http.begin(client, url);
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("User-Agent", "ESP32-OTA");
    int code = http.GET();
    if (code != 200)
    {
        dashLog("[AUTO-OTA] GitHub API error " + String(code));
        http.end();
        return;
    }
    String payload = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, payload))
    {
        dashLog("[AUTO-OTA] JSON parse error");
        return;
    }

    JsonObject release;
    if (updateBetaChannel)
    {
        JsonArray arr = doc.as<JsonArray>();
        for (JsonObject r : arr)
        {
            release = r;
            break;
        }
    }
    else
    {
        release = doc.as<JsonObject>();
    }
    if (release.isNull())
    {
        dashLog("[AUTO-OTA] No release found");
        return;
    }

    String tagName = release["tag_name"] | "";
    String version = tagName;
    if (version.startsWith("v"))
        version = version.substring(1);
    if (!isVersionNewer(version, String(FIRMWARE_VERSION)))
    {
        dashLog("[AUTO-OTA] No newer release (latest=" + version + ", current=" FIRMWARE_VERSION ")");
        return;
    }

    const char *artifact = getFirmwareArtifact();
    String downloadUrl = "";
    for (JsonObject asset : release["assets"].as<JsonArray>())
    {
        String name = asset["name"] | "";
        if (name == artifact)
        {
            downloadUrl = String(asset["browser_download_url"] | "");
            break;
        }
    }
    if (!downloadUrl.length())
    {
        dashLog("[AUTO-OTA] No matching artifact for this build");
        return;
    }

    dashLog("[AUTO-OTA] Update " + version + " available. Installing...");

    HTTPClient http2;
    http2.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http2.begin(client, downloadUrl);
    http2.addHeader("Accept", "application/octet-stream");
    int code2 = http2.GET();
    if (code2 != 200)
    {
        dashLog("[AUTO-OTA] Download failed: HTTP " + String(code2));
        http2.end();
        return;
    }
    int len = http2.getSize();
    if (len <= 0)
    {
        dashLog("[AUTO-OTA] Invalid content length: " + String(len));
        http2.end();
        return;
    }
    if (!Update.begin(len))
    {
        dashLog("[AUTO-OTA] Update.begin failed: " + String(Update.errorString()));
        http2.end();
        return;
    }
    WiFiClient *stream = http2.getStreamPtr();
    size_t written = Update.writeStream(*stream);
    http2.end();
    if (written != (size_t)len)
    {
        dashLog("[AUTO-OTA] Written " + String(written) + "/" + String(len) + " bytes: " + String(Update.errorString()));
        Update.abort();
        return;
    }
    if (!Update.end(true))
    {
        dashLog("[AUTO-OTA] Finalize failed: " + String(Update.errorString()));
        return;
    }
    dashLog("[AUTO-OTA] Update successful! Rebooting...");
    delay(1000);
    ESP.restart();
}

static void handleAutoUpdate()
{
    if (server.hasArg("enabled"))
    {
        autoUpdateEnabled = server.arg("enabled") == "1";
        prefs.begin(PREFS_NS, false);
        prefs.putBool("auto_upd", autoUpdateEnabled);
        prefs.end();
        dashLog("[AUTO-OTA] " + String(autoUpdateEnabled ? "enabled" : "disabled"));
    }
    String j = "{\"ok\":true,\"enabled\":";
    j += autoUpdateEnabled ? "true" : "false";
    j += "}";
    server.send(200, "application/json", j);
}

static void handleUpdateBeta()
{
    if (server.hasArg("beta"))
    {
        updateBetaChannel = server.arg("beta") == "1";
        prefs.begin(PREFS_NS, false);
        prefs.putBool("update_beta", updateBetaChannel);
        prefs.end();
        dashLog("[OTA] Channel: " + String(updateBetaChannel ? "beta" : "stable"));
    }
    String j = "{\"ok\":true,\"beta\":" + String(updateBetaChannel ? "true" : "false");
    j += ",\"version\":\"" + jsonEscape(FIRMWARE_VERSION) + "\"}";
    server.send(200, "application/json", j);
}

// Dashboard frame callback wrapper

static void webTask(void *)
{
    for (;;)
    {
        ArduinoOTA.handle();
        server.handleClient();
        dashCheckWifi();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static CarManagerBase *handlerPool[3] = {};

static void dashInitHandlers()
{
    handlerPool[0] = new LegacyHandler();
    handlerPool[1] = new HW3Handler();
    handlerPool[2] = new HW4Handler();
    for (int i = 0; i < 3; i++)
    {
        handlerPool[i]->onFrame = mcpDashOnFrame;
    }
}

static void dashSwapHandler(uint8_t mode)
{
    if (mode > 2 || !handlerPool[mode])
        return;
    CarManagerBase *next = handlerPool[mode];
    if (dashHandler)
        next->enablePrint = (bool)dashHandler->enablePrint;
    appActiveHandler = next;
    dashHandler = next;
    dashApplyRuntimeState();
    // Update driver acceptance filters for the new handler.
    // For MCP2515 (ext) dashApplyFilters() will also fine-tune the hardware
    // filter registers. For TWAI and old MCP2515 this abstract call is enough.
    if (dashDriver)
        dashDriver->setFilters(next->filterIds(), next->filterIdCount());
    const char *hwName = "LEGACY";
    if (mode == 1)
        hwName = "HW3";
    else if (mode == 2)
        hwName = "HW4";
    dashLog("[CFG] Handler switched to " + String(hwName));
}

#if defined(DRIVER_ESP32_EXT_MCP2515)
static void mcpDashboardSetup(CarManagerBase *handler, CanDriver *driver, MCP2515 *mcp)
{
    dashHandler = handler;
    dashDriver = driver;
    dashMcp = mcp;
#else
static void mcpDashboardSetup(CarManagerBase *handler, CanDriver *driver)
{
    dashHandler = handler;
    dashDriver = driver;
#endif
    if (dashDriver)
        dashDriver->onSendFrame = mcpDashOnTxFrame;
    startMs = millis();
    fpsLastMs = millis();
    dashResetWriteProbe();

    if (!SPIFFS.begin(true))
        dashLog("[WARN] SPIFFS mount failed");

    dashLoadPrefs();
    dashGatewayLoad();
    // Always boot in AP+STA mode so the STA interface is ready immediately.
    // This prevents connection failures to saved networks caused by late mode switching.
    dashStartAccessPoint(true);
    if (apHidden)
        dashLog("[WIFI] AP SSID is hidden");
    Serial.printf("[WIFI] AP: %s  IP: %s\n", apSSID, WiFi.softAPIP().toString().c_str());

    dashInitHandlers();
    dashSwapHandler(hwMode);
    dashApplyFilters();


    ArduinoOTA.setHostname("ev-open-can-tools");
    ArduinoOTA.setPassword(DASH_OTA_PASS);
    ArduinoOTA.onStart([]()
                       { dashLog("[OTA] Starting..."); });
    ArduinoOTA.onEnd([]()
                     { dashLog("[OTA] Done -- rebooting"); });
    ArduinoOTA.onError([](ota_error_t e)
                       { dashLog("[OTA] Error: " + String(e)); });
    ArduinoOTA.begin();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/config", HTTP_POST, handleConfig);
    server.on("/logging", HTTP_POST, handleLoggingConfig);
    server.on("/frames", HTTP_GET, handleFrames);
    server.on("/log", HTTP_GET, handleLog);
    server.on("/reset_stats", HTTP_POST, handleResetStats);
    server.on("/rec_start", HTTP_POST, handleRecStart);
    server.on("/rec_stop", HTTP_POST, handleRecStop);
    server.on("/rec_status", HTTP_GET, handleRecStatus);
#ifdef DRIVER_T2CAN_DUAL
    server.on("/service_mode", HTTP_GET, handleServiceMode);
    server.on("/burst", HTTP_GET, handleBurst);
    server.on("/bus2_filter", HTTP_GET, handleBus2Filter);
    server.on("/stalk_test", HTTP_GET, handleStalkTest);
    server.on("/bus2_ids", HTTP_GET, handleBus2Ids);
#endif
    server.on("/rec_download", HTTP_GET, handleRecDownload);
    server.on("/disable", HTTP_POST, handleDisable);
    server.on("/reboot", HTTP_POST, handleReboot);
    server.on("/update", HTTP_POST, handleOtaResult, handleOtaUpload);
    server.on("/ap_config", HTTP_POST, handleApConfig);
    server.on("/ap_status", HTTP_GET, handleApStatus);
    server.on("/can_pins", HTTP_GET, handleCanPins);
    server.on("/can_pins", HTTP_POST, handleCanPinsSave);
    server.on("/settings_export", HTTP_GET, handleSettingsExport);
    server.on("/settings_import", HTTP_POST, handleSettingsImport);
    server.on("/wifi_scan", HTTP_GET, handleWifiScan);
    server.on("/wifi_config", HTTP_POST, handleWifiConfig);
    server.on("/wifi_status", HTTP_GET, handleWifiStatus);
    server.on("/system_status", HTTP_GET, handleSystemStatus);
    server.on("/task_stats", HTTP_GET, handleTaskStats);
    server.on("/wifi_networks", HTTP_GET, handleWifiNetworks);
    server.on("/wifi_connect", HTTP_POST, handleWifiConnect);
    server.on("/wifi_delete", HTTP_POST, handleWifiDelete);
    server.on("/update_check", HTTP_GET, handleUpdateCheck);
    server.on("/update_install", HTTP_POST, handleUpdateInstall);
    server.on("/update_beta", HTTP_POST, handleUpdateBeta);
    server.on("/auto_update", HTTP_GET, handleAutoUpdate);
    server.on("/auto_update", HTTP_POST, handleAutoUpdate);
#if defined(ESP_PLATFORM) && defined(DASH_STA_AP_GATEWAY)
    server.on("/gateway_status", HTTP_GET, handleGatewayStatus);
    server.on("/gateway_dns", HTTP_GET, handleGatewayDnsGet);
    server.on("/gateway_dns", HTTP_POST, handleGatewayDnsPost);
    server.on("/gateway_dns_test", HTTP_GET, handleGatewayDnsTest);
    server.on("/gateway_dns_stats_reset", HTTP_POST, handleGatewayDnsStatsReset);
    server.on("/gateway_whitelist_add", HTTP_POST, handleGatewayWhitelistAdd);
    server.on("/gateway_blocked", HTTP_GET, handleGatewayBlocked);
    server.on("/gateway_blocked_clear", HTTP_POST, handleGatewayBlockedClear);
#endif

    server.begin();
    if (strlen(staSSID) > 0)
        dashScheduleSTAConnect(kDashStaBootDelayMs);
#if CONFIG_FREERTOS_UNICORE
    xTaskCreate(webTask, "web", 8192, nullptr, 1, nullptr);
#else
    xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 1, nullptr, 1);
#endif
    Serial.println("[WEB] Dashboard: http://" + WiFi.softAPIP().toString());
    dashLog("[BOOT] ev-open-can-tools ready");
}

static void mcpDashboardLoop()
{
    if (Update.isRunning())
        return;
    dashSerialDiagnosticsPoll();
    if (recActive && (millis() - recStartMs >= kRecMaxDurationMs))
        dashStopRecordingAndSave("time limit");
    dashCheckBusHealth();
    if (canOnline && millis() - lastFrameMs > 10000)
    {
        canOnline = false;
        dashLog("[CAN] Bus OFFLINE (timeout)");
    }
#if defined(DASH_RGB_STATUS_LED)
    appRefreshStatusLed(false);
#endif
}

#endif
