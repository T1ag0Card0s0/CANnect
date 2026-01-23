#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "CanFrame.hpp"
#include "cannect/core/ICanObserver.hpp"
#include "cannect/core/CanSender.hpp"

namespace cannect
{

  class IProtocol: public ICanObserver
  {
  public:
    virtual ~IProtocol() = default;
    virtual void setSender(CanSender *sender) = 0;
    virtual void send(std::vector<uint8_t> data) = 0;
    virtual void onCanFrame(CanFrame canFrame);
    virtual std::vector<uint8_t> receive() = 0;
  };

} // namespace cannect
