#pragma once

#include "cannect/ICanInterface.hpp"

#include <string>

namespace cannect
{

class SocketCanInterface : public ICanInterface
{
  public:
    explicit SocketCanInterface(std::string name);
    ~SocketCanInterface();

    Status open() override;
    Status close() override;
    Status receive(CanFrame &canFrame) override;
    Status send(const CanFrame &canFrame) override;
    std::string getName() override;
    bool isClosed() const override;

  private:
    std::string name;
    bool isOpen;
    int fd;
};

} // namespace cannect
