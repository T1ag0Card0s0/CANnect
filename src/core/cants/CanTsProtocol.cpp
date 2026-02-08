#include "cannect/core/cants/CanTsProtocol.hpp"

#include <iostream>

#include "cannect/core/cants/EncoderDecoder.hpp"
#include "cannect/core/cants/Types.hpp"

using namespace cannect;
using namespace cannect::cants;

CanTsProtocol::CanTsProtocol(ICanTransport &transport)
    : sender(transport)
{
}

void CanTsProtocol::update(const CanFrame &frame)
{
    CanTsHeader header = EncoderDecoder::decode(frame.getCanId());

    switch (header.type)
    {
        case CanTsMessageType::TELECOMMAND:
            std::cout << "TELECOMMAND from=" << (int) header.from << " to=" << (int) header.to
                      << " cmd=" << header.command << std::endl;
            break;
        case CanTsMessageType::GETBLOCK:
            std::cout << "GETBLOCK" << std::endl;
            break;
        case CanTsMessageType::SETBLOCK:
            std::cout << "SETBLOCK" << std::endl;
            break;
        case CanTsMessageType::TELEMETRY:
            std::cout << "TELEMETRY" << std::endl;
            break;
        case CanTsMessageType::TIMESYNC:
            std::cout << "TIMESYNC" << std::endl;
            break;
        case CanTsMessageType::UNSOLICITED:
            std::cout << "UNSOLICITED" << std::endl;
            break;
        default:
            std::cout << "Unknown type" << std::endl;
    }
}

void CanTsProtocol::send(std::vector<uint8_t> data)
{
    (void) data;
}

std::vector<uint8_t> CanTsProtocol::receive()
{
    return {};
}
