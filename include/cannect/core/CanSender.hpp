#pragma once

#include "CanFrame.hpp"
#include "ICanTransport.hpp"

namespace cannect
{
class CanSender
{
  public:
    explicit CanSender(ICanTransport &transport) : transport(transport) {}

    ~CanSender() = default;

    CanSender(const CanSender &) = delete;
    CanSender &operator=(const CanSender &) = delete;

    int sendFrame(const CanFrame &frame);

  private:
    ICanTransport &transport;
};
} // namespace cannect
