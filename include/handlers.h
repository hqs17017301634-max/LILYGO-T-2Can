#pragma once

#include <memory>
#include <algorithm>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"
#include "shared_types.h"
#include "log_buffer.h"
#include "dash_hw3_speed.h"
#include "dash_legacy_speed.h"

#ifndef DASH_FSD_252_COMPAT
#define DASH_FSD_252_COMPAT 0
#endif

#ifndef NATIVE_BUILD
#ifdef ESP_PLATFORM
#include "platform/espidf_runtime.h"
#else
#include <Arduino.h>
#endif
#endif

inline LogRingBuffer logRing;

static inline bool framePayloadChanged(const CanFrame &original, const CanFrame &modified)
{
    if (original.id != modified.id || original.dlc != modified.dlc)
        return true;

    const uint8_t dlc = (original.dlc <= 8) ? original.dlc : 8;
    for (uint8_t i = 0; i < dlc; ++i)
    {
        if (original.data[i] != modified.data[i])
            return true;
    }
    return false;
}

struct CarManagerBase
{
    Shared<int> speedProfile{1};
    Shared<bool> speedProfileAuto{true};
    Shared<bool> ADEnabled{false};
    Shared<bool> APActive{false};
    // Default Parked=true so the AP Injection Gate opens immediately on
    // module boot when the DI is asleep (e.g. car locked with Sentry on,
    // CAN ID 280 not broadcast). The first DI_systemStatus frame with a
    // driving gear (R/N/D) flips this to false; if 280 never arrives,
    // the car is asleep / parked and the gate stays open by design.
    Shared<bool> Parked{true};
    Shared<bool> Summoning{false};
    Shared<int> gatewayAutopilot{-1};
    Shared<bool> enablePrint{true};
    Shared<uint32_t> frameCount{0};
    Shared<uint32_t> framesSent{0};
    Shared<int> speedOffset{0};

    unsigned long lastSummonActivityMs = 0;
    // Summon-vs-AP/TACC discrimination state. ACA (DI_autonomyControlActive)
    // alone is set during AP, TACC, and Smart Summon, so it cannot be the
    // sole gate signal. We only treat ACA as "summon active" when we have
    // also observed UI_selfParkRequest go non-zero during the current
    // autonomy episode. ACA falling edge clears sprSeen so the next ACA
    // rising edge (e.g. user engaging TACC after a completed summon) does
    // not falsely keep the gate open.
    bool sprSeen = false;
    bool lastAca = false;

    void (*onFrame)(const CanFrame &) = nullptr;
    void (*onSend)(uint8_t mux, bool ok) = nullptr;
    bool (*checkAD)() = nullptr;
    bool (*checkNag)() = nullptr;
    bool (*checkSummon)() = nullptr;
    bool (*checkIsa)() = nullptr;
    bool (*checkEvd)() = nullptr;

    bool injectionGateOpen() const
    {
        return (bool)APActive || (bool)Parked || (bool)Summoning;
    }

    // Recompute Summoning from current sprSeen + lastAca state. Summoning
    // requires both: ACA bit currently set AND we have seen at least one
    // UI_selfParkRequest non-zero command in the current autonomy episode.
    // This excludes plain TACC (ACA=1, no spr) and post-AP ACA tail
    // (ACA blip with no fresh spr) from latching the gate.
    void recomputeSummoning()
    {
        Summoning = lastAca && sprSeen;
    }

    // Update summon state from UI_driverAssistControl (CAN ID 1016).
    // Tesla DBC: UI_selfParkRequest at byte 3 bits 4-7 (4=PRIME, 5=PAUSE,
    // 7/8=AUTO_SUMMON_FWD/REV, 11=SMART_SUMMON, 0=NONE). Records that a
    // summon command has been issued during the current autonomy episode.
    void updateSummonFrom1016(const CanFrame &frame)
    {
        if (frame.dlc < 4)
            return;
        uint8_t spr = static_cast<uint8_t>((frame.data[3] >> 4) & 0x0F);
        if (spr != 0)
            sprSeen = true;
        recomputeSummoning();
    }

    // Update summon state from DI_systemStatus (CAN ID 280).
    // Tesla DBC: DI_autonomyControlActive at bit 50 (byte 6 bit 2). Held
    // high while the DI is being driven by AP, TACC, Smart Summon, etc.
    // ACA falling edge ends the autonomy episode and clears sprSeen so a
    // subsequent TACC engagement (ACA=1 again) does not re-latch the gate.
    void updateSummonFromDISystemStatus(const CanFrame &frame)
    {
        if (frame.dlc < 7)
            return;
        bool aca = (frame.data[6] & 0x04) != 0;
        if (lastAca && !aca)
            sprSeen = false;
        lastAca = aca;
        recomputeSummoning();
    }

    // Force Summoning off and reset sprSeen when the vehicle is observed
    // in Park with no active autonomy episode, so a manual P->D shift
    // afterwards correctly waits for AP. During Smart Summon startup the
    // DI can report ACA=1 while gear is still P; keep sprSeen latched so
    // it survives the pending shift out of Park.
    void clearSummonOnPark()
    {
        Summoning = false;
        sprSeen = false;
#ifndef NATIVE_BUILD
        lastSummonActivityMs = 0;
#endif
    }

    void clearSummonOnParkIfAcaInactive(uint8_t gear)
    {
        if (gear == 1 && !lastAca)
            clearSummonOnPark();
    }

    bool shouldInjectSpeedProfile() const
    {
#if defined(ESP32_DASHBOARD)
        return !speedProfileAuto;
#else
        return true;
#endif
    }

    virtual void handleMessage(CanFrame &frame, CanDriver &driver) = 0;
    virtual const uint32_t *filterIds() const = 0;
    virtual uint8_t filterIdCount() const = 0;
    virtual ~CarManagerBase() = default;
};

struct LegacyHandler : public CarManagerBase
{
    const uint32_t *filterIds() const override
    {
        // 760 added for UI_mppSpeedLimit override (Legacy MPP custom-speed feature).
        static constexpr uint32_t ids[] = {69, 280, 390, 760, 921, 1006};
        return ids;
    }
    uint8_t filterIdCount() const override { return 6; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (onFrame)
            onFrame(frame);
        // STW_ACTN_RQ (0x045 = 69): Follow-Distance-Stalk as Source for Profile Mapping
        // byte[1]: 0x00=Pos1, 0x21=Pos2, 0x42=Pos3, 0x64=Pos4, 0x85=Pos5, 0xA6=Pos6, 0xC8=Pos7
        if (frame.id == 69)
        {
            if (frame.dlc < 2)
                return;
            if (!speedProfileAuto)
                return;
            uint8_t pos = frame.data[1] >> 5;
            if (pos <= 1)
                speedProfile = 2;
            else if (pos == 2)
                speedProfile = 1;
            else
                speedProfile = 0;
            return;
        }
        // UI_gpsVehicleSpeed (0x2F8 = 760): UI_mppSpeedLimit raise-only override.
        // byte 6 low 5 bits hold the AP-fused/MPP speed limit raw, where
        // raw × 5 = km/h (max raw 31 → 155 km/h). We bucket-look up a target
        // km/h based on the gateway's current raw_mpp using the same low /
        // high bucket layout as HW3, then write the higher value back ONLY
        // when our target exceeds what the gateway sent. Byte 7 vehicle
        // checksum must be recomputed because byte 6 changed.
        if (frame.id == 760)
        {
            if (frame.dlc < 8) return;
            uint8_t rawMpp = frame.data[6] & 0x1F;
            legacyMppLastRaw = rawMpp;
            if (!dashLegacyMppActive()) return;
            if (rawMpp == 0) return;                  // gateway has no valid MPP yet
            int currentKph = static_cast<int>(rawMpp) * 5;
            uint16_t targetKph = dashComputeLegacyMppTargetKph(currentKph);
            if (targetKph == 0) return;               // no enabled feature covers this bucket
            if (static_cast<int>(targetKph) <= currentKph) return; // raise-only
            int targetKphClamped = std::min<int>(targetKph, kLegacyMppMaxKph);
            uint8_t targetRaw = static_cast<uint8_t>(targetKphClamped / 5);
            if (targetRaw > kLegacyMppMaxRaw) targetRaw = kLegacyMppMaxRaw;
            frame.data[6] = (frame.data[6] & 0xE0) | (targetRaw & 0x1F);
            frame.data[7] = computeVehicleChecksum(frame);
            legacyMppLastSentRaw = targetRaw;
            framesSent++;
            driver.send(frame);
            if (onSend) onSend(0, true);
            return;
        }
        if (frame.id == 280)
        {
            if (frame.dlc < 3)
                return;
            {
                uint8_t diGear = readDIGear(frame);
                Parked = isVehicleParked(diGear);
                // Only clear Summoning on a *definitive* Park (gear==1).
                // SNA (7) and INVALID (0) can blip during gear transitions
                // (e.g. during a Summon shift to Reverse) and would
                // otherwise drop the gate mid-summon.
                updateSummonFromDISystemStatus(frame);
                clearSummonOnParkIfAcaInactive(diGear);
            }
            return;
        }
        if (frame.id == 390)
        {
            if (frame.dlc < 8)
                return;
            {
                uint8_t difGear = readVehicleGear(frame);
                Parked = isVehicleParked(difGear);
                // Only clear Summoning on a *definitive* Park (gear==1).
                // SNA (7) and INVALID (0) can blip during gear transitions.
                clearSummonOnParkIfAcaInactive(difGear);
            }
            return;
        }
        if (frame.id == 921)
        {
            if (frame.dlc < 1)
                return;
            APActive = isDASAutopilotActive(readDASAutopilotStatus(frame));
            return;
        }
        if (frame.id == 1006)
        {
            if (frame.dlc < 8)
                return;
            auto index = readMuxID(frame);
            if (index == 0)
            {
                const bool fsdRequested = forceActivateRuntime || isADSelectedInUI(frame);
                ADEnabled = fsdRequested && (!checkAD || checkAD());
            }
            if (index == 0 && ADEnabled && (!checkAD || checkAD()))
            {
#if defined(ESP32_DASHBOARD) && DASH_FSD_252_COMPAT
                // Dashboard compatibility path injects after the handler from
                // the saved original 1006 mux0 frame, matching the Legacy
                // plugin rule: copy original, set bit46, optionally apply the
                // selected driving profile, then send once.
#else
                if (shouldInjectSpeedProfile())
                    setSpeedProfileV12V13(frame, speedProfile);
                setBit(frame, 46, true);
                // Match the stable FSD mode path: request smart speed offset
                // together with the FSD latch when the master switch is enabled.
                setBit(frame, 40, true);
                setBit(frame, 41, true);
                framesSent++;
                driver.send(frame);
                if (onSend)
                    onSend(0, true);
#endif
            }
            if (index == 1 && (!checkNag || checkNag()))
            {
#if !defined(ESP32_DASHBOARD)
                setBit(frame, 19, false);
                framesSent++;
                driver.send(frame);
                if (onSend)
                    onSend(1, true);
#endif
            }
            if (index == 0 && enablePrint)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "LegacyHandler: AD: %d, Profile: %d",
                         (bool)ADEnabled, (int)speedProfile);
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
        }
    }
};

struct HW3Handler : public CarManagerBase
{
    const uint32_t *filterIds() const override
    {
        static constexpr uint32_t ids[] = {280, 390, 921, 1016, 1021, 2047};
        return ids;
    }
    uint8_t filterIdCount() const override { return 6; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (onFrame)
            onFrame(frame);
        if (frame.id == 280)
        {
            if (frame.dlc < 3)
                return;
            {
                uint8_t diGear = readDIGear(frame);
                Parked = isVehicleParked(diGear);
                // Only clear Summoning on a *definitive* Park (gear==1).
                // SNA (7) and INVALID (0) can blip during gear transitions
                // (e.g. during a Summon shift to Reverse) and would
                // otherwise drop the gate mid-summon.
                updateSummonFromDISystemStatus(frame);
                clearSummonOnParkIfAcaInactive(diGear);
            }
            return;
        }
        if (frame.id == 390)
        {
            if (frame.dlc < 8)
                return;
            {
                uint8_t difGear = readVehicleGear(frame);
                Parked = isVehicleParked(difGear);
                // Only clear Summoning on a *definitive* Park (gear==1).
                // SNA (7) and INVALID (0) can blip during gear transitions.
                clearSummonOnParkIfAcaInactive(difGear);
            }
            return;
        }
        if (frame.id == 1016)
        {
            if (frame.dlc < 6)
                return;
            updateSummonFrom1016(frame);
            if (!speedProfileAuto)
                return;
            uint8_t followDistance = (frame.data[5] & 0b11100000) >> 5;
            switch (followDistance)
            {
            case 1:
                speedProfile = 2;
                break;
            case 2:
                speedProfile = 1;
                break;
            case 3:
                speedProfile = 0;
                break;
            default:
                break;
            }
            return;
        }
        if (frame.id == 921)
        {
            if (frame.dlc < 1)
                return;
            APActive = isDASAutopilotActive(readDASAutopilotStatus(frame));
            // Capture ISA fused speed limit from byte1[4:0]. raw*5 = kph;
            // 0 = SNA, 31 = NONE-broadcast — both treated as "unknown" by the
            // HW3 mux-2 override path. Used by dashComputeHw3OffsetRaw().
            if (frame.dlc >= 2)
                fusedSpeedLimitRaw = static_cast<uint8_t>(frame.data[1] & 0x1F);
            return;
        }
        if (frame.id == 2047)
        {
            if (frame.dlc < 6)
                return;
            if (readMuxID(frame) != 2)
                return;

            uint8_t next = readGTWAutopilot(frame);
            int prev = gatewayAutopilot;
            gatewayAutopilot = next;

            if (enablePrint && prev != next)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "HW3Handler: GTW_autopilot: %d -> %u (%s)",
                         prev, (unsigned int)next, describeGTWAutopilot(next));
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
            return;
        }
        if (frame.id == 1021)
        {
            if (frame.dlc < 8)
                return;
            auto index = readMuxID(frame);
            if (index == 0)
            {
                const bool fsdRequested = forceActivateRuntime || isADSelectedInUI(frame);
                ADEnabled = fsdRequested && (!checkAD || checkAD());
            }
            if (index == 0 && ADEnabled && (!checkAD || checkAD()))
            {
                speedOffset = std::max(std::min(((uint8_t)((frame.data[3] >> 1) & 0x3F) - 30) * 5, 100), 0);
                // Mirror stock offset to a global so the mux-2 override path
                // (and the WebUI status JSON) can read it without going
                // through the Shared<int> wrapper.
                hw3StockOffsetKph = speedOffset;
#if defined(ESP32_DASHBOARD) && DASH_FSD_252_COMPAT
                // 2.5.2 compat sends after the handler from the saved
                // original frame, once the AP Gate allows injection.
#else
                // Built-in full activation sequence for non-compat builds.
                setSpeedProfileV12V13(frame, speedProfile);
                setBit(frame, 46, true);
                framesSent++;
                driver.send(frame);
                if (onSend)
                    onSend(0, true);
#endif
            }
            if (index == 1 && (!checkAD || checkAD()))
            {
#if defined(ESP32_DASHBOARD) && DASH_FSD_252_COMPAT
                // 2.5.2 AD plugin did not shadow mux 1; keep bus writes minimal.
#else
                // HW3 mux 1: clear nag bit ONLY. Reference RP2040CAN-FSD
                // (field-stable on the same car this firmware targets) sends
                // ONLY bit 19=0 on mux 1 — no bit 46. tesla-open-can-mod and
                // tesla-fsd-controller-main agree. Setting bit 46 here on top
                // of the gateway's stock mux 1 byte 5 fights other unrelated
                // signals in that byte and on some firmwares destabilizes the
                // grey wheel. Also note: this fires unconditionally (just
                // requires CAN injection on), not gated on ADEnabled — nag
                // suppression is harmless when FSD isn't selected.
                bool modified = false;
                setBit(frame, 19, false);
                modified = true;
#if !defined(ESP32_DASHBOARD)
#if defined(ENHANCED_AUTOPILOT)
                if (enhancedAutopilotRuntime && enhancedAutopilotInjectionAllowed(injectionGateOpen()))
                {
                    // already set above
                }
#endif
#endif
                if (modified)
                {
                    framesSent++;
                    driver.send(frame);
                    if (onSend)
                        onSend(1, true);
                }
#endif
            }
#if defined(ESP32_DASHBOARD) && DASH_FSD_252_COMPAT
            // Stable FSD compatibility keeps mux 0 as bit46-only post-handler
            // injection. Restore only the HW3 custom-speed write on mux 2:
            // no readiness assist, no stock passthrough, no default cap frame.
            if (index == 2 && (ADEnabled || forceActivateRuntime) && dashHw3CustomSpeedActive())
            {
                uint8_t fl = fusedSpeedLimitRaw;
                if (fl > 0 && fl < 31)
                {
                    CanFrame shaped = frame;
                    uint8_t activeRaw = dashComputeHw3OffsetRaw(speedOffset);
                    hw3OffsetTargetRaw = activeRaw;
                    dashWriteHw3OffsetRawShared(shaped, activeRaw);
                    dashApplyHw3OffsetSlew(shaped, frame);

                    if (framePayloadChanged(frame, shaped))
                    {
                        framesSent++;
                        bool ok = driver.send(shaped);
                        if (onSend)
                            onSend(2, ok);
                    }
                }
            }
#endif
#if defined(ESP32_DASHBOARD) && !DASH_FSD_252_COMPAT
            // ─── 1021 mux 2: HW3 stock-offset passthrough + optional boost ─
            // 2.3.2-beta.3 always re-injected mux 2 with the stock speed
            // offset captured from mux 0 byte 3, even when no custom-speed
            // feature was active. That passthrough is what makes the ECU's
            // readiness check pass — without it the grey wheel flickers.
            // The optional Custom/Auto/HighSpeed boost overrides the offset
            // value but the *frame itself* must always be re-injected.
            if (index == 2 && (ADEnabled || forceActivateRuntime))
            {
                CanFrame shaped = frame;
                if (dashHw3CustomSpeedActive())
                {
                    uint8_t activeRaw = dashComputeHw3OffsetRaw(speedOffset);
                    hw3OffsetTargetRaw = activeRaw;
                    dashWriteHw3OffsetRawShared(shaped, activeRaw);
                    dashApplyHw3OffsetSlew(shaped, frame);
                }
                else
                {
                    // IDF reference default (fsdSpeedOffsetEnabled=true):
                    // set the FSD speed-offset cap to 60% by writing 0x0f
                    // into mux-2 byte1[0:5] while leaving byte0 offset bits
                    // untouched. The previous stock-offset passthrough wrote
                    // raw=50, which changed byte0[6:7] and did not match the
                    // stable "FSD mode CN" behavior.
                    shaped.data[1] = static_cast<uint8_t>((shaped.data[1] & 0xC0) | 0x0F);
                    uint8_t raw = 0;
                    if (dashReadHw3OffsetRawShared(shaped, raw))
                        hw3OffsetTargetRaw = raw;
                }
                framesSent++;
                driver.send(shaped);
                if (onSend)
                    onSend(2, true);
            }
#endif
            if (index == 0 && enablePrint)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "HW3Handler: AD: %d, Profile: %d, Offset: %d",
                         (bool)ADEnabled, (int)speedProfile, (int)speedOffset);
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
        }
    }
};

/**
 * NagHandler — Autosteer nag suppression (counter+1 echo method)
 *
 * Replicates the Chinese TSL6P module behavior:
 * - Listens for CAN 880 (0x370) = EPAS3P_sysStatus
 * - When handsOnLevel = 0 (nag would trigger):
 *   1. Copies the real frame
 *   2. Sets byte 3 = 0xB6 (fixed torsionBarTorque = 1.80 Nm)
 *   3. Sets byte 4 |= 0x40 (handsOnLevel = 1)
 *   4. Increments counter (byte 6 lower nibble + 1)
 *   5. Recalculates checksum (byte 7)
 * - The real EPAS frame with the same counter arrives AFTER -> rejected as duplicate
 *
 * Tested: Model Y Performance 2022 HW3, Basic Autopilot
 * Bus: X179 pin 2/3 (CAN bus 4)
 *
 * Enable with build flag: -D NAG_KILLER
 */
struct NagHandler : public CarManagerBase
{
    Shared<bool> nagKillerActive{true};
    Shared<uint32_t> nagEchoCount{0};

    const uint32_t *filterIds() const override
    {
        static constexpr uint32_t ids[] = {880};
        return ids;
    }
    uint8_t filterIdCount() const override { return 1; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (frame.id != 880 || frame.dlc < 8)
            return;

        uint8_t handsOn = (frame.data[4] >> 6) & 0x03;

        if (!nagKillerActive || !nagKillerRuntime || handsOn != 0)
            return;

        CanFrame echo;
        echo.id = 880;
        echo.dlc = 8;

        echo.data[0] = frame.data[0];
        echo.data[1] = frame.data[1];
        echo.data[2] = (frame.data[2] & 0xF0) | 0x08;
        echo.data[5] = frame.data[5];

        // Fixed torque = 1.80 Nm (tRaw = 0x08B6)
        echo.data[3] = 0xB6;

        // handsOnLevel = 1
        echo.data[4] = frame.data[4] | 0x40;

        // Counter + 1
        uint8_t cnt = (frame.data[6] & 0x0F);
        cnt = (cnt + 1) & 0x0F;
        echo.data[6] = (frame.data[6] & 0xF0) | cnt;

        // Checksum: sum(byte0..byte6) + 0x73
        uint16_t sum = echo.data[0] + echo.data[1] + echo.data[2] + echo.data[3] + echo.data[4] + echo.data[5] + echo.data[6];
        echo.data[7] = static_cast<uint8_t>((sum + 0x73) & 0xFF);

        framesSent++;
        nagEchoCount++;
        driver.send(echo);

        if (enablePrint && (nagEchoCount % 500 == 1))
        {
            char buf[LogRingBuffer::kMaxMsgLen];
            snprintf(buf, sizeof(buf), "NagHandler: echo=%u",
                     (unsigned int)(uint32_t)nagEchoCount);
            logRing.push(buf,
#ifndef NATIVE_BUILD
                         millis()
#else
                         0
#endif
            );
#ifndef NATIVE_BUILD
            Serial.println(buf);
#endif
        }
    }
};

struct HW4Handler : public CarManagerBase
{
    const uint32_t *filterIds() const override
    {
#if defined(ISA_SPEED_CHIME_SUPPRESS) && !defined(ESP32_DASHBOARD)
        static constexpr uint32_t ids[] = {280, 390, 921, 1016, 1021, 2047};
        return ids;
    }
    uint8_t filterIdCount() const override { return 6; }
#else
        static constexpr uint32_t ids[] = {280, 390, 921, 1016, 1021, 2047};
        return ids;
    }
    uint8_t filterIdCount() const override { return 6; }
#endif

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (onFrame)
            onFrame(frame);
        if (frame.id == 280)
        {
            if (frame.dlc < 3)
                return;
            {
                uint8_t diGear = readDIGear(frame);
                Parked = isVehicleParked(diGear);
                // Only clear Summoning on a *definitive* Park (gear==1).
                // SNA (7) and INVALID (0) can blip during gear transitions
                // (e.g. during a Summon shift to Reverse) and would
                // otherwise drop the gate mid-summon.
                updateSummonFromDISystemStatus(frame);
                clearSummonOnParkIfAcaInactive(diGear);
            }
            return;
        }
        if (frame.id == 390)
        {
            if (frame.dlc < 8)
                return;
            {
                uint8_t difGear = readVehicleGear(frame);
                Parked = isVehicleParked(difGear);
                // Only clear Summoning on a *definitive* Park (gear==1).
                // SNA (7) and INVALID (0) can blip during gear transitions.
                clearSummonOnParkIfAcaInactive(difGear);
            }
            return;
        }
        if (frame.id == 921)
        {
            if (frame.dlc < 1)
                return;
            APActive = isDASAutopilotActive(readDASAutopilotStatus(frame));
            // Capture ISA fused speed limit; same path as HW3.
            if (frame.dlc >= 2)
                fusedSpeedLimitRaw = static_cast<uint8_t>(frame.data[1] & 0x1F);
        }
#if defined(ISA_SPEED_CHIME_SUPPRESS) && !defined(ESP32_DASHBOARD)
        if (isaSpeedChimeSuppressRuntime && frame.id == 921)
        {
            if (frame.dlc < 8)
                return;
            if (!isaSpeedChimeSuppressRuntime)
                return;
            frame.data[1] |= 0x20;
            uint8_t sum = 0;
            for (int i = 0; i < 7; i++)
                sum += frame.data[i];
            sum += (921 & 0xFF) + (921 >> 8);
            frame.data[7] = sum & 0xFF;
            framesSent++;
            driver.send(frame);
            if (onSend)
                onSend(0, true);
            return;
        }
#endif
        if (frame.id == 1016)
        {
            if (frame.dlc < 6)
                return;
            updateSummonFrom1016(frame);
            if (!speedProfileAuto)
                return;
            auto fd = (frame.data[5] & 0b11100000) >> 5;
            switch (fd)
            {
            case 1:
                speedProfile = 3;
                break;
            case 2:
                speedProfile = 2;
                break;
            case 3:
                speedProfile = 1;
                break;
            case 4:
                speedProfile = 0;
                break;
            case 5:
                speedProfile = 4;
                break;
            }
        }
        if (frame.id == 2047)
        {
            if (frame.dlc < 6)
                return;
            if (readMuxID(frame) != 2)
                return;

            uint8_t next = readGTWAutopilot(frame);
            int prev = gatewayAutopilot;
            gatewayAutopilot = next;

            if (enablePrint && prev != next)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "HW4Handler: GTW_autopilot: %d -> %u (%s)",
                         prev, (unsigned int)next, describeGTWAutopilot(next));
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
            return;
        }
        if (frame.id == 1021)
        {
            if (frame.dlc < 8)
                return;
            auto index = readMuxID(frame);
            if (index == 0)
            {
                const bool fsdRequested = forceActivateRuntime || isADSelectedInUI(frame);
                ADEnabled = fsdRequested && (!checkAD || checkAD());
            }
            if (index == 0 && ADEnabled && (!checkAD || checkAD()))
            {
                // Built-in FSD activation (ported from tesla-fsd-controller-main mod_fsd.h
                // handleHW4 mux-0). Bit 46 = FSD activation latch, bit 60 = HW4-specific
                // FSD enable. Done in C++ to ensure stable
                // activation matching the reference project.
                setBit(frame, 46, true);
                setBit(frame, 60, true);
#if defined(EMERGENCY_VEHICLE_DETECTION)
                if (emergencyVehicleDetectionRuntime)
                    setBit(frame, 59, true);
#endif
                framesSent++;
                driver.send(frame);
                if (onSend)
                    onSend(0, true);
            }
            if (index == 2 && ADEnabled && !speedProfileAuto && (!checkAD || checkAD()))
            {
                setSpeedProfileHW4(frame, speedProfile);
                framesSent++;
                driver.send(frame);
                if (onSend)
                    onSend(2, true);
            }
            if (index == 1 && ADEnabled && (!checkAD || checkAD()))
            {
                // Nag suppression + FSD ready (ported from tesla-fsd-controller-main
                // mod_fsd.h handleHW4 mux-1). bit 19=0 (suppress nag), bit 47=1
                // (HW4-specific FSD ready signal — without this HW4 will NOT activate).
                setBit(frame, 19, false);
                setBit(frame, 47, true);
                framesSent++;
                driver.send(frame);
                if (onSend)
                    onSend(1, true);
            }
            if (index == 0 && enablePrint)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "HW4Handler: AD: %d, Profile: %d",
                         (bool)ADEnabled, (int)speedProfile);
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
        }
    }
};
