#include "cannect/cli/Logger.hpp"

using namespace cannect;

Logger &Logger::getInstance()
{
    static Logger instance;
    return instance;
}
