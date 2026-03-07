#pragma once

#include "cannect/ICanInterface.hpp"

#include <string>

namespace cannect
{

class SocketCanInterface : public ICanInterface
{
  public:
    SocketCanInterface(std::string name);
    SocketCanInterface(SocketCanInterface &&) = delete;
    SocketCanInterface(const SocketCanInterface &) = delete;
    SocketCanInterface &operator=(SocketCanInterface &&) = delete;
    SocketCanInterface &operator=(const SocketCanInterface &) = delete;
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
