#include "cannect/core/CanDispatcher.hpp"

using namespace cannect;

void CanDispatcher::attach(ICanObserver *canObserver)
{
  if (canObserver == nullptr)
  {
    return;
  }

  observers.push_back(canObserver);
}

void CanDispatcher::detach(ICanObserver *canObserver)
{
  observers.remove(canObserver);
}

void CanDispatcher::notify()
{
  for (auto &frame : canFrames)
  {
    for (auto *observer : observers)
    {
      if (observer != nullptr)
      {
        observer->update(frame);
      }
    }
  }
  canFrames.clear();
}

void CanDispatcher::addFrame(CanFrame &canFrame)
{
  canFrames.push_back(canFrame);
}
