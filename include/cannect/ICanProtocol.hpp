#pragma once

#include "cannect/ICanFrameHandler.hpp"
#include "cannect/ICanFrameTransmitter.hpp"
#include "cannect/Status.hpp"

namespace cannect
{

class ICanProtocol : public ICanFrameHandler
{
  public:
    virtual ~ICanProtocol() = default;

    virtual Status setFrameTransmitter(ICanFrameTransmitter &frameTransmitter) = 0;
};

} // namespace cannect
