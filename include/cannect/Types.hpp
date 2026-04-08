/**
 * @file Types.hpp
 * @brief Defines the basic CAN frame types used throughout CANnect.
 */

#pragma once

#include <cstdint>

/**
 * @brief Maximum payload size, in bytes, for one classic CAN frame.
 */
#define CAN_FRAME_MAX_DATA 8

namespace cannect
{

/**
 * @brief CAN identifier width used by a frame.
 */
enum class FrameFormat
{
    Standard, ///< 11-bit CAN identifier.
    Extended ///< 29-bit CAN identifier.
};

/**
 * @brief Semantic type of a CAN frame.
 */
enum class FrameType
{
    Data, ///< Regular data frame.
    Remote, ///< Remote transmission request frame.
    Error, ///< Error frame reported by the controller.
    Overload ///< Overload frame.
};

/**
 * @brief Portable representation of a CAN frame.
 */
struct __attribute__((__packed__)) CanFrame
{
    uint32_t id = 0; ///< CAN identifier.
    uint8_t dlc = 0; ///< Number of valid bytes in data.
    uint8_t data[CAN_FRAME_MAX_DATA] = {}; ///< Frame payload bytes.
};

} // namespace cannect
