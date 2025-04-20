#pragma once
#include <string>

enum class LogLevel {
    DEBUG,
    INFO,
    ERROR,
};

class Logger {
public:
    static void setLogLevel(LogLevel level);
    static void info(const std::string& msg);
    static void debug(const std::string& msg);
    static void error(const std::string& msg);
private:
    static LogLevel currentLogLevel;
    static void log(const std::string& msg, LogLevel level);
    static std::string logLevelToString(LogLevel level);
    static std::string colorForLevel(LogLevel level);
};
