#pragma once

#include <vector>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>

// Forward declare FFTW types to avoid including the header in the .hpp file
typedef struct fftw_plan_s *fftw_plan;
typedef double fftw_complex[2];

// Default ADC channel for HB100 radar
constexpr int RADAR_ADC_CHANNEL = 0;
// Default number of samples for FFT
constexpr int DEFAULT_SAMPLE_COUNT = 1024;
// Default sampling frequency in Hz
constexpr int DEFAULT_SAMPLE_FREQ = 10000;

// Structure to hold radar measurement results
struct RadarMeasurement {
    float speedMPS;        // Speed in meters per second
    float speedMPH;        // Speed in miles per hour
    float signalStrength;  // Signal strength (arbitrary units)
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
};

class RadarManager {
public:
    static RadarManager& getInstance() {
        static RadarManager instance;
        return instance;
    }
    virtual void init(int adcChannel = RADAR_ADC_CHANNEL);
    
    virtual void cleanup();

    void setMeasurementCallback(std::function<void(const RadarMeasurement&)> callback);
    
    // Start a measurement (can be called from trigger callback)
    void startMeasurement();

    // Start a debug measurement with synthetic data    
    void startDebugMeasurement();
    
    // Read samples from the ADC
    virtual std::vector<int> readSamples(int numSamples = DEFAULT_SAMPLE_COUNT, 
                                        int sampleFreq = DEFAULT_SAMPLE_FREQ);
    
    // Process samples to extract velocity
    RadarMeasurement processSamples(const std::vector<int>& samples, 
                                   int sampleFreq = DEFAULT_SAMPLE_FREQ);
    
protected:
    RadarManager() = default;
    virtual ~RadarManager() = default;
    
    RadarManager(const RadarManager&) = delete;
    RadarManager& operator=(const RadarManager&) = delete;
    
    float frequencyToSpeed(float frequency);
    
    int adcChannel = RADAR_ADC_CHANNEL;
    std::function<void(const RadarMeasurement&)> measurementCallback;
    
    // Constants for Doppler radar calculations
    const float RADAR_FREQ = 10.525e9;  // HB100 frequency in Hz
    const float SPEED_OF_LIGHT = 299792458.0;  // in m/s
    
    // FFTW resources
    static bool fftw_initialized;
    static double* fftw_in;
    static fftw_complex* fftw_out;
    static fftw_plan fftw_plan_r2c;
    std::mutex fftw_mutex;
    std::atomic<bool> measurement_in_progress{false};
};
