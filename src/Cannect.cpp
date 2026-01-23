#include "cannect/Cannect.hpp"

#include "cannect/cli/CanLogger.hpp"
#include "cannect/core/SocketCanTransport.hpp"
#include "cannect/core/cants/CanTsProtocol.hpp"

#include <iostream>
#include <pthread.h>
#include <signal.h>

using namespace cannect;

Cannect::Cannect() : argumentParser(TARGET, VERSION), socket(nullptr), listener(nullptr) {}

Cannect::~Cannect()
{
    if (socket && socket->isOpen())
    {
        socket->close();
    }
}

int Cannect::run(int argc, char **argv)
{
    argumentParser.addArgument("--can-iface", ArgType::STRING, "Can interface name (e.g. can0, vcan0)");
    argumentParser.addArgument("--protocol", ArgType::STRING, "Protocol: raw | canopen | cants (default: raw)");
    argumentParser.addArgument("--addr", ArgType::INT, "Local CAN-TS node address (0-255)");
    argumentParser.addArgument("--version", ArgType::NONE, "Version of CANnect application");
    argumentParser.addArgument("--json", ArgType::BOOL, "Machine-readable JSON output");
    argumentParser.addArgument("--output", ArgType::STRING, "Output file (record)");
    argumentParser.addArgument("--decode", ArgType::BOOL, "Decode frames using selected protocol");

    // Keep the rest if you want, but they aren't used yet:
    argumentParser.addArgument("--iface-a", ArgType::STRING, "Source Can interface (for bridge)");
    argumentParser.addArgument("--iface-b", ArgType::STRING, "Destination Can interface (for bridge)");
    argumentParser.addArgument("--input", ArgType::STRING, "Input file (replay)");
    argumentParser.addArgument("--speed", ArgType::FLOAT, "Replay speed factor (1.0 = realtime, 0 = max)");
    argumentParser.addArgument("--loop", ArgType::INT, "Replay loop count (0 = infinite)");
    argumentParser.addArgument("--filter", ArgType::STRING, "Frame filter expression (e.g. id=0x123, id=0x100-0x1FF)");
    argumentParser.addArgument("--id", ArgType::STRING, "Can ID (hex)");
    argumentParser.addArgument("--data", ArgType::STRING, "Can data bytes (e.g. \"DE AD BE EF\")");
    argumentParser.addArgument("--period", ArgType::STRING, "Send period (e.g. 100ms, 1s)");
    argumentParser.addArgument("--count", ArgType::INT, "Number of frames to send");
    argumentParser.addArgument("--listen", ArgType::STRING, "Listen address (server mode, e.g. 0.0.0.0:5555)");
    argumentParser.addArgument("--connect", ArgType::STRING, "Connect address (client mode, e.g. 10.0.0.5:5555)");
    argumentParser.addArgument("--bidir", ArgType::BOOL, "Bidirectional forwarding");

    if (!argumentParser.parse(argc, argv))
    {
        return 1;
    }

    if (!argumentParser.has("--can-iface"))
    {
        std::cerr << "No CAN interface specified" << std::endl;
        return 1;
    }

    socket = std::make_unique<SocketCanTransport>(argumentParser.getString("--can-iface"));
    if (!socket->open())
    {
        std::cerr << "Failed to open CAN interface" << std::endl;
        return 1;
    }

    listener = std::make_unique<CanListener>(*socket);

    if (argumentParser.has("--output"))
    {
        observers.push_back(std::make_unique<CanLogger>());
    }

    std::string protocol = "raw";
    if (argumentParser.has("--protocol"))
    {
        protocol = argumentParser.getString("--protocol");
    }

    if (protocol == "cants")
    {
        int addrInt = 2; 
        if (argumentParser.has("--addr"))
        {
            addrInt = argumentParser.getInt("--addr");
        }
        if (addrInt < 0 || addrInt > 255)
        {
            std::cerr << "Invalid --addr (must be 0..255)\n";
            return 1;
        }
        uint8_t localAddr = static_cast<uint8_t>(addrInt);

        auto proto = std::make_unique<cants::CanTsProtocol>(*socket, localAddr);

        proto->setTelecommandHandler(
            [](uint8_t from, uint8_t channel, const std::vector<uint8_t> &args, std::vector<uint8_t> &resp) -> bool
            {
                std::cout << "TC REQ from=" << (int)from << " ch=" << (int)channel << " len=" << args.size() << "\n";
                resp = args; 
                return true; 
            });

        proto->setTelemetryHandler(
            [](uint8_t from, uint8_t channel, std::vector<uint8_t> &value) -> bool
            {
                std::cout << "TM REQ from=" << (int)from << " ch=" << (int)channel << "\n";
                
                value = {0x11, 0x22, 0x33, 0x44};
                return true; 
            });

        proto->setUnsolicitedTelemetryHandler(
            [](uint8_t from, uint8_t channel, const std::vector<uint8_t> &value)
            {
                std::cout << "UTM from=" << (int)from << " ch=" << (int)channel << " len=" << value.size() << "\n";
            });

        proto->setTimeSyncHandler(
            [](uint8_t from, const std::vector<uint8_t> &timeLE)
            {
                std::cout << "TIMESYNC from=" << (int)from << " len=" << timeLE.size() << "\n";
            });

        observers.push_back(std::move(proto));
    }
    else if (protocol == "raw" || protocol == "canopen")
    {

    }
    else
    {
        std::cerr << "Unknown protocol: " << protocol << std::endl;
        return 1;
    }

    for (auto &observer : observers)
    {
        listener->registerObserver(observer.get());
    }

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);

    pthread_sigmask(SIG_BLOCK, &set, nullptr);

    listener->start();

    int sig = 0;
    sigwait(&set, &sig);

    std::cout << "\nReceived signal shutting down\n";

    listener->stop();
    socket->close();
    return 0;
}
