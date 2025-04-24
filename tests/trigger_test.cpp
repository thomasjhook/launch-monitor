#include <gtest/gtest.h>
#include <sstream>
#include <chrono>
#include <thread>
#include "trigger.hpp"
#include "logger.hpp"
#include <gpiod.h>

// Test subclass of TriggerManager
class TestTriggerManager : public TriggerManager {
public:
    TestTriggerManager() : TriggerManager() {}
    
    // Override readDigitalPin to control test behavior
    bool readDigitalPin() override {
        return mockGpioValue;
    }
    
    // Control mock GPIO value
    void setMockGpioValue(bool value) {
        mockGpioValue = value;
    }
    
    // For testing, we need to make these do nothing (since we have no real GPIO)
    void init(int digitalPin = IR_DIGITAL_PIN) override {
        this->digitalPin = digitalPin;
        Logger::info("IR Trigger initialized on GPIO pin " + std::to_string(digitalPin));
    }
    
    void cleanup() override {
        Logger::info("IR Trigger resources cleaned up");
    }
    
    TriggerState getState() const {
        return state;
    }
    
private:
    bool mockGpioValue = false;
};

class TriggerTest : public ::testing::Test {
protected:
    std::stringstream testStream;
    TestTriggerManager testManager;
    bool callbackCalled = false;
    std::chrono::time_point<std::chrono::steady_clock> callbackTimestamp;
    
    void SetUp() override {
        Logger::init(testStream);
        Logger::setLogLevel(LogLevel::DEBUG);
        
        // Clear the stream before each test
        testStream.str("");
        testStream.clear();
        callbackCalled = false;
        
        // Initialize the trigger manager with a test pin
        testManager.init(25);
        
        // Set our test callback
        testManager.setTriggerCallback([this](std::chrono::time_point<std::chrono::steady_clock> timestamp) {
            callbackCalled = true;
            callbackTimestamp = timestamp;
            Logger::debug("Callback was called!");
        });
    }
    
    void TearDown() override {
        testManager.cleanup();
        Logger::init();
    }
};

// Test initialization
TEST_F(TriggerTest, Initialization) {
    std::string logOutput = testStream.str();
    EXPECT_TRUE(logOutput.find("IR Trigger initialized") != std::string::npos);
}

// Test callback with simulated trigger
TEST_F(TriggerTest, SimulatedTrigger) {
    testManager.simulateTrigger();
    
    // Verify that our callback was called
    EXPECT_TRUE(callbackCalled);
    
    // Check that the timestamp is reasonably recent (within last second)
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - callbackTimestamp).count();
    
    EXPECT_LE(elapsed, 1000); // Should be less than 1000ms old
    
    // State should be TRIGGERED
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerManager::TriggerState::TRIGGERED));
}

// Test GPIO reading and state machine
TEST_F(TriggerTest, GPIOReading) {
    using TriggerState = TriggerManager::TriggerState;
    
    // Start with GPIO low (no trigger) in IDLE state
    testManager.setMockGpioValue(false);
    callbackCalled = false;
    
    // Log state before and after
    Logger::debug("Initial state before update: " + 
                 std::to_string(static_cast<int>(testManager.getState())));
    
    // Update should not trigger callback
    testManager.update();
    
    Logger::debug("After LOW update: " + 
                 std::to_string(static_cast<int>(testManager.getState())));
    EXPECT_FALSE(callbackCalled);
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerState::IDLE));
    
    // Set GPIO high (trigger)
    testManager.setMockGpioValue(true);
    Logger::debug("Set GPIO HIGH, state before update: " + 
                 std::to_string(static_cast<int>(testManager.getState())));
    
    // Update should now trigger callback and change state to TRIGGERED
    testManager.update();
    
    Logger::debug("After HIGH update: " + 
                 std::to_string(static_cast<int>(testManager.getState())));
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerState::TRIGGERED));
    
    // Reset callback flag
    callbackCalled = false;
    
    // Next update should move to COOLDOWN
    testManager.update();
    EXPECT_FALSE(callbackCalled);
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerState::COOLDOWN));
    
    // Wait for cooldown to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    
    // Update should now be back to IDLE state
    testManager.update();
    EXPECT_FALSE(callbackCalled);
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerState::IDLE));
    
    // Now we should be able to trigger again
    // Set GPIO LOW first to simulate falling edge
    testManager.setMockGpioValue(false);
    testManager.update();
    
    // Set GPIO HIGH to simulate rising edge
    testManager.setMockGpioValue(true);
    testManager.update();
    
    // Callback should be called again
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerState::TRIGGERED));
}

// Test state transitions
TEST_F(TriggerTest, StateTransitions) {
    using TriggerState = TriggerManager::TriggerState;
    
    // Start in IDLE
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerState::IDLE));
    
    // Trigger an event
    testManager.simulateTrigger();
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerState::TRIGGERED));
    
    // Update to move to COOLDOWN
    testManager.update();
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerState::COOLDOWN));
    
    // Wait for cooldown
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    
    // Update to move back to IDLE
    testManager.update();
    EXPECT_EQ(static_cast<int>(testManager.getState()), 
              static_cast<int>(TriggerState::IDLE));
}
