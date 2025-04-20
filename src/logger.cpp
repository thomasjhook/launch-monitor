#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <ctime>

LogLevel Logger::currentLogLevel = LogLevel::DEBUG;

void Logger::setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

void Logger::info(const std::string& msg) {
    log(msg, LogLevel::INFO);
}

void Logger::debug(const std::string& msg) {
    log(msg, LogLevel::DEBUG);
}

void Logger::error(const std::string& msg) {
    log(msg, LogLevel::ERROR);
}

void Logger::log(const std::string& msg, LogLevel level) {
    if (level < currentLogLevel) {
        return;
    }
    // Get timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::string timeStr = std::string(std::ctime(&t));
    timeStr.pop_back(); // remove newline

    // Output with color and level tag
    std::string color = colorForLevel(level);
    std::string reset = "\033[0m";

    std::cout << color << "[" << logLevelToString(level) << "] "
              << reset << timeStr << " - " << msg  << std::endl;
    
}
std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::colorForLevel(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "\033[32m"; // Green
        case LogLevel::DEBUG: return "\033[34m"; // Blue
        case LogLevel::ERROR: return "\033[31m"; // Red
        default: return "\033[0m"; // Reset
    }
}