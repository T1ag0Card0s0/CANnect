#pragma once

#include "cannect/core/CanDispatcher.hpp"
#include "cannect/core/ICanObserver.hpp"
#include "cannect/core/ICanTransport.hpp"

#include <atomic>
#include <thread>

namespace cannect
{
  class CanListener
  {
  public:
    CanListener(ICanTransport *socket);
    ~CanListener();
    void start();
    void stop();
    bool isRunning();
    void addObserver(ICanObserver *canObserver);

  private:
    void runner();

    ICanTransport *socket;
    CanDispatcher canDispatcher;
    std::thread listenerThread;
    std::atomic<bool> running;
  };
} // namespace cannect
