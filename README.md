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
| Doppler Radar Module                           | [SMAKN HB100 (10.525GHz)](https://www.amazon.com/dp/B00FFW4AZ4)             |
| ADC for Analog Radar Signal                    | [MCP3008-I/P 8-Channel ADC](https://www.amazon.com/dp/B0C5774W5S)           |
| Infrared Trigger Sensor                        | [TCRT5000 Reflective IR](https://www.amazon.com/dp/B00LZV1V10)              |

---

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

### 🧪 macOS (development)

```bash
./scripts/build_mac.sh
./build/launch_monitor
```
