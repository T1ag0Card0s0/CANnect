#pragma once

#include "cannect/core/CanFrame.hpp"

namespace cannect
{

  class ICanTransport
  {
  public:
    ICanTransport() = default;
    virtual ~ICanTransport() = default;

    ICanTransport(const ICanTransport &) = delete;
    ICanTransport &operator=(const ICanTransport &) = delete;

    ICanTransport(ICanTransport &&) = default;
    ICanTransport &operator=(ICanTransport &&) = default;

    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool isOpen() const = 0;

    virtual int writeFrame(const CanFrame &frame) = 0;
    virtual int readFrame(CanFrame &frame) = 0;
  };

} // namespace cannect
