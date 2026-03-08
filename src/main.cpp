#include "SocketCanInterface.hpp"
#include "cannect/CanDispatcher.hpp"
#include "cannect/ICanFrameHandler.hpp"
#include "cannect/LogSinks.hpp"
#include "cannect/Logger.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

using namespace cannect;

std::atomic<bool> running = true;

class PrintFrameHandler : public ICanFrameHandler
{
  public:
    Status onFrame(const CanFrame &frame) override
    {
        std::cout << "Received frame ID: 0x" << std::hex << frame.id << " DLC: " << std::dec << (int)frame.dlc
                  << " Data: ";

        for (int i = 0; i < frame.dlc; i++)
        {
            std::cout << std::hex << (int)frame.data[i] << " ";
        }

        std::cout << std::dec << std::endl;

        return Status::SUCCESS;
    }
};

void signalHandler(int) { running = false; }

int main()
{
    signal(SIGINT, signalHandler);

    Logger::instance()->addSink(std::make_shared<ConsoleSink>());
    LOG_INFO("Starting CAN example");

    CanDispatcher dispatcher;

    auto canInterface = std::make_shared<SocketCanInterface>("vcan0");

    if (dispatcher.addInterface(canInterface) != Status::SUCCESS)
    {
        LOG_ERROR("Failed to add CAN interface");
        return -1;
    }

    auto receiver = std::make_shared<PrintFrameHandler>();

    if (dispatcher.addReceiver("vcan0", receiver) != Status::SUCCESS)
    {
        LOG_ERROR("Failed to add receiver");
        return -1;
    }

    dispatcher.start();
    LOG_INFO("Dispatcher started");

    while (running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("Stopping dispatcher");
    dispatcher.stop();

    return 0;
}
