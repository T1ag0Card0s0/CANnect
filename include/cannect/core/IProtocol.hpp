#pragma once

#include "cannect/core/ICanObserver.hpp"

#include <cstdint>
#include <vector>

namespace cannect
{

class IProtocol : public ICanObserver
{
  public:
    virtual ~IProtocol() = default;
    virtual void send(const std::vector<uint8_t> &data) = 0;
    virtual std::vector<uint8_t> receive() = 0;
};

} // namespace cannect
