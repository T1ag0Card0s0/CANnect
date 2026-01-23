#include "cannect/core/CanListener.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace cannect;

CanListener::CanListener(ICanTransport &canTransport) : canTransport(canTransport), running(false) {}

CanListener::~CanListener() { stop(); }

void CanListener::start()
{
    bool expected = false;
    if (!running.compare_exchange_strong(expected, true))
    {
        return;
    }

    if (!canTransport.isOpen())
    {
        running.store(false);
        throw std::runtime_error("Transport must be opened before starting listener");
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

bool CanListener::isRunning() { return running.load(); }

void CanListener::registerObserver(ICanObserver *observer)
{
    if (observer != nullptr)
    {
        canDispatcher.attach(observer);
    }
}

void CanListener::unregisterObserver(ICanObserver *observer) { canDispatcher.detach(observer); }

void CanListener::runner()
{
    int consecutive_errors = 0;
    const int MAX_CONSECUTIVE_ERRORS = 10;

    while (running.load(std::memory_order_acquire))
    {
        CanFrame frame{};
        int result = canTransport.readFrame(frame);

        if (result > 0)
        {
            consecutive_errors = 0;
            canDispatcher.notify(frame);
        }
        else if (result == 0)
        {
            consecutive_errors = 0;
        }
        else
        {
            consecutive_errors++;
            std::cerr << "CAN read error (consecutive: " << consecutive_errors << ")" << std::endl;

            if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS)
            {
                std::cerr << "Too many consecutive errors, stopping listener" << std::endl;
                running.store(false);
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
