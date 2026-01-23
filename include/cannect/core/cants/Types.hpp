#pragma once

#include <cstdint>

#define CANTS_FRAME_MAX_DATA_SIZE 8

namespace cannect
{
namespace cants
{

enum class CanTsMessageType : uint8_t
{
    TIMESYNC = 0,     // 000
    UNSOLICITED = 1,  // 001
    TELECOMMAND = 2,  // 010
    TELEMETRY = 3,    // 011
    SETBLOCK = 4,     // 100
    GETBLOCK = 5      // 101
};

enum class CanTsReqAck : uint8_t
{
    REQ = 0b00,
    ACK = 0b01,
    NACK = 0b10
};

enum class SetBlockFrameType : uint8_t
{
    REQUEST = 0b000,
    TRANSFER = 0b001,
    ACK = 0b010,
    ABORT = 0b011,
    NACK = 0b100,
    STATUS_REQUEST = 0b110,
    REPORT = 0b111
};

enum class GetBlockFrameType : uint8_t
{
    REQUEST = 0b000,
    ACK = 0b010,
    ABORT = 0b011,
    NACK = 0b100,
    START = 0b110,
    TRANSFER = 0b111
};

struct __attribute__((packed)) CanTsHeader
{
    uint8_t to;
    uint8_t from;
    CanTsMessageType type;
    uint16_t command;
};

inline constexpr uint16_t CMD_MASK_10BIT = 0x03FF;

inline constexpr uint8_t cmd_low8(uint16_t cmd)
{
    return static_cast<uint8_t>(cmd & 0xFF);
}

inline constexpr CanTsReqAck tc_tm_reqack(uint16_t cmd)
{
    return static_cast<CanTsReqAck>((cmd >> 8) & 0x3);
}

inline constexpr uint8_t tc_tm_channel(uint16_t cmd)
{
    return static_cast<uint8_t>(cmd & 0xFF);
}

inline constexpr uint8_t sb_gb_type3(uint16_t cmd)
{
    return static_cast<uint8_t>((cmd >> 7) & 0x7);
}

inline constexpr uint8_t sb_gb_index6(uint16_t cmd)
{
    return static_cast<uint8_t>(cmd & 0x3F);
}

inline constexpr bool sb_report_done(uint16_t cmd)
{
    return ((cmd >> 6) & 0x1) != 0;
}

inline constexpr uint16_t make_tc_tm_cmd(CanTsReqAck ra, uint8_t channel)
{
    return (static_cast<uint16_t>(ra) << 8) | channel;
}

inline constexpr uint16_t make_sb_cmd(SetBlockFrameType t, uint8_t idx6, bool doneBit = false)
{
    uint16_t cmd = (static_cast<uint16_t>(t) & 0x7) << 7;
    if (t == SetBlockFrameType::REPORT && doneBit)
    {
        cmd |= (1u << 6);
    }
    cmd |= (idx6 & 0x3F);
    return cmd;
}

inline constexpr uint16_t make_gb_cmd(GetBlockFrameType t, uint8_t idx6)
{
    uint16_t cmd = (static_cast<uint16_t>(t) & 0x7) << 7;
    cmd |= (idx6 & 0x3F);
    return cmd;
}

} // namespace cants
} // namespace cannect
