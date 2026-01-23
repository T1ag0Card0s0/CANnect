#pragma once

#include "cannect/core/ICanObserver.hpp"

namespace cannect
{

  class CanLogger : public ICanObserver
  {
  public:
    CanLogger() = default;
    ~CanLogger() = default;
    void update(std::vector<CanFrame> canFrames) override;
  };

} // namespace cannect