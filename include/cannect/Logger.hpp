/**
 * @file Logger.hpp
 * @brief Declares the global logger and convenience macros used across CANnect.
 */

#pragma once
#include "ILogSink.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifndef LOG_LEVEL
/**
 * @brief Compile-time log threshold.
 *
 * Use `0` for debug, `1` for info, `2` for warning, and `3` for error-only logging.
 */
#define LOG_LEVEL 0
#endif

/**
 * @brief Evaluates whether a log statement of the given numeric level is enabled.
 */
#define CANNECT_LOG_ENABLED(lvl) ((lvl) >= LOG_LEVEL)

namespace cannect
{

/**
 * @brief Source location attached to a log entry.
 */
struct LogLocation
{
    const char *file; ///< Source file path.
    const char *func; ///< Function name.
    int line; ///< Source line number.
};

/**
 * @brief Thread-safe singleton logger that fans messages out to registered sinks.
 */
class Logger
{
  protected:
    Logger() = default;
    static Logger *singleton;

  public:
    Logger(Logger &) = delete;
    void operator=(const Logger &) = delete;

    /**
     * @brief Returns the process-wide logger instance.
     *
     * @return Pointer to the singleton logger.
     */
    static Logger *instance();

    /**
     * @brief Registers a new output sink.
     *
     * @param sink Sink that will receive future log entries.
     */
    void addSink(std::shared_ptr<ILogSink> sink);

    /**
     * @brief Removes all registered sinks.
     */
    void removeSinks();

    /**
     * @brief Formats a message with source context and forwards it to all sinks.
     *
     * @param level Severity associated with the message.
     * @param message Message body.
     * @param loc Source location captured at the call site.
     */
    void log(LogLevel level, const std::string &message, LogLocation loc);

    /**
     * @brief Logs a debug message without source location metadata.
     *
     * @param msg Message body.
     */
    void debug(const std::string &msg) { log(LogLevel::Debug, msg, {"?", "?", 0}); }

    /**
     * @brief Logs an informational message without source location metadata.
     *
     * @param msg Message body.
     */
    void info(const std::string &msg) { log(LogLevel::Info, msg, {"?", "?", 0}); }

    /**
     * @brief Logs a warning message without source location metadata.
     *
     * @param msg Message body.
     */
    void warning(const std::string &msg) { log(LogLevel::Warning, msg, {"?", "?", 0}); }

    /**
     * @brief Logs an error message without source location metadata.
     *
     * @param msg Message body.
     */
    void error(const std::string &msg) { log(LogLevel::Error, msg, {"?", "?", 0}); }

  private:
    std::vector<std::shared_ptr<ILogSink>> sinks_;
    std::mutex mutex_;
};
} // namespace cannect

/**
 * @brief Captures the current file, function, and line for a log statement.
 */
#define CANNECT_LOC                                                                                                    \
    ::cannect::LogLocation { __FILE__, __func__, __LINE__ }

/**
 * @brief Emits a debug log entry when debug logging is enabled.
 */
#if CANNECT_LOG_ENABLED(0)
#define LOG_DEBUG(msg) ::cannect::Logger::instance()->log(::cannect::LogLevel::Debug, msg, CANNECT_LOC)
#else
#define LOG_DEBUG(msg)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

/**
 * @brief Emits an informational log entry when info logging is enabled.
 */
#if CANNECT_LOG_ENABLED(1)
#define LOG_INFO(msg) ::cannect::Logger::instance()->log(::cannect::LogLevel::Info, msg, CANNECT_LOC)
#else
#define LOG_INFO(msg)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

/**
 * @brief Emits a warning log entry when warning logging is enabled.
 */
#if CANNECT_LOG_ENABLED(2)
#define LOG_WARNING(msg) ::cannect::Logger::instance()->log(::cannect::LogLevel::Warning, msg, CANNECT_LOC)
#else
#define LOG_WARNING(msg)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

/**
 * @brief Emits an error log entry when error logging is enabled.
 */
#if CANNECT_LOG_ENABLED(3)
#define LOG_ERROR(msg) ::cannect::Logger::instance()->log(::cannect::LogLevel::Error, msg, CANNECT_LOC)
#else
#define LOG_ERROR(msg)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif
