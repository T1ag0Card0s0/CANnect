#pragma once

#include "cannect/Status.hpp"
#include "cannect/Types.hpp"
#include "cannect/ICanFrameTransmiter.hpp"
#include <cstdint>
#include <string>

namespace cannect
{

enum class CanInterfaceStatus : uint8_t
{
    Timeout,
    NotOpen,
    AlreadyOpen
};

class ICanInterface: public ICanFrameTransmiter
{
  public:
    virtual ~ICanInterface() = default;

    virtual Status open() = 0;
    virtual Status close() = 0;
    virtual Status receive(CanFrame &canFrame) = 0;
    virtual std::string getName() = 0;
    virtual bool isClosed() const = 0;
};

} // namespace cannect
