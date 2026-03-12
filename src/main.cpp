#include "CanTsProtocol.hpp"
#include "SocketCanInterface.hpp"
#include "cannect/Cannect.hpp"
#include "cannect/LogSinks.hpp"
#include "cannect/Logger.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

using namespace cannect;

int main()
{
    Logger::instance()->addSink(std::make_shared<ConsoleSink>());
    Logger::instance()->addSink(std::make_shared<FileSink>("log.txt"));

    Cannect app;

    auto iface = std::make_shared<SocketCanInterface>("vcan0");
    auto proto = std::make_shared<CanTsProtocol>(65);
    proto->setSetBlockHandler(
        [](uint8_t from, uint8_t channel __attribute__((unused)), std::vector<uint8_t> &request) -> bool {
            std::cout << "Received SETBLOCK from " << static_cast<int>(from) << ", size=" << request.size() << "\n";

            std::cout << "Data: ";
            for (uint8_t b : request)
            {
                std::cout << "0x" << std::hex << static_cast<int>(b) << " ";
            }
            std::cout << std::dec << "\n"; // restore decimal

            return true;
        });
    proto->setFrameTransmitter(iface);

    if (app.addInterface(iface) != Status::SUCCESS)
    {
        std::cerr << "Failed to add SocketCanInterface\n";
        return 1;
    }

    if (app.addHandler(iface->getName(), proto) != Status::SUCCESS)
    {
        std::cerr << "Failed to add CanTsProtocol handler\n";
        return 1;
    }

    // if (app.start() != Status::SUCCESS)
    // {
    //  std::cerr << "Failed to start Cannect\n";
    //  return 1;
    // }

    // std::cout << "CAN-TS node started at address 65\n";

    // constexpr uint8_t remoteNode = 10;

    // std::vector<uint8_t> addressLE = {0x34, 0x12, 0x00, 0x00};

    // std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF, 0x10, 0x20, 0x30, 0x40,
    // 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0};

    // const bool ok = proto->setBlock(remoteNode, addressLE, payload, 1000);

    // std::cout << "\nSETBLOCK result: " << (ok ? "SUCCESS" : "FAIL") << "\n";
    app.run();
    app.stop();
    return 0;
}
