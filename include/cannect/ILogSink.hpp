#pragma once
#include <string>

namespace cannect
{
enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

class ILogSink
{
  public:
    virtual ~ILogSink() = default;
    virtual void log(LogLevel level, const std::string &message) = 0;
};
} // namespace cannect
