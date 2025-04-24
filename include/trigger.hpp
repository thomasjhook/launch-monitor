#pragma once
#include <functional>
#include <chrono>
#include <string>
#include <memory>

// Forward declarations for gpiod types. Allows us to use gpiod 
// without including the full header.
struct gpiod_chip;
struct gpiod_line;

// Default pin for TCRT5000 sensor
constexpr int IR_DIGITAL_PIN = 17;

// Function to initialize the trigger system
void initTrigger();

// Function to cleanup resources
void cleanupTrigger();

// Follows the Singleton pattern to manage the trigger system
class TriggerManager {
public:
    static TriggerManager& getInstance() {
        static TriggerManager instance;
        return instance;
    }

    enum class TriggerState {
        IDLE,
        TRIGGERED,
        COOLDOWN
    };

    virtual void init(int digitalPin = IR_DIGITAL_PIN);
    virtual void cleanup();
    
    // Register a callback to be executed when a trigger event occurs
    void setTriggerCallback(std::function<void(std::chrono::time_point<std::chrono::steady_clock>)> callback);
    
    // Update function - call this in the main loop to check for events
    void update();
    
    // For testing - manually trigger
    void simulateTrigger();
    
protected:
    TriggerManager() = default;
    virtual ~TriggerManager() = default;
    // Deleted copy constructor and assignment operator
    TriggerManager(const TriggerManager&) = delete;
    TriggerManager& operator=(const TriggerManager&) = delete;
    
    // GPIO resources
    struct gpiod_chip* chip = nullptr;
    struct gpiod_line* line = nullptr;
    
    int digitalPin = IR_DIGITAL_PIN;
    std::chrono::milliseconds cooldownPeriod{500};
    std::chrono::time_point<std::chrono::steady_clock> lastTriggerTime = std::chrono::steady_clock::now();

    
    TriggerState state = TriggerState::IDLE;
    std::function<void(std::chrono::time_point<std::chrono::steady_clock>)> triggerCallback;
    
    // Helper to read the digital pin
    virtual bool readDigitalPin();
};
