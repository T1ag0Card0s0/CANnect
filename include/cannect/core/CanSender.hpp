#pragma once

#include "CanFrame.hpp"
#include "ICanTransport.hpp"

namespace cannect
{
  class CanSender
  {
  public:
    explicit CanSender(ICanTransport *transport) : transport(transport)
    {
    }

    ~CanSender() = default;

    int sendFrame(const CanFrame &frame);

  private:
    ICanTransport *transport;
  };
} // namespace cannect