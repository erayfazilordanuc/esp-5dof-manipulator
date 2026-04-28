# ESP32 5-DOF Manipulator

<p align="center">
  <img src="docs/media/full_assembly.jpeg" width="500" alt="Physical Prototype">
</p>

A 5-axis robotic manipulator powered by an ESP32 and PCA9685. This project focuses on non-blocking network communication, kinematic stability, and hardware-level PWM offloading for jitter-free servo control.

## System Design & CAD

<p align="center">
  <img src="https://github.com/user-attachments/assets/7d4763a0-c3e8-4d4e-81aa-59ce54602eef" width="500" alt="Mechanical Assembly (CAD)">
</p>

All structural components are 3D-printed in PETG to minimize flex under load while keeping the overall assembly lightweight for the micro servos.

## Architecture Highlights

* **Asynchronous Web Control:** WebSocket communication and web server operations run asynchronously. The main MCU loop is dedicated entirely to calculating kinematics and servo steps, preventing motor jitter during network traffic.
* **Motion Profiling (`SmartServo`):** Driving servos at maximum default speed causes structural wobble and high current spikes. A custom `SmartServo` C++ class implements independent speed and step-delay parameters for each joint.
* **Hardware PWM Offloading:** To avoid timer conflicts between Wi-Fi interrupts and PWM generation, signal processing is offloaded to an external PCA9685 module via I2C, ensuring a stable 50Hz signal.
* **Soft-Start Homing:** The firmware executes a low-speed homing routine on boot. This safely drives all joints to a known 90-degree position, preventing brownouts from sudden inrush currents.

## Hardware & Electronics

<p align="center">
  <img src="https://github.com/user-attachments/assets/b1373e59-2dc7-416f-b129-d10f7a266556" width="600" alt="Circuit Diagram">
</p>

<p align="center">
  <img src="docs/media/electronics_base.jpeg" width="500" alt="Electronics Integration">
</p>

* **Microcontroller:** ESP32 (WROOM-32)
* **Actuator Driver:** Adafruit PCA9685 (I2C Address: `0x40`)
* **Actuators:** 5x MG90S Metal Gear Micro Servos
* **Power Supply:** XL4015 DC-DC Buck Converter (Stepped down to `5.0V`)
* **Filtering:** 100µF and 1mF decoupling capacitors across the power rails to suppress inductive spikes.

## Getting Started

### Prerequisites
* [PlatformIO](https://platformio.org/) or Arduino IDE
* Required Libraries: `Adafruit PWM Servo Driver Library`, `ESPAsyncWebServer`, `AsyncTCP`

### Installation
1. Clone the repository:
   ```bash
   git clone [https://github.com/erayfazilordanuc/esp-5dof-manipulator.git](https://github.com/erayfazilordanuc/esp-5dof-manipulator.git)
   ```
2. Navigate to the firmware directory:
   ```bash
   cd esp-5dof-manipulator/firmware
   ```
3. Copy `secrets_example.h` to `secrets.h` and configure your Wi-Fi credentials.
4. Build and flash to your ESP32.

## Upcoming Work: ROS 2 Integration

This firmware serves as the low-level hardware interface for a future autonomous setup. Planned updates:
* micro-ROS integration to bridge the ESP32 with a host machine.
* URDF model generation for the physical system.
* MoveIt 2 integration for Inverse Kinematics (IK) and trajectory planning.

## Directory Structure

* `/cad` - Mechanical assembly references, STEP files, and print-ready STLs.
* `/electronics` - Circuit schematics and wiring diagrams.
* `/docs` - Project media, hardware integration photos, and reference sheets.
* `/firmware` - ESP32 source code, custom classes, and environment configurations.
* `/ros2_ws` - Workspace for upcoming ROS 2 packages and launch files.
