#include "logger.hpp"
#include "camera.hpp"
#include "radar.hpp"
#include "trigger.hpp"
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>

// Flag for graceful shutdown
std::atomic<bool> running(true);

std::atomic<int> shotCount(0);
struct ShotData {
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
    float ballSpeedMPH;
    std::string timeString;
};

std::vector<ShotData> shotHistory;

std::string timestampToString(const std::chrono::time_point<std::chrono::steady_clock>& timestamp) {
    // Convert to system time
    auto systemTime = std::chrono::system_clock::now() + 
                     (timestamp - std::chrono::steady_clock::now());
    auto timeT = std::chrono::system_clock::to_time_t(systemTime);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeT), "%H:%M:%S");
    return ss.str();
}

// Display shot data in a formatted way
void displayShotData(const ShotData& shot, int shotNumber) {
    std::string divider = "----------------------------------------";
    
    std::cout << std::endl << divider << std::endl;
    std::cout << "SHOT #" << shotNumber << std::endl;
    std::cout << divider << std::endl;
    std::cout << "Ball Speed: " << std::fixed << std::setprecision(1) << shot.ballSpeedMPH << " mph" << std::endl;
    std::cout << "Time:       " << shot.timeString << std::endl;
    std::cout << divider << std::endl << std::endl;
    Logger::info("Shot #" + std::to_string(shotNumber) + " - Ball speed: " + 
                std::to_string(shot.ballSpeedMPH) + " mph");
}

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    Logger::info("Received signal " + std::to_string(signal) + ", shutting down gracefully...");
    running = false;
}

int main(int argc, char* argv[]) {
    // Register signal handlers
    std::signal(SIGINT, signalHandler);  // Ctrl+C
    std::signal(SIGTERM, signalHandler); // Termination request
    
    // Initialize logging
    Logger::init();
    Logger::setLogLevel(LogLevel::DEBUG);
    Logger::info("Starting DIY Launch Monitor...");
    
    // Check if debug mode is requested
    bool debugMode = false;
    if (argc > 1 && std::string(argv[1]) == "--debug") {
        debugMode = true;
        Logger::info("Running in DEBUG mode without hardware");
    }
    
    // Initialize components
    Logger::info("Initializing components...");
    initCamera();
    RadarManager::getInstance().init();
    
    if (!debugMode) {
        TriggerManager::getInstance().init();
    }
    
    // Register radar callback to store and display measurements
    RadarManager::getInstance().setMeasurementCallback([](const RadarMeasurement& measurement) {
        ShotData shot;
        shot.timestamp = measurement.timestamp;
        shot.ballSpeedMPH = measurement.speedMPH;
        shot.timeString = timestampToString(measurement.timestamp);
        shotHistory.push_back(shot);
        
        int currentShot = ++shotCount;
        displayShotData(shot, currentShot);
    });
    
    if (!debugMode) {
        // Register trigger callback to start radar measurement
        TriggerManager::getInstance().setTriggerCallback([](std::chrono::time_point<std::chrono::steady_clock> timestamp) {
            static auto programStart = std::chrono::steady_clock::now();
            auto millisSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                timestamp - programStart).count();
            
            Logger::info("Ball detected at " + std::to_string(millisSinceStart) + " ms");
            RadarManager::getInstance().startMeasurement();
            
            // TODO: Start camera capture
            // captureFrames();
        });
        
        Logger::info("Components initialized.");
        
        // Main program loop
        while (running) {
            // Update the trigger - this checks the IR sensor
            TriggerManager::getInstance().update();
            
            // Sleep for a small amount to prevent CPU hogging
            // 10ms gives ~100Hz sampling rate which is sufficient for triggering
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } else {
        // In debug mode, just run the test measurements
        Logger::info("Running debug measurements...");
        
    
        // Run a few test measurements
        for (int i = 0; i < 3; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            Logger::info("Debug measurement " + std::to_string(i+1));
            RadarManager::getInstance().startDebugMeasurement();
            
            // Give time for the measurement and display
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // Signal to exit
        running = false;
    }
    
    // Display summary before shutdown
    if (!shotHistory.empty()) {
        std::cout << std::endl << "Session Summary:" << std::endl;
        std::cout << "Total Shots: " << shotHistory.size() << std::endl;
        
        // Calculate average speed
        float totalSpeed = 0.0f;
        float maxSpeed = 0.0f;
        for (const auto& shot : shotHistory) {
            totalSpeed += shot.ballSpeedMPH;
            if (shot.ballSpeedMPH > maxSpeed) {
                maxSpeed = shot.ballSpeedMPH;
            }
        }
        float avgSpeed = totalSpeed / shotHistory.size();
        
        std::cout << "Average Speed: " << std::fixed << std::setprecision(1) << avgSpeed << " mph" << std::endl;
        std::cout << "Max Speed:     " << std::fixed << std::setprecision(1) << maxSpeed << " mph" << std::endl;
        
        Logger::info("Session complete - " + std::to_string(shotHistory.size()) + 
                    " shots, avg: " + std::to_string(avgSpeed) + 
                    " mph, max: " + std::to_string(maxSpeed) + " mph");
    } else {
        Logger::debug("No shots recorded");
    }
    
    // Cleanup
    Logger::info("Cleaning up resources...");
    if (!debugMode) {
        TriggerManager::getInstance().cleanup();
    }
    RadarManager::getInstance().cleanup();
    Logger::info("Shutdown complete.");
    return 0;
}