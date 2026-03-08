#pragma once
#include "ILogSink.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifndef LOG_LEVEL
#define LOG_LEVEL 0
#endif

#define CANNECT_LOG_ENABLED(lvl) ((lvl) >= LOG_LEVEL)

namespace cannect
{
struct LogLocation
{
    const char *file;
    const char *func;
    int line;
};

class Logger
{
  protected:
    Logger() = default;
    static Logger *singleton;

  public:
    Logger(Logger &) = delete;
    void operator=(const Logger &) = delete;

    static Logger *instance();

    void addSink(std::shared_ptr<ILogSink> sink);
    void removeSinks();

    void log(LogLevel level, const std::string &message, LogLocation loc);

    void debug(const std::string &msg) { log(LogLevel::Debug, msg, {"?", "?", 0}); }
    void info(const std::string &msg) { log(LogLevel::Info, msg, {"?", "?", 0}); }
    void warning(const std::string &msg) { log(LogLevel::Warning, msg, {"?", "?", 0}); }
    void error(const std::string &msg) { log(LogLevel::Error, msg, {"?", "?", 0}); }

  private:
    std::vector<std::shared_ptr<ILogSink>> sinks_;
    std::mutex mutex_;
};
} // namespace cannect

#define CANNECT_LOC                                                                                                    \
    ::cannect::LogLocation { __FILE__, __func__, __LINE__ }

#if CANNECT_LOG_ENABLED(0)
#define LOG_DEBUG(msg) ::cannect::Logger::instance()->log(::cannect::LogLevel::Debug, msg, CANNECT_LOC)
#else
#define LOG_DEBUG(msg)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

#if CANNECT_LOG_ENABLED(1)
#define LOG_INFO(msg) ::cannect::Logger::instance()->log(::cannect::LogLevel::Info, msg, CANNECT_LOC)
#else
#define LOG_INFO(msg)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

#if CANNECT_LOG_ENABLED(2)
#define LOG_WARNING(msg) ::cannect::Logger::instance()->log(::cannect::LogLevel::Warning, msg, CANNECT_LOC)
#else
#define LOG_WARNING(msg)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

#if CANNECT_LOG_ENABLED(3)
#define LOG_ERROR(msg) ::cannect::Logger::instance()->log(::cannect::LogLevel::Error, msg, CANNECT_LOC)
#else
#define LOG_ERROR(msg)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif
