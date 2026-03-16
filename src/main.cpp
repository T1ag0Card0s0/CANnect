#include "CanTsProtocol.hpp"
#include "SocketCanInterface.hpp"
#include "cannect/Cannect.hpp"
#include "cannect/LogSinks.hpp"
#include "cannect/Logger.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace cannect;

static void help(const char *prog)
{
    std::cerr << "Usage: " << prog << " [options] [--send <command>]\n\n"
              << "Options:\n"
              << "  -i, --iface  <type:config>   CAN interface (e.g. socketcan:vcan0)\n"
              << "  -o, --output <file>          Log output file\n"
              << "  -n, --node   <addr>          Local node address (default: 65)\n\n"
              << "Send commands (require --send / -s):\n"
              << "  telecommand  <to> <ch> <b0..b7>           Send a telecommand (up to 8 bytes)\n"
              << "  telemetry    <to> <ch>                    Request telemetry; prints received response\n"
              << "  unsolicited  <to> <ch> <b0..b7>           Send unsolicited telemetry\n"
              << "  timesync     <b0..b7>                     Broadcast time sync (8 bytes LE)\n"
              << "  setblock     <to> <a0 a1 a2 a3> <b0..bN>  [--timeout <ms>]\n"
              << "  getblock     <to> <a0 a1 a2 a3>            [--timeout <ms>]\n"
              << "  keepalive    <ch> <b0..b7>                Send keepalive unsolicited frame\n"
              << "Notes:\n"
              << "  After sending, the connection stays open to receive responses.\n"
              << "  Press Ctrl+C to exit.\n\n"
              << "Examples:\n"
              << "  " << prog << " -i socketcan:vcan0 --send telecommand 10 1 0xDE 0xAD 0xBE 0xEF 0 0 0 0\n"
              << "  " << prog << " -i socketcan:vcan0 --send telemetry 10 2\n"
              << "  " << prog << " -i socketcan:vcan0 --send timesync 0x00 0x11 0x22 0x33 0x44 0x55 0x66 0x77\n"
              << "  " << prog << " -i socketcan:vcan0 --send setblock 10 0x34 0x12 0x00 0x00 0xDE 0xAD 0xBE 0xEF\n"
              << "  " << prog << " -i socketcan:vcan0 --send getblock 10 0x34 0x12 0x00 0x00 --timeout 2000\n"
              << "  " << prog << " -i socketcan:vcan0 --send keepalive 0 0x01 0x02 0x03 0x04 0 0 0 0\n"
              << "  " << prog << " -i socketcan:vcan0          (listen / receive mode)\n";
}

static uint8_t u8(const char *s) { return static_cast<uint8_t>(std::stoul(s, nullptr, 0)); }
static uint32_t u32(const char *s) { return static_cast<uint32_t>(std::stoul(s, nullptr, 0)); }

static void printHexBytes(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        std::cout << "0x" << std::hex << static_cast<int>(data[i]) << (i + 1 < len ? " " : "");
    }
    std::cout << std::dec;
}

static void printHexBytes(const std::vector<uint8_t> &v) { printHexBytes(v.data(), v.size()); }

static void runSend(CanTsProtocol &proto, int argc, char *argv[], int idx)
{
    if (idx >= argc)
    {
        std::cerr << "Missing command after --send\n";
        return;
    }

    const std::string cmd = argv[idx++];
    uint32_t timeoutMs = DEFAULT_TIMEOUT_MS;

    auto readBytes = [&](uint8_t *dst, int max) {
        for (int b = 0; b < max && idx < argc && argv[idx][0] != '-'; ++b)
        {
            dst[b] = u8(argv[idx++]);
        }
    };

    auto readVec = [&]() {
        std::vector<uint8_t> v;
        while (idx < argc && std::strcmp(argv[idx], "--timeout") != 0)
        {
            v.push_back(u8(argv[idx++]));
        }
        return v;
    };

    auto consumeTimeout = [&]() {
        if (idx + 1 < argc && std::strcmp(argv[idx], "--timeout") == 0)
        {
            ++idx;
            timeoutMs = u32(argv[idx++]);
        }
    };

    bool ok = false;

    if (cmd == "telecommand")
    {
        uint8_t to = u8(argv[idx++]), ch = u8(argv[idx++]), data[CAN_FRAME_MAX_DATA]{};
        readBytes(data, CAN_FRAME_MAX_DATA);
        ok = proto.sendTelecommand(to, ch, data);
        std::cout << "TELECOMMAND -> node " << (int)to << " ch=" << (int)ch << " data=[ ";
        printHexBytes(data, CAN_FRAME_MAX_DATA);
        std::cout << " ] : " << (ok ? "OK" : "FAIL") << "\n";
    }
    else if (cmd == "telemetry")
    {
        uint8_t to = u8(argv[idx++]), ch = u8(argv[idx++]);
        ok = proto.requestTelemetry(to, ch);
        std::cout << "TELEMETRY REQUEST -> node " << (int)to << " ch=" << (int)ch << " : " << (ok ? "OK" : "FAIL")
                  << "\n";
        if (ok)
        {
            uint8_t data[CAN_FRAME_MAX_DATA]{}, dlc = 0;
            if (proto.getLastTelemetry(data, dlc))
            {
                std::cout << "  response (" << (int)dlc << " bytes): [ ";
                printHexBytes(data, dlc);
                std::cout << " ]\n";
            }
        }
    }
    else if (cmd == "unsolicited")
    {
        uint8_t to = u8(argv[idx++]), ch = u8(argv[idx++]), data[CAN_FRAME_MAX_DATA]{};
        readBytes(data, CAN_FRAME_MAX_DATA);
        ok = proto.sendUnsolicitedTelemetry(to, ch, data);
        std::cout << "UNSOLICITED TELEMETRY -> node " << (int)to << " ch=" << (int)ch << " data=[ ";
        printHexBytes(data, CAN_FRAME_MAX_DATA);
        std::cout << " ] : " << (ok ? "OK" : "FAIL") << "\n";
    }
    else if (cmd == "timesync")
    {
        uint8_t data[CAN_FRAME_MAX_DATA]{};
        readBytes(data, CAN_FRAME_MAX_DATA);
        ok = proto.broadcastTimeSync(data);
        std::cout << "TIMESYNC broadcast data=[ ";
        printHexBytes(data, CAN_FRAME_MAX_DATA);
        std::cout << " ] : " << (ok ? "OK" : "FAIL") << "\n";
    }
    else if (cmd == "setblock")
    {
        uint8_t to = u8(argv[idx++]);
        std::vector<uint8_t> addr(4);
        for (auto &b : addr)
        {
            b = u8(argv[idx++]);
        }
        auto data = readVec();
        consumeTimeout();
        std::cout << "SETBLOCK -> node " << (int)to << " addr=0x";
        for (int i = (int)addr.size() - 1; i >= 0; --i)
        {
            std::cout << std::hex << (int)addr[i];
        }
        std::cout << std::dec << " size=" << data.size() << " ...\n";
        ok = proto.setBlock(to, addr, data, timeoutMs);
        std::cout << "SETBLOCK : " << (ok ? "OK" : "FAIL") << "\n";
    }
    else if (cmd == "getblock")
    {
        uint8_t to = u8(argv[idx++]);
        std::vector<uint8_t> addr(4);
        for (auto &b : addr)
        {
            b = u8(argv[idx++]);
        }
        consumeTimeout();
        std::cout << "GETBLOCK -> node " << (int)to << " addr=0x";
        for (int i = (int)addr.size() - 1; i >= 0; --i)
        {
            std::cout << std::hex << (int)addr[i];
        }
        std::cout << std::dec << " ...\n";
        ok = proto.getBlock(to, addr, timeoutMs);
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
    }
    else if (cmd == "keepalive")
    {
        uint8_t ch = u8(argv[idx++]), data[CAN_FRAME_MAX_DATA]{};
        readBytes(data, CAN_FRAME_MAX_DATA);
        ok = proto.sendKeepAlive(ch, data);
        std::cout << "KEEPALIVE ch=" << (int)ch << " data=[ ";
        printHexBytes(data, CAN_FRAME_MAX_DATA);
        std::cout << " ] : " << (ok ? "OK" : "FAIL") << "\n";
    }
    else
    {
        std::cerr << "Unknown send command: " << cmd << "\n";
        return;
    }

    std::cout << (ok ? "(connection open -- press Ctrl+C to exit)\n" : "(send failed -- press Ctrl+C to exit)\n");
}

int main(int argc, char *argv[])
{
    std::shared_ptr<ICanInterface> iface;
    uint8_t localNode = 65;
    int sendIdx = -1;

    Logger::instance()->addSink(std::make_shared<ConsoleSink>());

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-i" || arg == "--iface") && i + 1 < argc)
        {
            std::string spec = argv[++i];
            auto pos = spec.find(':');
            if (pos == std::string::npos)
            {
                std::cerr << "Invalid interface format. Use type:config (e.g. socketcan:vcan0)\n";
                return 1;
            }
            if (spec.substr(0, pos) == "socketcan")
            {
                iface = std::make_shared<SocketCanInterface>(spec.substr(pos + 1));
            }
            else
            {
                std::cerr << "Unknown interface type: " << spec.substr(0, pos) << "\n";
                return 1;
            }
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc)
        {
            Logger::instance()->addSink(std::make_shared<FileSink>(argv[++i]));
        }
        else if ((arg == "-n" || arg == "--node") && i + 1 < argc)
        {
            localNode = u8(argv[++i]);
        }
        else if ((arg == "-s" || arg == "--send") && i + 1 < argc)
        {
            sendIdx = ++i;
            while (i + 1 < argc && (argv[i + 1][0] != '-' || std::strcmp(argv[i + 1], "--timeout") == 0))
            {
                ++i;
            }
        }
        else if (arg == "-h" || arg == "--help")
        {
            help(argv[0]);
            return 0;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << "\n";
            help(argv[0]);
            return 1;
        }
    }

    if (!iface)
    {
        std::cerr << "No CAN interface specified. Use -i / --iface.\n";
        help(argv[0]);
        return 1;
    }

    Cannect app;
    auto proto = std::make_shared<CanTsProtocol>(localNode);

    proto->setTelecommandHandler([](uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA]) {
        std::cout << "TELECOMMAND from node " << (int)from << " ch=" << (int)ch << " data=[ ";
        printHexBytes(data, CAN_FRAME_MAX_DATA);
        std::cout << " ]\n";
        return true;
    });
    proto->setTelemetryHandler([](uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA]) {
        std::cout << "TELEMETRY from node " << (int)from << " ch=" << (int)ch << " data=[ ";
        printHexBytes(data, CAN_FRAME_MAX_DATA);
        std::cout << " ]\n";
        return true;
    });
    proto->setUnsolicitedTelemetryHandler([](uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA]) {
        std::cout << "UNSOLICITED from node " << (int)from << " ch=" << (int)ch << " data=[ ";
        printHexBytes(data, CAN_FRAME_MAX_DATA);
        std::cout << " ]\n";
    });
    proto->setTimeSyncHandler([](uint8_t from, uint8_t t[CAN_FRAME_MAX_DATA]) {
        std::cout << "TIMESYNC from node " << (int)from << " time=[ ";
        printHexBytes(t, CAN_FRAME_MAX_DATA);
        std::cout << " ]\n";
    });
    proto->setSetBlockHandler([](uint8_t from, uint8_t ch, std::vector<uint8_t> &data) {
        std::cout << "SETBLOCK from node " << (int)from << " ch=" << (int)ch << " size=" << data.size() << " data=[ ";
        printHexBytes(data);
        std::cout << " ]\n";
        return true;
    });
    proto->setGetBlockHandler([](uint8_t from, uint8_t ch, std::vector<uint8_t> &data) {
        std::cout << "GETBLOCK from node " << (int)from << " ch=" << (int)ch << " size=" << data.size() << " data=[ ";
        printHexBytes(data);
        std::cout << " ]\n";
        return true;
    });

    proto->setFrameTransmitter(iface);

    if (app.addInterface(iface) != Status::SUCCESS)
    {
        std::cerr << "Failed to add interface\n";
        return 1;
    }
    if (app.addHandler(iface->getName(), proto) != Status::SUCCESS)
    {
        std::cerr << "Failed to add protocol handler\n";
        return 1;
    }

    if (sendIdx >= 0)
    {
        std::thread([&, sendIdx] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            runSend(*proto, argc, argv, sendIdx);
        }).detach();
    }
    else
    {
        std::cout << "CANnect listening on " << iface->getName() << " as node " << (int)localNode << "\n";
    }

    app.run();
    return 0;
}
