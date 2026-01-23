#pragma once

#include "cannect/core/ICanObserver.hpp"
#include "cannect/core/ICanTransport.hpp"
#include "cli/ArgumentParser.hpp"

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
    ICanTransport *socket;
    ICanObserver *observer;
  };

} // namespace cannect