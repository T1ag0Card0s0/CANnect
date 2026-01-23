#include "cannect/cli/Logger.hpp"

using namespace cannect;

Logger *Logger::instance = nullptr;

Logger *Logger::getInstance()
{
  if (instance == nullptr)
  {
    instance = new Logger();
  }
  return instance;
}
