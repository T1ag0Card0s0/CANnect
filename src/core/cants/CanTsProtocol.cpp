#include "cannect/core/cants/CanTsProtocol.hpp"
#include "cannect/core/ICanTransport.hpp"
#include "cannect/core/cants/EncoderDecoder.hpp"
#include "cannect/core/cants/Types.hpp"

#include <iostream>

using namespace cannect;
using namespace cannect::cants;

CanTsProtocol::CanTsProtocol(std::shared_ptr<ICanTransport> canTransport) : canSender(canTransport)
{
}

void CanTsProtocol::update(const CanFrame &canFrame)
{
    CanTsHeader canTsHeader = EncoderDecoder::decode(canFrame.getCanId());
    switch (canTsHeader.type)
    {
    case CanTsMessageType::TELECOMMAND:
        std::cout << "TELECOMMAND" << std::endl;
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
}

std::vector<uint8_t> CanTsProtocol::receive()
{
    return {};
}
