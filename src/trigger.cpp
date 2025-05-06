#include "trigger.hpp"
#include "logger.hpp"
#include <gpiod.h>
#include <chrono>
#include <thread>
#include <stdexcept>

void TriggerManager::init(int pin) {
    digitalPin = pin;
    Logger::debug("Initializing IR Trigger on GPIO pin " + std::to_string(digitalPin));
    
    try {
        // Open the GPIO chip (Raspberry Pi uses chip "gpiochip0")
        chip = gpiod_chip_open_by_name("gpiochip0");
        if (!chip) {
            throw std::runtime_error("Failed to open GPIO chip");
        }
        
        // Get the GPIO line
        line = gpiod_chip_get_line(chip, digitalPin);
        if (!line) {
            throw std::runtime_error("Failed to get GPIO line");
        }
        
        // Request the line as input
        int ret = gpiod_line_request_input(line, "launch_monitor");
        if (ret < 0) {
            throw std::runtime_error("Failed to request GPIO line as input");
        }
        
        Logger::info("IR Trigger initialized on GPIO pin " + std::to_string(digitalPin));
    } catch (const std::exception& e) {
        Logger::error("Failed to initialize IR Trigger: " + std::string(e.what()));
        cleanup();
    }
}

void TriggerManager::cleanup() {
    if (line) {
        gpiod_line_release(line);
        line = nullptr;
    }
    
    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }
    Logger::info("IR Trigger resources cleaned up");
}

void TriggerManager::setTriggerCallback(std::function<void(std::chrono::time_point<std::chrono::steady_clock>)> callback) {
    triggerCallback = callback;
}

void TriggerManager::update() {
    auto now = std::chrono::steady_clock::now();
    
    // State machine for the trigger
    switch (state) {
        case TriggerState::IDLE:
            // Check if the sensor is triggered (ball detected)
            if (readDigitalPin()) {
                state = TriggerState::TRIGGERED;
                lastTriggerTime = now;
                
                Logger::debug("IR Trigger activated");
                
                // Call the registered callback if there is one
                if (triggerCallback) {
                    triggerCallback(lastTriggerTime);
                }
            }
            break;
            
        case TriggerState::TRIGGERED:
            // Transition to cooldown state
            state = TriggerState::COOLDOWN;
            break;
            
        case TriggerState::COOLDOWN:
            // Wait for cooldown period to avoid multiple triggers
            if (now - lastTriggerTime >= cooldownPeriod) {
                state = TriggerState::IDLE;
                Logger::debug("IR Trigger cooldown complete");
            }
            break;
    }
}

bool TriggerManager::readDigitalPin() {
    if (!line) {
        Logger::error("Cannot read GPIO: line not initialized");
        return false;
    }
    
    try {
        // Read the value from the GPIO pin (returns 1 when triggered, 0 when not)
        // Note: Depending on your sensor wiring, you might need to invert this value
        int value = gpiod_line_get_value(line);
        if (value < 0) {
            throw std::runtime_error("Failed to get GPIO line value");
        }
        return value == 1;
    } catch (const std::exception& e) {
        Logger::error("Failed to read GPIO: " + std::string(e.what()));
        return false;
    }
}

void TriggerManager::simulateTrigger() {
    auto now = std::chrono::steady_clock::now();
    lastTriggerTime = now;
    state = TriggerState::TRIGGERED;
    
    Logger::debug("IR Trigger manually simulated");
    
    // Call the registered callback if there is one
    if (triggerCallback) {
        triggerCallback(lastTriggerTime);
    }
}
