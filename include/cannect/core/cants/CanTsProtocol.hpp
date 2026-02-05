#pragma once

#include "cannect/core/CanSender.hpp"
#include "cannect/core/IProtocol.hpp"
namespace cannect
{
  class CanTsProtocol : public IProtocol
  {
  public:
    CanTsProtocol() = default;
    ~CanTsProtocol() = default;
    void update(CanFrame &canFrame) override;
    void setSender(CanSender *sender) override;
    void send(std::vector<uint8_t> data) override;
    std::vector<uint8_t> receive() override;

  private:
    CanSender *canSender;
  };
} // namespace cannect
