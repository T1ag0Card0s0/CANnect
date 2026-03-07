#pragma once

#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

namespace cannect
{

  class ICanFrameHandler
  {
    public:
      virtual ~ICanFrameHandler() = default;
      virtual Status onFrame(const CanFrame canFrame) = 0;
  };

}
