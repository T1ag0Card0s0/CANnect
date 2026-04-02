#include "Cli.hpp"

#include "Handlers.hpp"
#include "cannect/SocketCanInterface.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cannect;

namespace
{
constexpr int kCanDataSize = CAN_FRAME_MAX_DATA;

struct ArgReader
{
    int argc;
    char **argv;
    int index;
    uint32_t timeoutMs = DEFAULT_TIMEOUT_MS;

    bool hasNext() const { return index < argc; }

    const char *peek() const { return hasNext() ? argv[index] : nullptr; }

    const char *next()
    {
        if (!hasNext())
        {
            throw std::runtime_error("Missing command argument");
        }
        return argv[index++];
    }

    uint8_t nextU8() { return parseU8(next()); }

    void readFrame(uint8_t data[kCanDataSize])
    {
        int pos = 0;
        while (hasNext() && pos < kCanDataSize && peek()[0] != '-')
        {
            data[pos++] = nextU8();
        }
    }

    std::vector<uint8_t> readVectorUntilTimeout()
    {
        std::vector<uint8_t> out;
        while (hasNext() && std::strcmp(peek(), "--timeout") != 0)
        {
            out.push_back(nextU8());
        }
        return out;
    }

    std::vector<uint8_t> readAddress4()
    {
        std::vector<uint8_t> addr(4);
        for (auto &b : addr)
        {
            b = nextU8();
        }
        return addr;
    }

    void readOptionalTimeout()
    {
        if (hasNext() && std::strcmp(peek(), "--timeout") == 0)
        {
            next();
            timeoutMs = parseU32(next());
        }
    }
};

bool executeTelecommand(CanTsProtocol &proto, ArgReader &args)
{
    const uint8_t to = args.nextU8();
    const uint8_t ch = args.nextU8();
    uint8_t data[kCanDataSize]{};
    args.readFrame(data);

    const bool ok = proto.sendTelecommand(to, ch, data);

    std::cout << "TELECOMMAND -> node " << static_cast<int>(to) << " ch=" << static_cast<int>(ch) << " data=[ ";
    printHexBytes(data, kCanDataSize);
    std::cout << " ] : " << (ok ? "OK" : "FAIL") << "\n";

    return ok;
}

bool executeTelemetry(CanTsProtocol &proto, ArgReader &args)
{
    const uint8_t to = args.nextU8();
    const uint8_t ch = args.nextU8();

    const bool ok = proto.requestTelemetry(to, ch);

    std::cout << "TELEMETRY REQUEST -> node " << static_cast<int>(to) << " ch=" << static_cast<int>(ch) << " : "
              << (ok ? "OK" : "FAIL") << "\n";

    if (ok)
    {
        uint8_t data[kCanDataSize]{};
        uint8_t dlc = 0;
        if (proto.getLastTelemetry(data, dlc))
        {
            std::cout << "  response (" << static_cast<int>(dlc) << " bytes): [ ";
            printHexBytes(data, dlc);
            std::cout << " ]\n";
        }
    }

    return ok;
}

bool executeUnsolicited(CanTsProtocol &proto, ArgReader &args)
{
    const uint8_t to = args.nextU8();
    const uint8_t ch = args.nextU8();
    uint8_t data[kCanDataSize]{};
    args.readFrame(data);

    const bool ok = proto.sendUnsolicitedTelemetry(to, ch, data);

    std::cout << "UNSOLICITED TELEMETRY -> node " << static_cast<int>(to) << " ch=" << static_cast<int>(ch)
              << " data=[ ";
    printHexBytes(data, kCanDataSize);
    std::cout << " ] : " << (ok ? "OK" : "FAIL") << "\n";

    return ok;
}

bool executeTimeSync(CanTsProtocol &proto, ArgReader &args)
{
    uint8_t data[kCanDataSize]{};
    args.readFrame(data);

    const bool ok = proto.broadcastTimeSync(data);

    std::cout << "TIMESYNC broadcast data=[ ";
    printHexBytes(data, kCanDataSize);
    std::cout << " ] : " << (ok ? "OK" : "FAIL") << "\n";

    return ok;
}

bool executeSetBlock(CanTsProtocol &proto, ArgReader &args)
{
    const uint8_t to = args.nextU8();
    const auto addr = args.readAddress4();
    const auto data = args.readVectorUntilTimeout();
    args.readOptionalTimeout();

    std::cout << "SETBLOCK -> node " << static_cast<int>(to) << " addr=";
    printAddress(addr);
    std::cout << " size=" << data.size() << " ...\n";

    const bool ok = proto.setBlock(to, addr, data, args.timeoutMs);
    std::cout << "SETBLOCK : " << (ok ? "OK" : "FAIL") << "\n";

    return ok;
}

bool executeGetBlock(CanTsProtocol &proto, ArgReader &args)
{
    const uint8_t to = args.nextU8();
    const auto addr = args.readAddress4();
    args.readOptionalTimeout();

    std::cout << "GETBLOCK -> node " << static_cast<int>(to) << " addr=";
    printAddress(addr);
    std::cout << " ...\n";

    const bool ok = proto.getBlock(to, addr, args.timeoutMs);
    std::cout << "GETBLOCK : " << (ok ? "OK" : "FAIL") << "\n";

    if (ok)
    {
        std::vector<uint8_t> received;
        if (proto.getLastGetBlockData(received))
        {
            std::cout << "  received (" << received.size() << " bytes): [ ";
            printHexBytes(received);
            std::cout << " ]\n";
        }
    }

    return ok;
}

bool executeKeepAlive(CanTsProtocol &proto, ArgReader &args)
{
    const uint8_t ch = args.nextU8();
    uint8_t data[kCanDataSize]{};
    args.readFrame(data);

    const bool ok = proto.sendKeepAlive(ch, data);

    std::cout << "KEEPALIVE ch=" << static_cast<int>(ch) << " data=[ ";
    printHexBytes(data, kCanDataSize);
    std::cout << " ] : " << (ok ? "OK" : "FAIL") << "\n";

    return ok;
}
} // namespace

uint8_t parseU8(const char *s) { return static_cast<uint8_t>(std::stoul(s, nullptr, 0)); }

uint32_t parseU32(const char *s) { return static_cast<uint32_t>(std::stoul(s, nullptr, 0)); }

void printUsage(const char *prog)
{
    std::cerr << "Usage: " << prog << " [options] [--send <command>]\n\n"
              << "Options:\n"
              << "  -i, --iface  <type:config>   CAN interface (e.g. socketcan:vcan0)\n"
              << "  -o, --output <file>          Log output file\n"
              << "  -n, --node   <addr>          Local node address (default: 65)\n\n"
              << "Send commands:\n"
              << "  telecommand  <to> <ch> <b0..b7>\n"
              << "  telemetry    <to> <ch>\n"
              << "  unsolicited  <to> <ch> <b0..b7>\n"
              << "  timesync     <b0..b7>\n"
              << "  setblock     <to> <a0 a1 a2 a3> <b0..bN> [--timeout <ms>]\n"
              << "  getblock     <to> <a0 a1 a2 a3> [--timeout <ms>]\n"
              << "  keepalive    <ch> <b0..b7>\n";
}

Options parseOptions(int argc, char *argv[])
{
    Options opts;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if ((arg == "-i" || arg == "--iface") && i + 1 < argc)
        {
            opts.ifaceSpec = argv[++i];
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc)
        {
            opts.outputFile = argv[++i];
        }
        else if ((arg == "-n" || arg == "--node") && i + 1 < argc)
        {
            opts.localNode = parseU8(argv[++i]);
        }
        else if (arg == "-h" || arg == "--help")
        {
            opts.showHelp = true;
        }
        else if ((arg == "-s" || arg == "--send") && i + 1 < argc)
        {
            opts.hasSendCommand = true;
            opts.sendArgIndex = i + 1;
            break;
        }
        else
        {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    return opts;
}

std::shared_ptr<ICanInterface> createInterface(const std::string &spec)
{
    const auto pos = spec.find(':');
    if (pos == std::string::npos)
    {
        throw std::runtime_error("Invalid interface format. Use type:config");
    }

    const std::string type = spec.substr(0, pos);
    const std::string config = spec.substr(pos + 1);

    if (type == "socketcan")
    {
        return std::make_shared<SocketCanInterface>(config);
    }

    throw std::runtime_error("Unknown interface type: " + type);
}

void executeSendCommand(CanTsProtocol &proto, int argc, char *argv[], int startIndex)
{
    ArgReader args{argc, argv, startIndex};

    if (!args.hasNext())
    {
        throw std::runtime_error("Missing command after --send");
    }

    const std::string command = args.next();
    bool ok = false;

    if (command == "telecommand")
    {
        ok = executeTelecommand(proto, args);
    }
    else if (command == "telemetry")
    {
        ok = executeTelemetry(proto, args);
    }
    else if (command == "unsolicited")
    {
        ok = executeUnsolicited(proto, args);
    }
    else if (command == "timesync")
    {
        ok = executeTimeSync(proto, args);
    }
    else if (command == "setblock")
    {
        ok = executeSetBlock(proto, args);
    }
    else if (command == "getblock")
    {
        ok = executeGetBlock(proto, args);
    }
    else if (command == "keepalive")
    {
        ok = executeKeepAlive(proto, args);
    }
    else
    {
        throw std::runtime_error("Unknown send command: " + command);
    }

    std::cout << (ok ? "(connection open -- press Ctrl+C to exit)\n" : "(send failed -- press Ctrl+C to exit)\n");
}
