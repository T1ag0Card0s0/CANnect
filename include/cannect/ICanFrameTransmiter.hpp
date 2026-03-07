#pragma once

#include "cannect/Types.hpp"
#include "cannect/Status.hpp"

namespace cannect
{

  class ICanFrameTransmiter
  {
    public:
      virtual ~ICanFrameTransmiter() = default;
      virtual Status send(const CanFrame &canFrame) = 0;
  };

}
