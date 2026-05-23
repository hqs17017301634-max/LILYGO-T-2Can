#pragma once
// HW3 custom speed-limit boost — config + math helpers + slew limiter.
// Ported from tesla-fsd-controller-main (include/fsd_config.h + include/mod_fsd.h).
//
// Lives in its own header (rather than mcp2515_dashboard.h) so that
// handlers.h can read fusedSpeedLimitRaw and the encode helpers — the include
// order is handlers.h -> ... -> mcp2515_dashboard.h, so dashboard.h is too
// late. Uses C++17 inline globals for cross-TU sharing.
//
// HW3 1021 mux-2 wire format (per tesla project):
//   data[0] bits 6-7 = offset_raw bits 0-1
//   data[1] bits 0-5 = offset_raw bits 2-7
// Two encodings exist for offset_raw → speed boost:
//   KPH5: raw = offsetKph * 5  (legacy fleets)
//   PCT4: raw = pct * 4        (current default)

#include <cstdint>
#include <algorithm>
#include "can_frame_types.h"

#ifndef NATIVE_BUILD
#ifdef ESP_PLATFORM
#include "platform/espidf_runtime.h" // millis()
#else
#include <Arduino.h>
#endif
#else
inline uint32_t millis() { return 0; }
#endif

// ─── Tunables ────────────────────────────────────────────────────────────────
inline constexpr uint8_t kHw3CustomBucketBaseKph = 30;
inline constexpr uint8_t kHw3CustomBucketStepKph = 10;
inline constexpr uint8_t kHw3CustomTargetCount = 5; // 30/40/50/60/70
inline constexpr uint8_t kHw3StockOffsetCutoverKph = 80;
inline constexpr uint8_t kHw3HighSpeedBucketBaseKph = 80;
inline constexpr uint8_t kHw3HighSpeedBucketStepKph = 20;
inline constexpr uint8_t kHw3HighSpeedBucketCount = 3; // 80/100/120
inline constexpr uint8_t kHw3SpeedOffsetMaxPct = 50;
inline constexpr uint8_t kHw3WireEncKph5 = 0;
inline constexpr uint8_t kHw3WireEncPct4 = 1;
inline constexpr uint8_t kHw3WireEncDefault = kHw3WireEncPct4;
inline constexpr uint8_t kHw3CustomTargetMaxKph = 160;
inline constexpr uint8_t kHw3HighSpeedTargetMaxKph = 200;
inline constexpr uint8_t kHw3CustomTargetMaxByBucket[kHw3CustomTargetCount] = {45, 60, 75, 90, 105};
inline constexpr uint8_t kHw3HighSpeedTargetMaxByBucket[kHw3HighSpeedBucketCount] = {120, 150, 180};

// ─── Runtime state (settings) ────────────────────────────────────────────────
inline bool hw3CustomSpeed = false;
inline uint8_t hw3CustomTarget[kHw3CustomTargetCount] = {45, 60, 75, 90, 105};
inline bool hw3HighSpeedEnable = false;
inline uint8_t hw3HighSpeedTarget[kHw3HighSpeedBucketCount] = {90, 110, 130};
inline uint8_t hw3WireEncoding = kHw3WireEncDefault;

// ─── Runtime state (live values) ─────────────────────────────────────────────
// Fused/ISA speed limit raw byte from 0x399/921 byte1[4:0] (×5 = kph).
// 0 = SNA, 31 = NONE → no override (stock pass-through).
inline uint8_t fusedSpeedLimitRaw = 0;
// Latest stock offset captured from 1021 mux 0 byte3[1:6] (kph, 0..100).
inline int hw3StockOffsetKph = 0;

// ─── Math helpers ────────────────────────────────────────────────────────────
inline uint8_t dashClampHw3HighSpeedTargetKph(int v)
{
    if (v < 0) v = 0;
    if (v > kHw3HighSpeedTargetMaxKph) v = kHw3HighSpeedTargetMaxKph;
    return static_cast<uint8_t>(v);
}

inline uint8_t dashClampHw3CustomTargetKph(int v)
{
    if (v < 0) v = 0;
    if (v > kHw3CustomTargetMaxKph) v = kHw3CustomTargetMaxKph;
    return static_cast<uint8_t>(v);
}

inline uint8_t dashClampHw3CustomTargetForBucket(uint8_t idx, int v)
{
    uint8_t maxKph = idx < kHw3CustomTargetCount ? kHw3CustomTargetMaxByBucket[idx] : kHw3CustomTargetMaxKph;
    if (v < 0) v = 0;
    if (v > maxKph) v = maxKph;
    return static_cast<uint8_t>(v);
}

inline uint8_t dashClampHw3HighSpeedTargetForBucket(uint8_t idx, int v)
{
    uint8_t maxKph = idx < kHw3HighSpeedBucketCount ? kHw3HighSpeedTargetMaxByBucket[idx] : kHw3HighSpeedTargetMaxKph;
    if (v < 0) v = 0;
    if (v > maxKph) v = maxKph;
    return static_cast<uint8_t>(v);
}

// "Custom" target — looks up hw3CustomTarget[idx] for the 30/40/50/60/70 kph
// bucket containing flKph. Returns 0 outside the supported range.
inline uint16_t dashComputeHw3CustomTargetKph(uint8_t flKph)
{
    if (flKph < kHw3CustomBucketBaseKph) return 0;
    if (flKph >= kHw3StockOffsetCutoverKph) return 0;
    uint8_t idx = static_cast<uint8_t>((flKph - kHw3CustomBucketBaseKph) /
                                       kHw3CustomBucketStepKph);
    if (idx >= kHw3CustomTargetCount) idx = kHw3CustomTargetCount - 1;
    return hw3CustomTarget[idx];
}

inline uint8_t dashEncodeHw3OffsetPct4(int pct)
{
    if (pct < 0) pct = 0;
    if (pct > kHw3SpeedOffsetMaxPct) pct = kHw3SpeedOffsetMaxPct;
    return static_cast<uint8_t>(pct * 4);
}

inline uint8_t dashEncodeHw3OffsetKph5(int kph)
{
    if (kph < 0) kph = 0;
    if (kph > 40) kph = 40;
    return static_cast<uint8_t>(kph * 5);
}

inline uint8_t dashEncodeHw3Offset(int offsetKph, uint8_t flKph)
{
    if (hw3WireEncoding == kHw3WireEncPct4)
    {
        if (flKph == 0) return 0;
        int pct = (offsetKph * 100 + flKph / 2) / flKph;
        return dashEncodeHw3OffsetPct4(pct);
    }
    return dashEncodeHw3OffsetKph5(offsetKph);
}

inline bool dashHw3CustomSpeedActive()
{
    return hw3CustomSpeed || hw3HighSpeedEnable;
}

// Compute the raw byte to write into 1021 mux-2. stockOffsetRaw is the
// previously-captured stock offset from mux 0 (used as fallback).
inline uint8_t dashComputeHw3OffsetRaw(int stockOffsetRaw)
{
    uint8_t fl = fusedSpeedLimitRaw;
    if (fl == 0 || fl == 31) // SNA / NONE: pass through stock raw.
        return static_cast<uint8_t>(std::max(std::min(stockOffsetRaw, 255), 0));
    uint16_t flKph = static_cast<uint16_t>(fl) * 5;

    int desiredOffsetKph = stockOffsetRaw;

    if (flKph < kHw3StockOffsetCutoverKph && hw3CustomSpeed)
    {
        uint16_t target = dashComputeHw3CustomTargetKph(static_cast<uint8_t>(flKph));
        if (target > flKph)
            desiredOffsetKph = static_cast<int>(target - flKph);
    }
    else if (flKph >= kHw3StockOffsetCutoverKph && hw3HighSpeedEnable)
    {
        uint8_t idx = static_cast<uint8_t>((flKph - kHw3HighSpeedBucketBaseKph) /
                                           kHw3HighSpeedBucketStepKph);
        if (idx >= kHw3HighSpeedBucketCount) idx = kHw3HighSpeedBucketCount - 1;
        uint8_t targetKph = hw3HighSpeedTarget[idx];
        desiredOffsetKph = targetKph > flKph ? static_cast<int>(targetKph - flKph) : 0;
    }
    return dashEncodeHw3Offset(desiredOffsetKph, static_cast<uint8_t>(flKph));
}

// ─── 1021 mux-2 wire codec ───────────────────────────────────────────────────
inline bool dashReadHw3OffsetRawShared(const CanFrame &frame, uint8_t &raw)
{
    if (frame.id != 1021 || frame.dlc < 2)
        return false;
    raw = static_cast<uint8_t>(((frame.data[1] & 0x3F) << 2) | ((frame.data[0] >> 6) & 0x03));
    return true;
}

inline void dashWriteHw3OffsetRawShared(CanFrame &frame, uint8_t raw)
{
    frame.data[0] = static_cast<uint8_t>((frame.data[0] & ~0xC0) | ((raw & 0x03) << 6));
    frame.data[1] = static_cast<uint8_t>((frame.data[1] & ~0x3F) | (raw >> 2));
}

// ─── Slew limiter ────────────────────────────────────────────────────────────
// Damps drops in the wire offset to avoid sudden braking when the AP-fused
// limit suddenly drops (e.g. transitioning into a school zone). Only limits
// downward motion; rising edge passes through immediately. Kept here (rather
// than in mcp2515_dashboard.h) so HW3Handler::handleMessage can call it
// directly without taking a dependency on dashboard.h.

inline constexpr uint8_t kHw3SlewRateMin = 1;
inline constexpr uint8_t kHw3SlewRateMax = 25;
inline constexpr uint8_t kHw3SlewRateDefault = 25;

inline bool hw3OffsetSlew = false;
inline uint8_t hw3SlewRate = kHw3SlewRateDefault;
inline uint8_t hw3OffsetTargetRaw = 0;
inline uint8_t hw3OffsetLastRaw = 0;
inline uint32_t hw3OffsetLastSentMs = 0;
inline uint32_t hw3OffsetSlewCount = 0;

inline uint8_t dashClampHw3SlewRate(int rate)
{
    if (rate < kHw3SlewRateMin) return kHw3SlewRateMin;
    if (rate > kHw3SlewRateMax) return kHw3SlewRateMax;
    return static_cast<uint8_t>(rate);
}

inline uint8_t dashLoadHw3SlewRate(uint8_t rate)
{
    if (rate < kHw3SlewRateMin || rate > kHw3SlewRateMax)
        return kHw3SlewRateDefault;
    return rate;
}

// Reads the current raw offset out of `modified`, records it as the active
// target, and (when hw3OffsetSlew is enabled) clamps any decrease to
// kHw3SlewRate*4 raw-units/sec. Returns true if the value was modified.
inline bool dashApplyHw3OffsetSlew(CanFrame &modified, const CanFrame & /*original*/)
{
    uint8_t activeRaw = 0;
    if (!dashReadHw3OffsetRawShared(modified, activeRaw))
        return false;
    // Mux-2 only: 1021 in this codebase uses readMuxID() = data[0] & 0x07.
    // Note we read this BEFORE writing back the offset, since the offset
    // bits live in data[0] high nibble which doesn't overlap the mux id.
    if ((modified.data[0] & 0x07) != 2)
        return false;

    hw3OffsetTargetRaw = activeRaw;
    uint8_t shapedRaw = activeRaw;
    uint32_t now = millis();

    if (hw3OffsetSlew)
    {
        uint8_t last = hw3OffsetLastRaw;
        if (activeRaw < last && hw3OffsetLastSentMs != 0)
        {
            uint32_t rateRawPerSec = static_cast<uint32_t>(dashLoadHw3SlewRate(hw3SlewRate)) * 4;
            uint32_t dt = now - hw3OffsetLastSentMs;
            uint32_t maxDrop = (rateRawPerSec * dt + 500) / 1000;
            uint8_t floorRaw = last > maxDrop ? static_cast<uint8_t>(last - maxDrop) : 0;
            if (activeRaw < floorRaw)
            {
                shapedRaw = floorRaw;
                hw3OffsetSlewCount++;
            }
        }
    }

    hw3OffsetLastRaw = shapedRaw;
    hw3OffsetLastSentMs = now;
    if (shapedRaw == activeRaw)
        return false;

    dashWriteHw3OffsetRawShared(modified, shapedRaw);
    return true;
}
