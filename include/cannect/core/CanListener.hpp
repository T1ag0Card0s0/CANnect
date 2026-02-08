#pragma once

#include "cannect/core/CanDispatcher.hpp"
#include "cannect/core/ICanObserver.hpp"
#include "cannect/core/ICanTransport.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace cannect
{
    class CanListener
    {
      public:
        CanListener(std::shared_ptr<ICanTransport> canTransport);
        ~CanListener();
        void start();
        void stop();
        bool isRunning();
        void addObserver(std::shared_ptr<ICanObserver> canObserver);

      private:
        void runner();

        std::shared_ptr<ICanTransport> canTransport;
        CanDispatcher canDispatcher;
        std::thread listenerThread;
        std::atomic<bool> running;
    };
} // namespace cannect
