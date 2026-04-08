/**
 * @file ICanInterface.hpp
 * @brief Declares the abstract transport interface used by the dispatcher.
 */

#pragma once

#include "cannect/ICanFrameTransmitter.hpp"
#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

#include <cstdint>
#include <string>

namespace cannect
{

/**
 * @brief Additional interface-specific state values.
 */
enum class CanInterfaceStatus : uint8_t
{
    Timeout, ///< The interface did not produce a frame before a timeout expired.
    NotOpen, ///< The interface has not been opened yet.
    AlreadyOpen ///< The interface is already open.
};

/**
 * @brief Represents a named CAN transport that can send and receive frames.
 */
class ICanInterface : public ICanFrameTransmitter
{
  public:
    /**
     * @brief Destroys the interface implementation.
     */
    virtual ~ICanInterface() = default;

    /**
     * @brief Opens the underlying CAN transport.
     *
     * @return Status::SUCCESS when the interface is ready for I/O, otherwise an error code.
     */
    virtual Status open() = 0;

    /**
     * @brief Closes the underlying CAN transport.
     *
     * @return Status::SUCCESS when the interface is closed, otherwise an error code.
     */
    virtual Status close() = 0;

    /**
     * @brief Receives one CAN frame from the transport.
     *
     * @param canFrame Output frame populated on success.
     * @return Status::SUCCESS when a frame was received, otherwise an error code.
     */
    virtual Status receive(CanFrame &canFrame) = 0;

    /**
     * @brief Returns the logical name used to register the interface.
     *
     * @return Interface name.
     */
    virtual std::string getName() = 0;

    /**
     * @brief Reports whether the transport is currently closed.
     *
     * @return true when the interface is closed, otherwise false.
     */
    virtual bool isClosed() const = 0;
};

} // namespace cannect
