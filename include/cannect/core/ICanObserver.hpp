#pragma once

#include "CanFrame.hpp"

#include <vector>

namespace cannect
{

  class ICanObserver
  {
  public:
    virtual ~ICanObserver() {};
    virtual void update(std::vector<CanFrame> canFrames) = 0;
  };

} // namespace cannect
