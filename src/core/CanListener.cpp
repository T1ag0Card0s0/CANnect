#include "cannect/core/CanListener.hpp"

using namespace cannect;

CanListener::CanListener(ICanTransport *socket) : socket(socket), running(false)
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
  if (!socket->isOpen())
  {
    socket->open();
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

void CanListener::addObserver(ICanObserver *canObserver)
{
  canDispatcher.attach(canObserver);
}

void CanListener::runner()
{
  while (running.load())
  {
    CanFrame frame{};
    if (socket->readFrame(frame) > 0)
    {
      canDispatcher.addFrame(frame);
      canDispatcher.notify();
    }
  }
}
