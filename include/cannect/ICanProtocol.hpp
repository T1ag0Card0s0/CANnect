/**
 * @file ICanProtocol.hpp
 * @brief Declares the base interface for protocols layered on top of CAN frames.
 */

#pragma once

#include "cannect/ICanFrameHandler.hpp"
#include "cannect/ICanFrameTransmitter.hpp"
#include "cannect/Status.hpp"

#include <memory>

namespace cannect
{

/**
 * @brief Receives CAN frames and emits protocol-generated CAN frames.
 */
class ICanProtocol : public ICanFrameHandler
{
  public:
    /**
     * @brief Destroys the protocol instance.
     */
    virtual ~ICanProtocol() = default;

    /**
     * @brief Provides the transmitter used for outbound protocol frames.
     *
     * @param transmitter Transport used to send generated CAN frames.
     * @return Status::SUCCESS when the transmitter is accepted, otherwise an error code.
     */
    virtual Status setFrameTransmitter(std::shared_ptr<ICanFrameTransmitter> transmitter) = 0;
};

} // namespace cannect
