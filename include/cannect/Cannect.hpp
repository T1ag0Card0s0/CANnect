#pragma once

#include "cannect/CanDispatcher.hpp"
#include "cannect/ICanInterface.hpp"

namespace cannect
{

class Cannect 
{
  public:
    Cannect() = default;
    ~Cannect() = default;

    Status addHandler(ICanInterface &canInterface, ICanFrameHandler &frameHandler); 

    Status run(int argc, char *argv[]);

  private:
    CanDispatcher dispatcher;
};

}
