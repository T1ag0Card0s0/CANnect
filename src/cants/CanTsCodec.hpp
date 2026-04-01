#pragma once

#include "cannect/Types.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#define CAN_TS_MAX_BLOCK_BYTES 512

namespace cannect
{

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

struct CanTsCodec
{
    // --- Header encode / decode ---

    static constexpr uint32_t TO_SHIFT = 21;
    static constexpr uint32_t TYPE_SHIFT = 18;
    static constexpr uint32_t FROM_SHIFT = 10;
    static constexpr uint32_t CMD_SHIFT = 0;

    static constexpr uint32_t TO_MASK = 0xFFu << TO_SHIFT;
    static constexpr uint32_t TYPE_MASK = 0x7u << TYPE_SHIFT;
    static constexpr uint32_t FROM_MASK = 0xFFu << FROM_SHIFT;
    static constexpr uint32_t CMD_MASK = 0x3FFu << CMD_SHIFT;

    static uint32_t encode(const CanTsHeader &h)
    {
        uint32_t id = 0;
        id |= (uint32_t(h.to) << TO_SHIFT) & TO_MASK;
        id |= (uint32_t(h.type) << TYPE_SHIFT) & TYPE_MASK;
        id |= (uint32_t(h.from) << FROM_SHIFT) & FROM_MASK;
        id |= (uint32_t(h.command) << CMD_SHIFT) & CMD_MASK;
        return id;
    }

    static CanTsHeader decode(uint32_t can_id)
    {
        CanTsHeader h{};
        h.to = uint8_t((can_id & TO_MASK) >> TO_SHIFT);
        h.type = static_cast<CanTsMessageType>((can_id & TYPE_MASK) >> TYPE_SHIFT);
        h.from = uint8_t((can_id & FROM_MASK) >> FROM_SHIFT);
        h.command = uint16_t((can_id & CMD_MASK) >> CMD_SHIFT);
        return h;
    }

    // --- TC / TM command fields ---

    static constexpr uint16_t TC_TM_KIND_SHIFT = 8;
    static constexpr uint16_t TC_TM_KIND_MASK = 0x3u << TC_TM_KIND_SHIFT;
    static constexpr uint16_t CHANNEL_MASK = 0xFFu;

    static uint16_t makeTcTmCommand(CanTsReqAck kind, uint8_t channel)
    {
        return (static_cast<uint16_t>(kind) << TC_TM_KIND_SHIFT) | uint16_t(channel);
    }

    static CanTsReqAck getTcTmKind(uint16_t command)
    {
        return static_cast<CanTsReqAck>((command & TC_TM_KIND_MASK) >> TC_TM_KIND_SHIFT);
    }

    static uint8_t getChannel(uint16_t command) { return static_cast<uint8_t>(command & CHANNEL_MASK); }

    // --- Block command fields ---

    static constexpr uint16_t BLOCK_KIND_SHIFT = 7;
    static constexpr uint16_t BLOCK_KIND_MASK = 0x7u << BLOCK_KIND_SHIFT;
    static constexpr uint16_t BLOCK_LOW6_MASK = 0x3Fu;
    static constexpr uint16_t BLOCK_LOW7_MASK = 0x7Fu;
    static constexpr uint16_t SETBLOCK_DONE_MASK = 1u << 6;

    static uint16_t makeSetBlockCommand(SetBlockFrameType type, uint8_t low6)
    {
        return (static_cast<uint16_t>(type) << BLOCK_KIND_SHIFT) | (uint16_t(low6) & BLOCK_LOW6_MASK);
    }

    static uint16_t makeSetBlockReportCommand(uint8_t low6, bool done)
    {
        uint16_t cmd = makeSetBlockCommand(SetBlockFrameType::REPORT, low6);
        if (done)
        {
            cmd |= SETBLOCK_DONE_MASK;
        }
        return cmd;
    }

    static SetBlockFrameType getSetBlockType(uint16_t command)
    {
        return static_cast<SetBlockFrameType>((command & BLOCK_KIND_MASK) >> BLOCK_KIND_SHIFT);
    }

    static uint8_t getSetBlockLow6(uint16_t command) { return static_cast<uint8_t>(command & BLOCK_LOW6_MASK); }

    static bool getSetBlockDone(uint16_t command) { return (command & SETBLOCK_DONE_MASK) != 0; }

    static uint16_t makeGetBlockCommand(GetBlockFrameType type, uint8_t low6)
    {
        return (static_cast<uint16_t>(type) << BLOCK_KIND_SHIFT) | (uint16_t(low6) & BLOCK_LOW6_MASK);
    }

    static GetBlockFrameType getGetBlockType(uint16_t command)
    {
        return static_cast<GetBlockFrameType>((command & BLOCK_KIND_MASK) >> BLOCK_KIND_SHIFT);
    }

    static uint8_t getGetBlockLow6(uint16_t command) { return static_cast<uint8_t>(command & BLOCK_LOW6_MASK); }

    static uint16_t ackCommandLow7(uint16_t command) { return command & BLOCK_LOW7_MASK; }

    // --- Bitmap helpers ---

    static size_t bitmapSizeBytes(uint8_t numBlocks) { return (static_cast<size_t>(numBlocks) + 7u) / 8u; }

    static bool allBlocksReceived(const std::vector<bool> &received)
    {
        return std::all_of(received.begin(), received.end(), [](bool v) { return v; });
    }

    static std::vector<uint8_t> makeBitmap(const std::vector<bool> &received)
    {
        std::vector<uint8_t> bitmap((received.size() + 7u) / 8u, 0u);
        for (size_t i = 0; i < received.size(); ++i)
        {
            if (received[i])
            {
                bitmap[i / 8u] |= static_cast<uint8_t>(1u << (i % 8u));
            }
        }
        return bitmap;
    }

    static bool bitmapBitIsSet(const std::vector<uint8_t> &bitmap, uint8_t index)
    {
        const size_t byteIndex = index / 8u;
        const uint8_t bitIndex = index % 8u;

        if (byteIndex >= bitmap.size())
        {
            return false;
        }

        return (bitmap[byteIndex] & static_cast<uint8_t>(1u << bitIndex)) != 0;
    }
};

} // namespace cannect
