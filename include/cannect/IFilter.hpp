/**
 * @file IFilter.hpp
 * @brief Declares the interface used to accept or reject incoming CAN frames.
 */

#pragma once

#include "cannect/Types.hpp"

namespace cannect
{

/**
 * @brief Evaluates whether a received frame should be forwarded to handlers.
 */
class IFilter
{
  public:
    /**
     * @brief Destroys the filter.
     */
    virtual ~IFilter() = default;

    /**
     * @brief Checks whether a frame passes this filter.
     *
     * @param frame Frame under evaluation.
     * @return true when the frame should be accepted, otherwise false.
     */
    virtual bool isValid(const CanFrame &frame) = 0;
};

} // namespace cannect
