#include <gtest/gtest.h>
#include <sstream>
#include <chrono>
#include <thread>
#include <cmath>
#include "radar.hpp"
#include "logger.hpp"
#include <fftw3.h>

// Test subclass of RadarManager that doesn't rely on actual hardware
class TestRadarManager : public RadarManager {
public:
    TestRadarManager() : RadarManager() {}
    
    // Override init to do nothing (since we have no real hardware)
    void init(int adcChannel = RADAR_ADC_CHANNEL) override {
        this->adcChannel = adcChannel;
        
        // Initialize FFTW without hardware
        if (!fftw_initialized) {
            // Allocate input and output arrays
            fftw_in = (double*)fftw_malloc(sizeof(double) * DEFAULT_SAMPLE_COUNT);
            fftw_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (DEFAULT_SAMPLE_COUNT/2 + 1));
            
            // Create FFTW plan with ESTIMATE to avoid overwriting input data
            fftw_plan_r2c = fftw_plan_dft_r2c_1d(DEFAULT_SAMPLE_COUNT, fftw_in, fftw_out, FFTW_ESTIMATE);
            
            if (!fftw_plan_r2c) {
                Logger::error("Failed to create FFTW plan");
                fftw_free(fftw_in);
                fftw_free(fftw_out);
                return;
            }
            
            fftw_initialized = true;
        }
        Logger::info("Radar initialized on ADC channel " + std::to_string(adcChannel));
    }
    
    void cleanup() override {
        // Free FFTW resources
        if (fftw_initialized) {
            fftw_destroy_plan(fftw_plan_r2c);
            fftw_free(fftw_in);
            fftw_free(fftw_out);
            fftw_initialized = false;
        }
        Logger::info("Radar resources cleaned up");
    }
    
    // Override readSamples to return synthetic test data instead of reading from hardware
    std::vector<int> readSamples(int numSamples = DEFAULT_SAMPLE_COUNT, 
                                int sampleFreq = DEFAULT_SAMPLE_FREQ) override {
        // Generate a test signal based on the test speed
        std::vector<int> samples(numSamples);
        
        // Convert mph to Doppler frequency
        float speedMPS = testSpeedMPH / 2.23694f; // Convert mph to m/s
        float dopplerFreq = (2.0f * speedMPS * RADAR_FREQ) / SPEED_OF_LIGHT;
        
        Logger::debug("Test generating samples for " + std::to_string(testSpeedMPH) + 
                     " mph, Doppler freq: " + std::to_string(dopplerFreq) + " Hz");
        
        // Generate a sine wave at the Doppler frequency, scaled to ADC range (0-1023)
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / sampleFreq;
            // Base signal (DC offset + sine wave)
            float value = 512 + 400 * sin(2 * M_PI * dopplerFreq * t);
            
            // Add noise
            value += (rand() % 40) - 20;
            
            // Clamp to ADC range
            if (value < 0) value = 0;
            if (value > 1023) value = 1023;
            
            samples[i] = static_cast<int>(value);
        }
        
        return samples;
    }
    
    // Set the speed that will be used for generating test data
    void setTestSpeed(float speedMPH) {
        testSpeedMPH = speedMPH;
    }
    
private:
    float testSpeedMPH = 80.0f; // Default test speed in mph
};

class RadarTest : public ::testing::Test {
protected:
    std::stringstream testStream;
    TestRadarManager testManager;
    bool callbackCalled = false;
    RadarMeasurement lastMeasurement;
    
    void SetUp() override {
        Logger::init(testStream);
        Logger::setLogLevel(LogLevel::DEBUG);
        
        // Clear the stream before each test
        testStream.str("");
        testStream.clear();
        
        // Reset test variables
        callbackCalled = false;
        
        // Initialize the radar manager with a test channel
        testManager.init(0); // Using channel 0 for test
        
        // Set our test callback
        testManager.setMeasurementCallback([this](const RadarMeasurement& measurement) {
            callbackCalled = true;
            lastMeasurement = measurement;
            Logger::debug("Test callback called with speed: " + 
                         std::to_string(measurement.speedMPH) + " mph");
        });
        
        // Seed random number generator for consistent test results
        srand(42);
    }
    
    void TearDown() override {
        testManager.cleanup();
        Logger::init();
    }
};

// Test initialization
TEST_F(RadarTest, Initialization) {
    // Check if initialization was logged
    std::string logOutput = testStream.str();
    EXPECT_TRUE(logOutput.find("Radar initialized") != std::string::npos);
}

// Test FFTW-based measurement with synthetic data
TEST_F(RadarTest, FFTWMeasurementWithSyntheticData) {
    // Set test speed
    float testSpeed = 75.0f;
    testManager.setTestSpeed(testSpeed);
    
    // Start measurement
    testManager.startMeasurement();
    
    // Wait for measurement to complete (since it runs in a separate thread)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check that callback was called
    EXPECT_TRUE(callbackCalled);
    
    // Check that the measured speed is close to the test speed
    // Allow for some error due to FFT resolution and noise
    EXPECT_NEAR(lastMeasurement.speedMPH, testSpeed, 3.0f);
    
    // Log the measured speed
    Logger::debug("Test speed: " + std::to_string(testSpeed) + 
                 " mph, measured: " + std::to_string(lastMeasurement.speedMPH) + 
                 " mph, difference: " + 
                 std::to_string(std::abs(testSpeed - lastMeasurement.speedMPH)) + " mph");
}

// Test with different speeds
TEST_F(RadarTest, DifferentSpeeds) {
    std::vector<float> testSpeeds = {30.0f, 60.0f, 90.0f, 120.0f};
    
    for (float speed : testSpeeds) {
        // Reset callback flag
        callbackCalled = false;
        
        // Set test speed
        testManager.setTestSpeed(speed);
        
        // Start measurement
        testManager.startMeasurement();
        
        // Wait for measurement to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Check that callback was called
        EXPECT_TRUE(callbackCalled);
        
        // Check that the measured speed is close to the test speed
        // Allow for some error due to FFT resolution and noise
        EXPECT_NEAR(lastMeasurement.speedMPH, speed, 3.0f);
        
        Logger::debug("Test speed: " + std::to_string(speed) + 
                     " mph, measured: " + std::to_string(lastMeasurement.speedMPH) + 
                     " mph, difference: " + 
                     std::to_string(std::abs(speed - lastMeasurement.speedMPH)) + " mph");
    }
}

// Test very slow speeds (which can be harder to detect)
TEST_F(RadarTest, SlowSpeeds) {
    std::vector<float> testSpeeds = {5.0f, 10.0f, 15.0f};
    
    for (float speed : testSpeeds) {
        // Reset callback flag
        callbackCalled = false;
        
        // Set test speed
        testManager.setTestSpeed(speed);
        
        // Start measurement
        testManager.startMeasurement();
        
        // Wait for measurement to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Check that callback was called
        EXPECT_TRUE(callbackCalled);
        
        // Check that the measured speed is close to the test speed
        // Allow for some error due to FFT resolution and noise
        EXPECT_NEAR(lastMeasurement.speedMPH, speed, 3.0f);
        
        Logger::debug("Test speed: " + std::to_string(speed) + 
                     " mph, measured: " + std::to_string(lastMeasurement.speedMPH) + 
                     " mph, difference: " + 
                     std::to_string(std::abs(speed - lastMeasurement.speedMPH)) + " mph");
    }
}

// Test diagnostic output
TEST_F(RadarTest, DiagnosticOutput) {
    // Set a known speed
    float testSpeed = 50.0f;
    testManager.setTestSpeed(testSpeed);
    
    // Get samples directly
    std::vector<int> samples = testManager.readSamples();
    
    // Process the samples
    RadarMeasurement measurement = testManager.processSamples(samples);
    
    // The processSamples method should correctly identify the dominant frequency
    // and convert it to a speed that matches our test speed
    EXPECT_NEAR(measurement.speedMPH, testSpeed, 3.0f);
    
    // Signal strength should be non-zero
    EXPECT_GT(measurement.signalStrength, 0.0f);
    
    // Check if diagnostics were logged
    std::string logOutput = testStream.str();
    
    // Should contain diagnostic information about frequency
    EXPECT_TRUE(logOutput.find("Dominant frequency") != std::string::npos);
    
    // Should contain speed calculation
    EXPECT_TRUE(logOutput.find("Speed calculation") != std::string::npos);
}

