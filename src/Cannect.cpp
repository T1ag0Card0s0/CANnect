#include "cannect/Cannect.hpp"

#include "cannect/cli/CanLogger.hpp"
#include "cannect/core/CanListener.hpp"
#include "cannect/core/SocketCanTransport.hpp"
#include "cannect/core/cants/CanTsProtocol.hpp"

using namespace cannect;

Cannect::Cannect() : argumentParser(TARGET, VERSION), socket(nullptr), observer(nullptr), protocol(nullptr)
{
}

Cannect::~Cannect()
{
}

int Cannect::run(int argc, char **argv)
{
  argumentParser.addArgument("--can-iface", ArgType::STRING, "Can interface name (e.g. can0, vcan0)");
  argumentParser.addArgument("--iface-a", ArgType::STRING, "Source Can interface (for bridge)");
  argumentParser.addArgument("--iface-b", ArgType::STRING, "Destination Can interface (for bridge)");
  argumentParser.addArgument("--protocol", ArgType::STRING, "Protocol: raw | canopen | cants (default: raw)");
  argumentParser.addArgument("--version", ArgType::NONE, "Verbose output (repeatable)");
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
  argumentParser.parse(argc, argv);

  if (argumentParser.has("--can-iface"))
  {
    socket = new SocketCanTransport(argumentParser.getString("--can-iface"));
    if (!socket->isOpen())
    {
      socket->open();
    }
  }

  if (argumentParser.has("--output"))
  {
    observer = new CanLogger();
  }

  if (argumentParser.has("--protocol"))
  {
    protocol = new CanTsProtocol();
  }

  if (socket)
  {
    CanListener listener(socket);
    if (observer)
    {
      listener.addObserver(observer);
    }
    if (protocol)
    {
      listener.addObserver(protocol);
    }
    listener.start();
    while (listener.isRunning())
      ;
    listener.stop();
  }
  return 0;
}
