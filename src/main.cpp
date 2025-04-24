#include "logger.hpp"
#include "camera.hpp"
#include "radar.hpp"
#include "trigger.hpp"
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

// Flag for graceful shutdown
std::atomic<bool> running(true);

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    Logger::info("Received signal " + std::to_string(signal) + ", shutting down gracefully...");
    running = false;
}

int main() {
    // Register signal handlers
    std::signal(SIGINT, signalHandler);  // Ctrl+C
    std::signal(SIGTERM, signalHandler); // Termination request
    
    // Initialize logging
    Logger::init();
    Logger::setLogLevel(LogLevel::DEBUG);
    Logger::info("Starting DIY Launch Monitor...");
    
    // Initialize components
    Logger::info("Initializing components...");
    initCamera();
    initRadar();
    initTrigger();
    Logger::info("Components initialized.");
    
    // Main program loop
    Logger::info("Waiting for ball detection...");
    
    while (running) {
        // Update the trigger - this checks the IR sensor
        TriggerManager::getInstance().update();
        
        // Sleep for a small amount to prevent CPU hogging
        // 10ms gives ~100Hz sampling rate which is sufficient for triggering
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    Logger::info("Cleaning up resources...");
    cleanupTrigger();
    Logger::info("Shutdown complete.");
    return 0;
}