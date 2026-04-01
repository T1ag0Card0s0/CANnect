#pragma once

#include <cstdint>

namespace cannect
{

enum class CanTsMessageType : uint8_t
{
    TIMESYNC = 0,
    UNSOLICITED = 1,
    TELECOMMAND = 2,
    TELEMETRY = 3,
    SETBLOCK = 4,
    GETBLOCK = 5
};

struct __attribute__((packed)) CanTsHeader
{
    uint8_t to = 0;
    uint8_t from = 0;
    CanTsMessageType type = CanTsMessageType::TIMESYNC;
    uint16_t command = 0;
};

} // namespace cannect
