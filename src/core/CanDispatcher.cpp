#include "cannect/core/CanDispatcher.hpp"
#include <memory>

using namespace cannect;

void CanDispatcher::attach(std::shared_ptr<ICanObserver> canObserver)
{
    if (canObserver == nullptr)
    {
        return;
    }

    observers.push_back(canObserver);
}

void CanDispatcher::detach(std::shared_ptr<ICanObserver> canObserver)
{
    observers.remove(canObserver);
}

void CanDispatcher::notify(CanFrame &canFrame)
{
    for (auto observer : observers)
    {
        if (observer != nullptr)
        {
            observer->update(canFrame);
        }
    }
}
