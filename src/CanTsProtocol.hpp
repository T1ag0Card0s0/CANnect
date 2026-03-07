#pragma once

#include "cannect/ICanProtocol.hpp"
#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

namespace cannect
{

class CanTsProtocol : public ICanProtocol
{
  public:
    CanTsProtocol() = default;

    Status onFrame(const CanFrame &frame) override;
    Status setFrameTransmitter(std::shared_ptr<ICanFrameTransmitter> transmitter) override;

    Status sendUtm();
    Status sendTelecommand();
    Status sendTelemetry();
    Status sendSetBlock();
    Status sendGetBlock();

  private:
    std::shared_ptr<ICanFrameTransmitter> transmitter;
};

} // namespace cannect
