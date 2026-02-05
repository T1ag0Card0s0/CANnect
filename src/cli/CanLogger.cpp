#include "cannect/cli/CanLogger.hpp"

#include <iostream>

using namespace cannect;

void CanLogger::update(CanFrame &canFrame)
{
  std::cout << canFrame << std::endl;
}
