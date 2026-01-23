#include "cannect/core/CanDispatcher.hpp"

#include <algorithm>
#include <iostream>
#include <mutex>
#include <shared_mutex>

using namespace cannect;

void CanDispatcher::attach(ICanObserver *observer)
{
    if (observer == nullptr)
    {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(mutex);

    auto it = std::find(observers.begin(), observers.end(), observer);
    if (it == observers.end())
    {
        observers.push_back(observer);
    }
}

void CanDispatcher::detach(ICanObserver *observer)
{
    std::unique_lock<std::shared_mutex> lock(mutex);

    auto it = std::remove(observers.begin(), observers.end(), observer);
    observers.erase(it, observers.end());
}

void CanDispatcher::notify(CanFrame &frame)
{
    std::shared_lock<std::shared_mutex> lock(mutex);
    std::vector<ICanObserver *> observer_copy = observers;
    lock.unlock();

    for (auto *observer : observer_copy)
    {
        if (observer != nullptr)
        {
            try
            {
                observer->update(frame);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Observer update failed: " << e.what() << std::endl;
            }
        }
    }
}