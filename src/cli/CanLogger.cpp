#include "cannect/cli/CanLogger.hpp"

#include <iostream>

using namespace cannect;

void CanLogger::update(std::vector<CanFrame> canFrames)
{
  for (auto frame : canFrames)
  {
    std::cout << frame << std::endl;
  }
}
