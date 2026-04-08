/**
 * @file cants/Types.hpp
 * @brief Defines the wire-level types used by the CAN-TS protocol.
 */

#pragma once

#include <cstdint>

namespace cannect
{

/**
 * @brief High-level CAN-TS message categories encoded in the CAN identifier.
 */
enum class CanTsMessageType : uint8_t
{
    TIMESYNC = 0, ///< Broadcast or point-to-point time synchronization frame.
    UNSOLICITED = 1, ///< Unsolicited telemetry or keep-alive frame.
    TELECOMMAND = 2, ///< Telecommand request or acknowledgement.
    TELEMETRY = 3, ///< Telemetry request or acknowledgement.
    SETBLOCK = 4, ///< Block write transaction frame.
    GETBLOCK = 5 ///< Block read transaction frame.
};

/**
 * @brief Decoded CAN-TS header fields extracted from a CAN identifier.
 */
struct __attribute__((packed)) CanTsHeader
{
    uint8_t to = 0; ///< Destination node address, or `0` for broadcast.
    uint8_t from = 0; ///< Source node address.
    CanTsMessageType type = CanTsMessageType::TIMESYNC; ///< Message category.
    uint16_t command = 0; ///< Type-specific command and control bits.
};

} // namespace cannect
