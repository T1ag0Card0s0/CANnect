#include "cannect/Cannect.hpp"

#include "cannect/Logger.hpp"

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <thread>

using namespace cannect;

namespace
{
std::atomic<bool> gRunning{false};
std::mutex gMutex;
std::condition_variable gCv;

void signalHandler(int)
{
    gRunning = false;
    gCv.notify_one();
}
} // namespace

Cannect::~Cannect() { stop(); }

Status Cannect::addHandler(std::shared_ptr<ICanInterface> canInterface, std::shared_ptr<ICanFrameHandler> frameHandler)
{
    const std::string name = canInterface->getName();

    if (Status s = dispatcher.addInterface(canInterface); s != Status::SUCCESS)
    {
        return s;
    }

    return dispatcher.addReceiver(name, frameHandler);
}

Status Cannect::addFilter(const std::string &interfaceName, std::shared_ptr<IFilter> filter)
{
    return dispatcher.addFilter(interfaceName, filter);
}

Status Cannect::start()
{
    if (running)
    {
        return Status::INTERFACE_ALREADY_OPEN;
    }

    dispatcher.start();
    running = true;
    LOG_INFO("Cannect started");
    return Status::SUCCESS;
}

void Cannect::stop()
{
    if (!running)
    {
        return;
    }

    dispatcher.stop();
    running = false;
    LOG_INFO("Cannect stopped");
}

Status Cannect::run()
{
    if (Status s = start(); s != Status::SUCCESS)
    {
        return s;
    }

    gRunning = true;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::unique_lock<std::mutex> lock(gMutex);
    gCv.wait(lock, [] { return !gRunning.load(); });

    stop();
    return Status::SUCCESS;
}
