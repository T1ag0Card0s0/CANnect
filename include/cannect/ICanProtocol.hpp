#pragma once

#include "cannect/ICanFrameHandler.hpp"
#include "cannect/ICanFrameTransmiter.hpp"
#include "cannect/Status.hpp"

namespace cannect
{

  class ICanProtocol: public ICanFrameHandler
  {
    public:
      virtual ~ICanProtocol() = default;

      virtual Status setFrameTransmiter(ICanFrameTransmiter &frameTansmiter) = 0;
  };

}
