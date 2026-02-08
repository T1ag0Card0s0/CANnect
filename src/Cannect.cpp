#include "cannect/Cannect.hpp"

#include <iostream>
#include <pthread.h>
#include <signal.h>

#include "cannect/cli/CanLogger.hpp"
#include "cannect/core/SocketCanTransport.hpp"
#include "cannect/core/cants/CanTsProtocol.hpp"

using namespace cannect;

Cannect::Cannect()
    : argumentParser(TARGET, VERSION)
    , socket(nullptr)
    , listener(nullptr)
{
}

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
    argumentParser.addArgument("--iface-a", ArgType::STRING, "Source Can interface (for bridge)");
    argumentParser.addArgument("--iface-b", ArgType::STRING, "Destination Can interface (for bridge)");
    argumentParser.addArgument("--protocol", ArgType::STRING, "Protocol: raw | canopen | cants (default: raw)");
    argumentParser.addArgument("--version", ArgType::NONE, "Version of CANnect application");
    argumentParser.addArgument("--json", ArgType::BOOL, "Machine-readable JSON output");
    argumentParser.addArgument("--output", ArgType::STRING, "Output file (record)");
    argumentParser.addArgument("--input", ArgType::STRING, "Input file (replay)");
    argumentParser.addArgument("--speed", ArgType::FLOAT, "Replay speed factor (1.0 = realtime, 0 = max)");
    argumentParser.addArgument("--loop", ArgType::INT, "Replay loop count (0 = infinite)");
    argumentParser.addArgument("--filter", ArgType::STRING, "Frame filter expression (e.g. id=0x123, id=0x100-0x1FF)");
    argumentParser.addArgument("--decode", ArgType::BOOL, "Decode frames using selected protocol");
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

    if (argumentParser.has("--can-iface"))
    {
        socket = std::make_unique<SocketCanTransport>(argumentParser.getString("--can-iface"));

        if (!socket->open())
        {
            std::cerr << "Failed to open CAN interface" << std::endl;
            return 1;
        }
    }
    else
    {
        std::cerr << "No CAN interface specified" << std::endl;
        return 1;
    }

    listener = std::make_unique<CanListener>(*socket);

    if (argumentParser.has("--output"))
    {
        observers.push_back(std::make_unique<CanLogger>());
    }

    if (argumentParser.has("--protocol"))
    {
        observers.push_back(std::make_unique<CanTsProtocol>(*socket));
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