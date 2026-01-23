#pragma once

#include "ICanObserver.hpp"

#include <list>
#include <vector>

namespace cannect
{

  class CanDispatcher
  {
  public:
    CanDispatcher() = default;
    ~CanDispatcher() = default;
    void attach(ICanObserver *canObserver);
    void detach(ICanObserver *canObserver);
    void notify();
    void addFrame(CanFrame &canFrame);

  private:
    std::list<ICanObserver *> observers;
    std::vector<CanFrame> canFrames;
  };

} // namespace cannect
