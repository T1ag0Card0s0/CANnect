#pragma once

#include "cannect/CanDispatcher.hpp"
#include "cannect/ICanFrameHandler.hpp"
#include "cannect/ICanInterface.hpp"
#include "cannect/Status.hpp"

#include <memory>
#include <string>

namespace cannect
{

class Cannect
{
  public:
    Cannect() = default;
    ~Cannect();
    Status addInterface(std::shared_ptr<ICanInterface> canInterface);
    Status addHandler(std::string name, std::shared_ptr<ICanFrameHandler> frameHandler);
    Status addFilter(const std::string &interfaceName, std::shared_ptr<IFilter> filter);

    Status waitSignal();
    Status start();
    void stop();

  private:
    CanDispatcher dispatcher;
    bool running = false;
};

} // namespace cannect
