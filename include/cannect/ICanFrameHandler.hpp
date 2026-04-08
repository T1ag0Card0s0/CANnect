/**
 * @file ICanFrameHandler.hpp
 * @brief Declares the callback interface for consumers of decoded CAN frames.
 */

#pragma once

#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

namespace cannect
{

/**
 * @brief Receives CAN frames dispatched from an interface or protocol layer.
 */
class ICanFrameHandler
{
  public:
    /**
     * @brief Destroys the frame handler.
     */
    virtual ~ICanFrameHandler() = default;

    /**
     * @brief Processes one received CAN frame.
     *
     * @param canFrame Frame to process.
     * @return Status::SUCCESS on success, or another status code when handling fails.
     */
    virtual Status onFrame(const CanFrame &canFrame) = 0;
};

} // namespace cannect
