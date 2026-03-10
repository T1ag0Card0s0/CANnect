#include "cannect/Logger.hpp"

#include <filesystem>

namespace cannect
{
Logger *Logger::singleton = nullptr;

Logger *Logger::instance()
{
    if (!singleton)
    {
        singleton = new Logger();
    }
    return singleton;
}

void Logger::addSink(std::shared_ptr<ILogSink> sink)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(std::move(sink));
}

void Logger::removeSinks()
{
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.clear();
}

void Logger::log(LogLevel level, const std::string &message, LogLocation loc)
{
    const std::string file = std::filesystem::path(loc.file).filename().string();

    const std::string full = "[" + file + ":" + std::to_string(loc.line) + " " + loc.func + "] " + message;

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &sink : sinks_)
    {
        sink->log(level, full);
    }
}
} // namespace cannect
