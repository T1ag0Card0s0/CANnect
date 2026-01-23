#pragma once

#include <iostream>
#include <string>
namespace cannect
{
  class Logger
  {
  protected:
    Logger() = default;

    static Logger *instance;

  public:
    Logger(Logger &other) = delete;
    void operator=(const Logger &) = delete;
    static Logger *getInstance();
    void log(std::string msg)
    {
      std::cout << msg << std::endl;
    }
  };
} // namespace cannect