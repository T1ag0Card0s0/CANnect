#pragma once

#include <atomic>
#include <thread>

#include "cannect/core/CanDispatcher.hpp"
#include "cannect/core/ICanObserver.hpp"
#include "cannect/core/ICanTransport.hpp"

namespace cannect
{
    class CanListener
    {
      public:
        CanListener(ICanTransport &canTransport);
        ~CanListener();
        void start();
        void stop();
        bool isRunning();
        void registerObserver(ICanObserver *observer);
        void unregisterObserver(ICanObserver *observer);

      private:
        void runner();

        ICanTransport    &canTransport;
        CanDispatcher     canDispatcher;
        std::thread       listenerThread;
        std::atomic<bool> running;
    };
} // namespace cannect
