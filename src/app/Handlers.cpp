#include "Handlers.hpp"

#include <iostream>

namespace
{
void printFixedFrame(const char *label, uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA])
{
    std::cout << label << " from node " << static_cast<int>(from) << " ch=" << static_cast<int>(ch) << " data=[ ";
    printHexBytes(data, CAN_FRAME_MAX_DATA);
    std::cout << " ]\n";
}

void printBlockFrame(const char *label, uint8_t from, uint8_t ch, std::vector<uint8_t> &data)
{
    std::cout << label << " from node " << static_cast<int>(from) << " ch=" << static_cast<int>(ch)
              << " size=" << data.size() << " data=[ ";
    printHexBytes(data);
    std::cout << " ]\n";
}
} // namespace

void printHexBytes(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        std::cout << "0x" << std::hex << static_cast<int>(data[i]);
        if (i + 1 < size)
        {
            std::cout << " ";
        }
    }
    std::cout << std::dec;
}

void printHexBytes(const std::vector<uint8_t> &data)
{
    if (!data.empty())
    {
        printHexBytes(data.data(), data.size());
    }
}

void printAddress(const std::vector<uint8_t> &addr)
{
    std::cout << "0x";
    for (int i = static_cast<int>(addr.size()) - 1; i >= 0; --i)
    {
        std::cout << std::hex << static_cast<int>(addr[i]);
    }
    std::cout << std::dec;
}

bool onTelecommand(uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA])
{
    printFixedFrame("TELECOMMAND", from, ch, data);
    return true;
}

bool onTelemetry(uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA])
{
    printFixedFrame("TELEMETRY", from, ch, data);
    return true;
}

void onUnsolicitedTelemetry(uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA])
{
    printFixedFrame("UNSOLICITED", from, ch, data);
}

void onTimeSync(uint8_t from, uint8_t data[CAN_FRAME_MAX_DATA])
{
    std::cout << "TIMESYNC from node " << static_cast<int>(from) << " time=[ ";
    printHexBytes(data, CAN_FRAME_MAX_DATA);
    std::cout << " ]\n";
}

bool onSetBlock(uint8_t from, uint8_t ch, std::vector<uint8_t> &data)
{
    printBlockFrame("SETBLOCK", from, ch, data);
    return true;
}

bool onGetBlock(uint8_t from, uint8_t ch, std::vector<uint8_t> &data)
{
    printBlockFrame("GETBLOCK", from, ch, data);
    return true;
}
