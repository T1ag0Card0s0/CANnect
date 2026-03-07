#pragma once

#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

namespace cannect
{

class ICanFrameTransmitter
{
  public:
    virtual ~ICanFrameTransmitter() = default;
    virtual Status send(const CanFrame &canFrame) = 0;
};

} // namespace cannect
