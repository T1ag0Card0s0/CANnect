#pragma once

#include "cannect/core/CanSender.hpp"
#include "cannect/core/ICanObserver.hpp"

#include <cstdint>
#include <vector>

namespace cannect
{

  class IProtocol : public ICanObserver
  {
  public:
    virtual ~IProtocol() = default;
    virtual void setSender(CanSender *sender) = 0;
    virtual void send(std::vector<uint8_t> data) = 0;
    virtual std::vector<uint8_t> receive() = 0;
  };

} // namespace cannect
