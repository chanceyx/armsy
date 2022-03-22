#include <iostream>

#include "logger.h"
#include "timestamp.h"

Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level)
{
    log_level_ = level;
}

void Logger::log(std::string msg)
{
    switch (log_level_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}