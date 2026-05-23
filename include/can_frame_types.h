#pragma once

#include <cstdint>
#include <cstring>

enum CanBusMask : uint8_t
{
    CAN_BUS_ANY = 0,
    CAN_BUS_CH = 1 << 0,
    CAN_BUS_VEH = 1 << 1,
    CAN_BUS_PARTY = 1 << 2,
};

#ifndef CAN_BUS_DEFAULT
#define CAN_BUS_DEFAULT CAN_BUS_ANY
#endif

struct CanFrame
{
    uint32_t id = 0;
    uint8_t dlc = 8;
    uint8_t data[8] = {};
    uint8_t bus = CAN_BUS_ANY;
};
