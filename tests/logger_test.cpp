#include <gtest/gtest.h>
#include <sstream>
#include <regex>
#include "logger.hpp"

class LoggerTest : public ::testing::Test {
protected:
    std::stringstream testStream;
    
    void SetUp() override {
        // Set logger to use our test stream
        Logger::init(testStream);
        
        // Set lowest log level to ensure all messages pass
        Logger::setLogLevel(LogLevel::DEBUG);
        
        // Clear the stream before each test
        testStream.str("");
        testStream.clear();
    }
    
    void TearDown() override {
        Logger::init();
    }
};

// Test log level filtering
TEST_F(LoggerTest, LogLevelFiltering) {
    // Test DEBUG level
    Logger::setLogLevel(LogLevel::DEBUG);
    Logger::debug("Debug message");
    EXPECT_TRUE(testStream.str().find("Debug message") != std::string::npos);
    testStream.str("");
    Logger::info("Info message");
    EXPECT_TRUE(testStream.str().find("Info message") != std::string::npos);
    testStream.str("");
    Logger::error("Error message");
    EXPECT_TRUE(testStream.str().find("Error message") != std::string::npos);
    
    // Test INFO level
    Logger::setLogLevel(LogLevel::INFO);
    testStream.str("");
    Logger::debug("Debug message");
    EXPECT_EQ(testStream.str(), ""); // Should be filtered out
    testStream.str("");
    Logger::info("Info message");
    EXPECT_TRUE(testStream.str().find("Info message") != std::string::npos);
    testStream.str("");
    Logger::error("Error message");
    EXPECT_TRUE(testStream.str().find("Error message") != std::string::npos);
    
    // Test ERROR level
    Logger::setLogLevel(LogLevel::ERROR);
    testStream.str("");
    Logger::debug("Debug message");
    EXPECT_EQ(testStream.str(), ""); /// Should be filtered out
    testStream.str("");
    Logger::info("Info message");
    EXPECT_EQ(testStream.str(), ""); // Should be filtered out
    testStream.str("");
    Logger::error("Error message");
    EXPECT_TRUE(testStream.str().find("Error message") != std::string::npos);
}

// Test logLevelToString through the log output
TEST_F(LoggerTest, LogLevelToString) {
    testStream.str("");
    Logger::info("Test message");
    EXPECT_TRUE(testStream.str().find("[INFO]") != std::string::npos);
    
    testStream.str("");
    Logger::debug("Test message");
    EXPECT_TRUE(testStream.str().find("[DEBUG]") != std::string::npos);
    
    testStream.str("");
    Logger::error("Test message");
    EXPECT_TRUE(testStream.str().find("[ERROR]") != std::string::npos);
}

// Test colorForLevel through the log output
TEST_F(LoggerTest, ColorForLevel) {
    testStream.str("");
    Logger::info("Test message");
    EXPECT_TRUE(testStream.str().find("\033[32m") != std::string::npos);
    
    testStream.str("");
    Logger::debug("Test message");
    EXPECT_TRUE(testStream.str().find("\033[34m") != std::string::npos);
    
    testStream.str("");
    Logger::error("Test message");
    EXPECT_TRUE(testStream.str().find("\033[31m") != std::string::npos);
}

// Test formatting of the log message
TEST_F(LoggerTest, LogMessageFormat) {
    testStream.str("");
    Logger::info("Test message");
    
    std::string output = testStream.str();
    
    // Check basic components are present
    EXPECT_TRUE(output.find("[INFO]") != std::string::npos);
    EXPECT_TRUE(output.find("Test message") != std::string::npos);
    EXPECT_TRUE(output.find(" - ") != std::string::npos); // Format separator
    
    // Check timestamp format with regex (basic check - has day and time)
    std::regex timestamp_pattern("[A-Za-z]+ [A-Za-z]+ [0-9]+ [0-9]+:[0-9]+:[0-9]+ [0-9]+");
    EXPECT_TRUE(std::regex_search(output, timestamp_pattern));
}
