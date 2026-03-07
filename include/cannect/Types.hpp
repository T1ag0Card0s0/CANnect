#pragma once

#include <array>
#include <cstdint>

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
    std::array<uint8_t, 8> data = {};
};

} // namespace cannect
