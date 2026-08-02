# ESP32 MPU-6050 Orientation Tracking Firmware

A high-performance embedded system that calculates real-time spatial orientation (Pitch, Roll, and Yaw) using custom I2C drivers and a complementary filter.

## Project Outline

1. **Hardware Setup:** Wire the ESP32 and MPU-6050 IMU sensor via I2C interface.
2. **Calibration & Filtering:** Implement automated power-on calibration and deadband filtering to eliminate stationary gyroscope drift.
3. **Software Implementation:** Parse low-level I2C registers, calculate spatial trigonometry, and stream live pitch, roll, and yaw telemetry over Serial.

---

## Hardware Architecture

### Components

* **Microcontroller:** ESP32
* **IMU Sensor:** MPU-6050 6-Axis Accelerometer and Gyroscope

### Wiring Overview

* **Power:** ESP32 3.3V Pin $\rightarrow$ MPU-6050 VCC. GND $\rightarrow$ GND.
* **I2C Data (SDA):** MPU-6050 SDA $\rightarrow$ ESP32 GPIO 6.
* **I2C Clock (SCL):** MPU-6050 SCL $\rightarrow$ ESP32 GPIO 7.

---

## Software & Signal Processing Plan

* **I2C Register Parsing:** Extract raw Big-Endian accelerometer and gyroscope bytes directly from MPU-6050 registers using low-level Wire library calls. Convert raw values into physical g-force and degrees per second.
* **Sensor Fusion & Calibration:** Run a 1,000-sample calibration routine at startup to calculate resting offsets. Apply an Euler Complementary Filter (96% Gyro, 4% Accel) and a zero-velocity deadband filter to maintain precise orientation tracking without integration drift.
