# Golf Launch Monitor

A DIY golf launch monitor built using a Raspberry Pi 5, high-speed camera, radar sensor, and infrared trigger system. This project measures golf ball launch speed, launch angle, and spin rate using a combination of computer vision and radar signal processing.

---

## 🎯 Features

- ⛳ High-speed camera tracking for launch angle and spin rate
- 📡 Doppler radar (HB100) + FFT analysis for ball speed
- ⚡ Infrared trigger for precise event timing
- 🐧 Fully compatible with Raspberry Pi 5
---

## 🧱 Hardware Components

| Component                                      | Model / Link                                                                 |
|-----------------------------------------------|------------------------------------------------------------------------------|
| Compute Unit                                   | [Raspberry Pi 5 (8 GB)](https://www.amazon.com/dp/B0CK2FCG1K)                |
| High-Speed Camera                              | [Arducam IMX477 Pi HQ Camera](https://www.amazon.com/dp/B0D95VWCV6)         |
| Doppler Radar Module                           | [CQRobot (10.525GHz)](https://www.amazon.com/dp/B089NKGWQQ)             |
| ADC for Analog Radar Signal                    | [MCP3008-I/P 8-Channel ADC](https://www.amazon.com/dp/B0C5774W5S)           |
| Infrared Trigger Sensor                        | [TCRT5000 Reflective IR](https://www.amazon.com/dp/B00LZV1V10)              |
| Op-Amp for Signal Amplification                | [LM358 Dual Op-Amp](https://www.amazon.com/dp/B0CBKJTDG2?psc=1&smid=A3FX7C4A9P37IQ&ref_=chk_typ_imgToDp)                  |
| Breadboard                                     | [Medium Breadboard](https://www.amazon.com/dp/B07DL13RZH?ref=ppx_yo2ov_dt_b_fed_asin_title)                   |

---
### 🧰 Additional Components
- Resistors (10K, 100K, 1M ohm)
- Capacitors (0.1μF, 1μF, 10μF)

## 🧩 Software Components
- `radar`: Reads analog signal from HB100 radar via MCP3008, applies FFT to extract velocity
- `camera`: Interfaces with the Arducam HQ camera using OpenCV
- `trigger`: Detects ball movement via IR and timestamps the event
- `logger`: Centralized logging utility with support for info/debug/error levels


## 📈 Measurements
- **Ball Speed**: Derived from Doppler radar FFT
- **Launch Angle**: Calculated from camera frame trajectory
- **Spin Rate**: Estimated via frame comparison and motion blur/edge rotation

## 🛠 Build Instructions

### ⚙️ Raspberry Pi Configuration & Dependencies

#### ✅ Step 1: Update & Upgrade the System
```bash
sudo apt update && sudo apt upgrade -y
```
#### ✅ Step 2: Install Required Build Tools and libraries
```bash
sudo apt install -y build-essential cmake git libcamera-dev libcamera-apps libgpiod-dev libfftw3-dev libopencv-dev libgtest-dev
```
#### ✅ Step 3: Enable SPI Interface (for MCP3008)

The MCP3008 analog-to-digital converter communicates with the Raspberry Pi over SPI.

1. Run the configuration tool:
```bash
sudo raspi-config
```
2. Interface Options → SPI → Enable
#### ✅ Step 4: Install BCM2835 Library (for SPI communication)
```bash
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.71.tar.gz
tar zxvf bcm2835-1.71.tar.gz
cd bcm2835-1.71
./configure
make
sudo make install
```
#### ✅ Step 5: Clone and Build the Project
```bash
git clone https://github.com/thomasjhook/launch-monitor.git
cd launch-monitor
./scripts/build.sh
./build/launch_monitor
```

## ⚡️ Testing with Google Test

This project uses Google Test for unit testing. Follow these steps to run the tests:

### Prerequisites

- Google Test framework installed on your system
- CMake 3.10 or higher

### Building and Running Tests

```bash
# Create a build directory
mkdir -p build && cd build

# Generate build files
cmake ..

# Build the project and tests
make

# Run the tests
ctest
```

### Writing New Tests

To add new tests:

1. Create a new test file in the `test/` directory with the naming convention `*_test.cpp`
2. Add the new test file to `test/CMakeLists.txt`
3. Follow the existing test patterns using Google Test macros

Test files should use the TEST() or TEST_F() macros to define test cases, and use
the various EXPECT_* or ASSERT_* macros for verification.
