#pragma once

#include "cannect/CanDispatcher.hpp"
#include "cannect/ICanInterface.hpp"
#include "cannect/ICanFrameHandler.hpp"
#include "cannect/Status.hpp"

#include <memory>

namespace cannect
{

class Cannect
{
  public:
    Cannect() = default;
    ~Cannect() = default;

    Status addHandler(std::shared_ptr<ICanInterface> canInterface, std::shared_ptr<ICanFrameHandler> frameHandler);

    Status run(int argc, char *argv[]);

  private:
    CanDispatcher dispatcher;
};

} // namespace cannect
