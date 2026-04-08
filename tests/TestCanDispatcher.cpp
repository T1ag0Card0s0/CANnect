#include "cannect/CanDispatcher.hpp"

#include <iostream>
#include <memory>

using namespace cannect;

class TestCanInterface : public ICanInterface
{
  public:
    TestCanInterface() = default;

    Status open() override { return Status::SUCCESS; }

    Status close() override { return Status::SUCCESS; }

    Status receive(CanFrame &canFrame __attribute__((unused))) override { return Status::SUCCESS; }

    Status send(const CanFrame &canFrame __attribute__((unused))) override { return Status::SUCCESS; }

    std::string getName() override { return "TestCanInterface"; }

    bool isClosed() const override { return false; }
};

int main(void)
{
    CanDispatcher dispatcher;
    dispatcher.addInterface(std::make_shared<TestCanInterface>());
    return 0;
}