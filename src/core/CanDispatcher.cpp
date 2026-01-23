#include "cannect/core/CanDispatcher.hpp"

using namespace cannect;

void CanDispatcher::attach(ICanObserver *canObserver)
{
  observers.push_back(canObserver);
}

void CanDispatcher::detach(ICanObserver *canObserver)
{
  observers.remove(canObserver);
}

void CanDispatcher::notify()
{
  std::list<ICanObserver *>::iterator iterator = observers.begin();
  while (iterator != observers.end())
  {
    (*iterator)->update(canFrames);
    ++iterator;
  }
  canFrames.clear();
}

void CanDispatcher::addFrame(CanFrame &canFrame)
{
  canFrames.push_back(canFrame);
}
