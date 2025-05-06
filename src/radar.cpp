#include "radar.hpp"
#include "logger.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <thread>
#include <bcm2835.h>
#include <fftw3.h>

void RadarManager::init(int channel) {
    adcChannel = channel;
    
    Logger::debug("Initializing Radar on ADC channel " + std::to_string(adcChannel));
    
    // Initialize BCM2835 library for SPI communication
    if (!bcm2835_init()) {
        Logger::error("Failed to initialize BCM2835 library");
        return;
    }
    
    // Initialize SPI
    if (!bcm2835_spi_begin()) {
        Logger::error("Failed to initialize SPI");
        bcm2835_close();
        return;
    }
    
    // Configure SPI
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64); // ~4MHz
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    
    // Initialize FFTW plans if not already created
    if (!fftw_initialized) {
        // Allocate input and output arrays
        fftw_in = (double*)fftw_malloc(sizeof(double) * DEFAULT_SAMPLE_COUNT);
        fftw_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (DEFAULT_SAMPLE_COUNT/2 + 1));
        
        // Create FFTW plan
        fftw_plan_r2c = fftw_plan_dft_r2c_1d(DEFAULT_SAMPLE_COUNT, fftw_in, fftw_out, FFTW_MEASURE);
        
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

void RadarManager::cleanup() {
    // Wait for any ongoing measurements to complete
    while (measurement_in_progress.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Use mutex to ensure thread safety during cleanup
    std::lock_guard<std::mutex> lock(fftw_mutex);
    
    // Free FFTW resources if they were initialized
    if (fftw_initialized) {
        if (fftw_plan_r2c) {
            fftw_destroy_plan(fftw_plan_r2c);
            fftw_plan_r2c = nullptr;
        }
        
        if (fftw_in) {
            fftw_free(fftw_in);
            fftw_in = nullptr;
        }
        
        if (fftw_out) {
            fftw_free(fftw_out);
            fftw_out = nullptr;
        }
        
        fftw_initialized = false;
    }
    
    // End SPI communication
    bcm2835_spi_end();
    // Close BCM2835 library
    bcm2835_close();
    
    Logger::info("Radar resources cleaned up");
}

void RadarManager::setMeasurementCallback(std::function<void(const RadarMeasurement&)> callback) {
    measurementCallback = callback;
}

void RadarManager::startMeasurement() {
    Logger::debug("Starting radar measurement");
    measurement_in_progress.store(true);
    
    // Create a separate thread for measurement to avoid blocking the main thread
    std::thread([this] {
        try {
            // Read samples from ADC
            std::vector<int> samples = readSamples();
            
            // Process samples to get velocity
            RadarMeasurement measurement = processSamples(samples);
            if (measurementCallback) {
                measurementCallback(measurement);
            }
        } catch (const std::exception& e) {
            Logger::error("Error in radar measurement: " + std::string(e.what()));
        }
        measurement_in_progress.store(false);
    }).detach();
}

std::vector<int> RadarManager::readSamples(int numSamples, int sampleFreq) {
    Logger::debug("Reading " + std::to_string(numSamples) + " samples at " + 
                 std::to_string(sampleFreq) + " Hz");
    
    std::vector<int> samples(numSamples);
    
    // Calculate delay between samples based on sample frequency
    auto delayMicros = static_cast<unsigned int>(1000000 / sampleFreq);
    
    // Read samples from MCP3008
    for (int i = 0; i < numSamples; i++) {
        // MCP3008 communication protocol
        unsigned char buffer[3];
        buffer[0] = 0x01;                         // Start bit
        buffer[1] = 0x80 | (adcChannel << 4);     // Single-ended, channel select
        buffer[2] = 0x00;                         // Don't care
        
        bcm2835_spi_transfern((char*)buffer, 3);
        
        // Extract 10-bit result
        int value = ((buffer[1] & 0x03) << 8) | buffer[2];
        samples[i] = value;
        
        // Delay for next sample
        bcm2835_delayMicroseconds(delayMicros);
    }
    
    return samples;
}


void RadarManager::startDebugMeasurement() {
    Logger::debug("Starting DEBUG radar measurement with synthetic data");
    
    try {
        // Generate synthetic samples - a sine wave
        std::vector<int> samples(DEFAULT_SAMPLE_COUNT);
        
        // Use a realistic Doppler frequency for a golf ball (85-100 mph)
        // For an HB100 radar (10.525 GHz), a 100 mph golf ball should produce
        // a Doppler shift of approximately 3130 Hz
        float speedMPH = 85.0f;
        float speedMPS = speedMPH / 2.23694f; // Convert mph to m/s
        
        // Calculate Doppler frequency using radar equation
        // f_doppler = 2 * v * f_radar / c
        float dopplerFreq = (2.0f * speedMPS * RADAR_FREQ) / SPEED_OF_LIGHT;
        
        Logger::debug("Debug setup: Speed=" + std::to_string(speedMPH) + 
                     " mph, Expected Doppler frequency=" + std::to_string(dopplerFreq) + " Hz");
        
        // Generate a sine wave at the Doppler frequency, scaled to ADC range (0-1023)
        for (int i = 0; i < DEFAULT_SAMPLE_COUNT; i++) {
            float t = static_cast<float>(i) / DEFAULT_SAMPLE_FREQ;
            // Base signal (DC offset + sine wave)
            float value = 512 + 400 * sin(2 * M_PI * dopplerFreq * t);
            
            // Add some noise
            value += (rand() % 40) - 20;
            
            // Clamp to ADC range
            if (value < 0) value = 0;
            if (value > 1023) value = 1023;
            
            samples[i] = static_cast<int>(value);
        }
        
        Logger::debug("Created synthetic samples with " + std::to_string(samples.size()) + 
                     " points at " + std::to_string(DEFAULT_SAMPLE_FREQ) + " Hz");
        
        // Process samples to get velocity
        RadarMeasurement measurement = processSamples(samples, DEFAULT_SAMPLE_FREQ);
        
        Logger::debug("Measurement processed: " + std::to_string(measurement.speedMPH) + 
                     " mph (expected: " + std::to_string(speedMPH) + " mph)");
        
        // Call the callback with the measurement
        if (measurementCallback) {
            measurementCallback(measurement);
            Logger::debug("Callback executed");
        } else {
            Logger::debug("No callback registered");
        }
    } catch (const std::exception& e) {
        Logger::error("Error in debug radar measurement: " + std::string(e.what()));
    }
}

float RadarManager::frequencyToSpeed(float frequency) {
    // Doppler equation: v = (c * f_doppler) / (2 * f_radar)
    // where:
    // v = speed of the object
    // c = speed of light
    // f_doppler = frequency shift
    // f_radar = radar frequency
    
    return (SPEED_OF_LIGHT * frequency) / (2.0 * RADAR_FREQ);
}

RadarMeasurement RadarManager::processSamples(const std::vector<int>& samples, int sampleFreq) {
    Logger::debug("Processing " + std::to_string(samples.size()) + " samples with diagnostics");
    
    RadarMeasurement result;
    result.timestamp = std::chrono::steady_clock::now();
    
    // Check if we have the correct number of samples for our pre-allocated FFTW plan
    if (samples.size() != DEFAULT_SAMPLE_COUNT) {
        Logger::debug("Sample count mismatch, expected " + std::to_string(DEFAULT_SAMPLE_COUNT) + 
                       " but got " + std::to_string(samples.size()));
        // Use a simple calculation for this case
        result.speedMPS = 0.0;
        result.speedMPH = 0.0;
        result.signalStrength = 0.0;
        return result;
    }
    
    // Use mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(fftw_mutex);
    
    // Check if FFTW is still initialized
    if (!fftw_initialized || !fftw_in || !fftw_out || !fftw_plan_r2c) {
        Logger::error("FFTW resources not available");
        result.speedMPS = 0.0;
        result.speedMPH = 0.0;
        result.signalStrength = 0.0;
        return result;
    }
    
    // Calculate and log the DC offset
    double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
    Logger::debug("DC offset (mean): " + std::to_string(mean));
    
    // Convert integer samples to double and remove DC offset
    for (size_t i = 0; i < samples.size(); i++) {
        fftw_in[i] = static_cast<double>(samples[i]) - mean;
    }
    
    // Apply a window function (Hamming window) to reduce spectral leakage
    for (size_t i = 0; i < samples.size(); i++) {
        double windowCoeff = 0.54 - 0.46 * cos(2.0 * M_PI * i / (samples.size() - 1));
        fftw_in[i] *= windowCoeff;
    }
    
    // Perform FFT using FFTW
    fftw_execute(fftw_plan_r2c);
    
    // Find dominant frequency and log all significant peaks
    double maxMagnitude = 0.0;
    int maxIndex = 0;
    Logger::debug("Significant frequency components:");
    
    // Calculate frequency resolution
    double freqResolution = static_cast<double>(sampleFreq) / samples.size();
    Logger::debug("Frequency resolution: " + std::to_string(freqResolution) + " Hz per bin");
    
    // Find the top 5 peaks
    struct Peak {
        int index;
        double frequency;
        double magnitude;
        double speed;
    };
    std::vector<Peak> peaks;
    
    // Start from index 1 to ignore DC component (0 Hz)
    for (size_t i = 1; i < samples.size() / 2; i++) {
        double real = fftw_out[i][0];
        double imag = fftw_out[i][1];
        double magnitude = sqrt(real*real + imag*imag);
        
        // Track highest peak
        if (magnitude > maxMagnitude) {
            maxMagnitude = magnitude;
            maxIndex = i;
        }
        
        // Track all significant peaks (>10% of max)
        if (peaks.size() < 5 || magnitude > peaks.back().magnitude) {
            double freq = i * freqResolution;
            double speed = frequencyToSpeed(freq) * 2.23694; // mph
            
            // Insert in sorted order
            Peak peak = {static_cast<int>(i), freq, magnitude, speed};
            peaks.push_back(peak);
            
            // Sort by magnitude (largest first)
            std::sort(peaks.begin(), peaks.end(), 
                [](const Peak& a, const Peak& b) { return a.magnitude > b.magnitude; });
            
            // Keep only top 5
            if (peaks.size() > 5) {
                peaks.pop_back();
            }
        }
    }
    
    // Log all significant peaks
    for (const auto& peak : peaks) {
        Logger::debug("Peak at bin " + std::to_string(peak.index) + 
                     ": " + std::to_string(peak.frequency) + " Hz, magnitude " + 
                     std::to_string(peak.magnitude) + ", equals " + 
                     std::to_string(peak.speed) + " mph");
    }
    
    // Convert highest peak to speed 
    double dominantFreq = maxIndex * freqResolution;
    Logger::debug("Dominant frequency: " + std::to_string(dominantFreq) + 
                 " Hz at bin " + std::to_string(maxIndex));
    
    // Convert frequency to speed using Doppler equation
    result.speedMPS = frequencyToSpeed(dominantFreq);
    result.speedMPH = result.speedMPS * 2.23694; // Convert m/s to mph
    
    Logger::debug("Speed calculation: " + std::to_string(dominantFreq) + 
                 " Hz → " + std::to_string(result.speedMPS) + 
                 " m/s → " + std::to_string(result.speedMPH) + " mph");
    
    // Set signal strength (magnitude of the dominant frequency component)
    result.signalStrength = maxMagnitude;
    
    return result;
}

// Initialize static members
bool RadarManager::fftw_initialized = false;
double* RadarManager::fftw_in = nullptr;
fftw_complex* RadarManager::fftw_out = nullptr;
fftw_plan RadarManager::fftw_plan_r2c = nullptr;
