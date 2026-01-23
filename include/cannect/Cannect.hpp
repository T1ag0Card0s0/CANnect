#pragma once

#include "cannect/cli/ArgumentParser.hpp"
#include "cannect/core/CanListener.hpp"
#include "cannect/core/ICanObserver.hpp"
#include "cannect/core/ICanTransport.hpp"

#include <memory>
#include <vector>

namespace cannect
{

class Cannect
{
  public:
    Cannect();
    ~Cannect();

    int run(int argc, char **argv);

  private:
    ArgumentParser argumentParser;

    std::unique_ptr<ICanTransport> socket;
    std::unique_ptr<CanListener> listener;
    std::vector<std::unique_ptr<ICanObserver>> observers;
};

} // namespace cannect
