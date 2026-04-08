/**
 * @file LogSinks.hpp
 * @brief Provides ready-to-use log sink implementations.
 */

#pragma once
#include "ILogSink.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace cannect
{

/**
 * @brief Writes log messages to the process console streams.
 *
 * Warning and error messages are routed to `std::cerr`; other levels go to `std::cout`.
 */
class ConsoleSink : public ILogSink
{
  public:
    /**
     * @brief Writes one log line to the console.
     *
     * @param level Severity associated with the message.
     * @param message Formatted message text.
     */
    void log(LogLevel level, const std::string &message) override
    {
        const char *prefix[] = {"[DBG]", "[INF]", "[WRN]", "[ERR]"};
        auto &out = (level >= LogLevel::Warning) ? std::cerr : std::cout;
        out << prefix[static_cast<int>(level)] << " " << message << "\n";
    }
};

/**
 * @brief Appends log messages to a file.
 */
class FileSink : public ILogSink
{
  public:
    /**
     * @brief Opens the output file in append mode.
     *
     * @param path Path to the log file.
     * @throws std::runtime_error If the file cannot be opened.
     */
    explicit FileSink(const std::string &path) : file_(path, std::ios::app)
    {
        if (!file_.is_open())
        {
            throw std::runtime_error("FileSink: cannot open " + path);
        }
    }

    /**
     * @brief Writes one log line to the file and flushes it immediately.
     *
     * @param level Severity associated with the message.
     * @param message Formatted message text.
     */
    void log(LogLevel level, const std::string &message) override
    {
        const char *prefix[] = {"[DBG]", "[INF]", "[WRN]", "[ERR]"};
        file_ << prefix[static_cast<int>(level)] << " " << message << "\n";
        file_.flush();
    }

  private:
    std::ofstream file_;
};

/**
 * @brief Function signature used by CallbackSink.
 */
using LogCallback = std::function<void(LogLevel, const std::string &)>;

/**
 * @brief Forwards log messages to a user-supplied callback.
 */
class CallbackSink : public ILogSink
{
  public:
    /**
     * @brief Stores the callback that will receive future log messages.
     *
     * @param cb Callback invoked for each log entry.
     */
    explicit CallbackSink(LogCallback cb) : cb_(std::move(cb)) {}

    /**
     * @brief Invokes the registered callback when present.
     *
     * @param level Severity associated with the message.
     * @param message Formatted message text.
     */
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
