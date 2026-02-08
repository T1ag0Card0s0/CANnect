#include "cannect/core/CanListener.hpp"
#include <memory>

using namespace cannect;

CanListener::CanListener(std::shared_ptr<ICanTransport> canTransport) : canTransport(canTransport), running(false)
{
}

CanListener::~CanListener()
{
    stop();
}

void CanListener::start()
{
    bool expected = false;
    if (!running.compare_exchange_strong(expected, true))
    {
        return;
    }
    if (!canTransport->isOpen())
    {
        canTransport->open();
    }
    listenerThread = std::thread(&CanListener::runner, this);
}

void CanListener::stop()
{
    if (!running.exchange(false))
    {
        return;
    }

    if (listenerThread.joinable())
    {
        listenerThread.join();
    }
}

bool CanListener::isRunning()
{
    return running.load();
}

void CanListener::addObserver(std::shared_ptr<ICanObserver> canObserver)
{
    canDispatcher.attach(canObserver);
}

void CanListener::runner()
{
    while (running.load())
    {
        CanFrame frame{};
        if (canTransport->readFrame(frame) > 0)
        {
            canDispatcher.notify(frame);
        }
    }
}
