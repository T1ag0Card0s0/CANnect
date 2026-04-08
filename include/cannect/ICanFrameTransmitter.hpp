/**
 * @file ICanFrameTransmitter.hpp
 * @brief Declares the interface used to transmit CAN frames.
 */

#pragma once

#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

namespace cannect
{

/**
 * @brief Sends encoded CAN frames to a transport.
 */
class ICanFrameTransmitter
{
  public:
    /**
     * @brief Destroys the transmitter.
     */
    virtual ~ICanFrameTransmitter() = default;

    /**
     * @brief Transmits one CAN frame.
     *
     * @param canFrame Frame to transmit.
     * @return Status::SUCCESS when the frame is queued or sent, otherwise an error code.
     */
    virtual Status send(const CanFrame &canFrame) = 0;
};

} // namespace cannect
