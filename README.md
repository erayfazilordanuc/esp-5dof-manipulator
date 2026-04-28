# ESP32 5-DOF Manipulator

<p align="center">
  <img src="docs/media/full_assembly.jpg" width="500" alt="Physical Prototype">
</p>

A custom-built, 5-axis robotic manipulator powered by an ESP32-C3 and PCA9685. This project focuses on non-blocking network communication, kinematic stability, and hardware-level PWM offloading for jitter-free servo control.

## System Design & CAD

<p align="center">
  <img src="cad/assembly.png" width="500" alt="Mechanical Assembly (CAD)">
</p>

The mechanical structure is designed for maximum rigidity. All parts are 3D-printed in PETG to minimize flex under load while maintaining a lightweight profile for the micro servos.

## Architecture Highlights

* **Asynchronous Web Control:** WebSocket communication and web server operations run completely asynchronously. The main MCU loop is dedicated to calculating kinematics and servo steps, eliminating motor jitter during network traffic.
* **Custom Motion Profiling (`SmartServo`):** Driving servos at their default maximum speed causes structural wobble and high current spikes. The custom `SmartServo` C++ class implements independent speed and step-delay parameters for each joint.
* **Hardware PWM Offloading:** To prevent timer conflicts between Wi-Fi interrupts and PWM generation, signal processing is offloaded to an external PCA9685 module via I2C, ensuring a rock-solid 50Hz pulse.
* **Soft-Start Homing Sequence:** The firmware includes a low-speed homing routine on boot. This safely drives all joints to a known 90-degree position, preventing brownouts caused by sudden inrush currents.

## Hardware & Electronics

<p align="center">
  <img src="electronics/circuit_diagram.png" width="600" alt="Circuit Diagram">
</p>

<p align="center">
  <img src="docs/media/electronics_base.jpg" width="500" alt="Electronics Integration">
</p>

* **Microcontroller:** ESP32-C3 SuperMini
* **Actuator Driver:** Adafruit PCA9685 (I2C Address: `0x40`)
* **Actuators:** 5x MG90S Metal Gear Micro Servos
* **Power Supply:** XL4015 DC-DC Buck Converter (Stepped down to `5.0V`)
* **Filtering:** 100µF and 1mF decoupling capacitors across the power rails to suppress inductive spikes.

## Getting Started

### Prerequisites
* [PlatformIO](https://platformio.org/) or Arduino IDE (with ESP32 board manager installed)
* Required Libraries: `Adafruit PWM Servo Driver Library`, `ESPAsyncWebServer`, `AsyncTCP`

### Installation
1. Clone the repository:
   ```bash
   git clone [https://github.com/erayfazilordanuc/esp-5dof-manipulator.git](https://github.com/erayfazilordanuc/esp-5dof-manipulator.git)