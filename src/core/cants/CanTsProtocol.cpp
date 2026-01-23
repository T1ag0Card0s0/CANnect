#include "cannect/core/CANTSProtocol.hpp"

CanTsProtocol::CANTSProtocol() : socket(std::make_unique<SocketCanTransport>())
{
}

bool CanTsProtocol::connect(const std::string &address)
{
}

bool CanTsProtocol::disconnect()
{
}

int CanTsProtocol::send(const uint8_t *data, size_t length)
{
}

int CanTsProtocol::receive(uint8_t *buffer, size_t maxLength)
{
}

bool CanTsProtocol::isConnected() const
{
}
