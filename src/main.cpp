#include "SocketCanInterface.hpp"
#include "cannect/Cannect.hpp"
#include "cannect/ICanFrameHandler.hpp"
#include "cannect/LogSinks.hpp"
#include "cannect/Logger.hpp"

#include <iostream>
#include <memory>

using namespace cannect;

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

int main()
{
    Logger::instance()->addSink(std::make_shared<ConsoleSink>());
    Logger::instance()->addSink(std::make_shared<FileSink>("log.txt"));

    Cannect app;

    if (app.addHandler(std::make_shared<SocketCanInterface>("vcan0"), std::make_shared<PrintFrameHandler>()) !=
        Status::SUCCESS)
    {
        return -1;
    }

    return app.run() == Status::SUCCESS ? 0 : -1;
}
