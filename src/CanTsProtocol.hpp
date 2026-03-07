#pragma once

#include "cannect/ICanProtocol.hpp"
#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

namespace cannect 
{

class CanTsProtocol: public ICanProtocol
{
  public:
    CanTsProtocol();
    Status onFrame(const CanFrame frame) override;
    Status setFrameTransmiter(ICanFrameTransmiter &frameTansmiter) override;

    Status sendUtm();
    Status sendTelecommand();
    Status sendTelemetry();
    Status sendSetBlock();
    Status sendGetBlock();

  private:

};

}
