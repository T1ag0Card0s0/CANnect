#pragma once

#include <cstdint>

#define CAN_FRAME_MAX_DATA 8

namespace cannect
{

enum class FrameFormat
{
    Standard,
    Extended
};

enum class FrameType
{
    Data,
    Remote,
    Error,
    Overload
};

struct __attribute__((__packed__)) CanFrame
{
    uint32_t id = 0;
    uint8_t dlc = 0;
    uint8_t data[CAN_FRAME_MAX_DATA] = {};
};

} // namespace cannect
