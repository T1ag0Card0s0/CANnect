/**
 * @file ILogSink.hpp
 * @brief Declares the logging sink abstraction used by Logger.
 */

#pragma once
#include <string>

namespace cannect
{

/**
 * @brief Severity assigned to a log message.
 */
enum class LogLevel
{
    Debug, ///< Verbose diagnostic output.
    Info, ///< Routine informational message.
    Warning, ///< Recoverable problem or unexpected condition.
    Error ///< Error that prevented an operation from succeeding.
};

/**
 * @brief Receives formatted log messages emitted by Logger.
 */
class ILogSink
{
  public:
    /**
     * @brief Destroys the sink.
     */
    virtual ~ILogSink() = default;

    /**
     * @brief Writes one log message to the sink.
     *
     * @param level Severity of the log message.
     * @param message Fully formatted message body.
     */
    virtual void log(LogLevel level, const std::string &message) = 0;
};
} // namespace cannect
