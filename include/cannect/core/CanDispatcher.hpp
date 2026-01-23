#pragma once

#include "ICanObserver.hpp"

#include <shared_mutex>
#include <vector>

namespace cannect
{

class CanDispatcher
{
  public:
    CanDispatcher() = default;
    ~CanDispatcher() = default;
    void attach(ICanObserver *observer);
    void detach(ICanObserver *observer);
    void notify(CanFrame &canFrame);

  private:
    std::vector<ICanObserver *> observers;
    mutable std::shared_mutex mutex;
};

} // namespace cannect
