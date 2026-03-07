#pragma once

#include "cannect/ICanFrameHandler.hpp"
#include "cannect/ICanFrameTransmitter.hpp"
#include "cannect/Status.hpp"

#include <memory>

namespace cannect
{

class ICanProtocol : public ICanFrameHandler
{
  public:
    virtual ~ICanProtocol() = default;
    virtual Status setFrameTransmitter(std::shared_ptr<ICanFrameTransmitter> transmitter) = 0;
};

} // namespace cannect
