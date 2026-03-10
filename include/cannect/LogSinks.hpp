#pragma once
#include "ILogSink.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace cannect
{
class ConsoleSink : public ILogSink
{
  public:
    void log(LogLevel level, const std::string &message) override
    {
        const char *prefix[] = {"[DBG]", "[INF]", "[WRN]", "[ERR]"};
        auto &out = (level >= LogLevel::Warning) ? std::cerr : std::cout;
        out << prefix[static_cast<int>(level)] << " " << message << "\n";
    }
};

class FileSink : public ILogSink
{
  public:
    explicit FileSink(const std::string &path) : file_(path, std::ios::app)
    {
        if (!file_.is_open())
        {
            throw std::runtime_error("FileSink: cannot open " + path);
        }
    }

    void log(LogLevel level, const std::string &message) override
    {
        const char *prefix[] = {"[DBG]", "[INF]", "[WRN]", "[ERR]"};
        file_ << prefix[static_cast<int>(level)] << " " << message << "\n";
        file_.flush();
    }

  private:
    std::ofstream file_;
};

using LogCallback = std::function<void(LogLevel, const std::string &)>;

class CallbackSink : public ILogSink
{
  public:
    explicit CallbackSink(LogCallback cb) : cb_(std::move(cb)) {}

    void log(LogLevel level, const std::string &message) override
    {
        if (cb_)
        {
            cb_(level, message);
        }
    }

  private:
    LogCallback cb_;
};
} // namespace cannect
