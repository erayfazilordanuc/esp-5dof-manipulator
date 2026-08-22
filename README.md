# ESP32 5-DOF Manipulator

<p align="center">
  <img src="docs/media/full_assembly_folded.jpeg" height="400" alt="Arm in the folded pose">
  <img src="docs/media/full_assembly.jpeg" height="400" alt="Arm reaching forward">
</p>

A five-axis robotic arm built from scratch: a 3D-printed structure, an ESP32 running the
motion controller, and a control page served straight off the board over Wi-Fi. No host
computer involved.

The firmware is called ArmPilot. The arm is driven by hobby servos, which report nothing
back — no encoder, no current sense, no way to ask one where it is. Three problems follow
from that, and most of the firmware is an answer to one of them:

- A servo that has just been powered up slams to whatever pulse width reaches it first, so
  firmware that attaches and writes an angle in the same breath makes the arm jump on boot.
- Software PWM on a chip that is also servicing Wi-Fi interrupts jitters, which you hear as
  buzzing and see as twitching.
- Commanding a servo straight to its target makes it accelerate as hard as it can, which on
  a printed arm shows up as wobble on departure and overshoot on arrival.

## Motion control

Without position feedback a PID loop has nothing to close around, so instead of correcting
error after the fact ArmPilot never commands a move the arm cannot follow. Every axis owns a
profile generator ([`motion_axis.h`](firmware/include/motion_axis.h)) stepped at 100 Hz,
handing the servo an intermediate setpoint every 10 ms under velocity, acceleration and jerk
limits.

```
velocity
   ^      ______________
   |     /              \     jerk limit smooths the take-off
   |    /                \    v = sqrt(2*a*distance) brake curve
   +---/------------------\--> time
```

The asymmetry matters: jerk is limited only while accelerating, while braking keeps full
acceleration authority. That is what stops the axis sailing past its target. Measured
overshoot is 0.0000° across all five axes, and retargeting mid-move continues from the
current velocity rather than restarting.

Measured on a 90° move at the default 0.85x scale:

| Axis | Duration | Peak velocity | Travelled in first 100 ms |
|---|---|---|---|
| Base | 3.52 s | 34 °/s | 2 % |
| Shoulder | 4.39 s | 25 °/s | 2 % |
| Elbow | 2.27 s | 60 °/s | 3 % |
| Wrist | 1.82 s | 81 °/s | 4 % |
| Gripper | 1.41 s | 111 °/s | 5 % |

Base and shoulder are deliberately slowest, carrying the most inertia and load. The speed
slider scales the set between 0.15x and 1.30x; acceleration and jerk scale with it (as `s²`
and `s³`) so a slow move is the same move stretched out, not a different one. Per-axis
limits live in [`config.cpp`](firmware/src/config.cpp):

| Axis | Channel | Range | Home | Park | v max | a max | jerk max |
|---|---|---|---|---|---|---|---|
| Base | 0 | 0–180° | 90° | 90° | 40 °/s | 55 °/s² | 300 °/s³ |
| Shoulder | 1 | 5–175° | 90° | 125° | 30 °/s | 42 °/s² | 220 °/s³ |
| Elbow | 2 | 0–180° | 90° | 155° | 70 °/s | 110 °/s² | 700 °/s³ |
| Wrist | 3 | 0–180° | 90° | 120° | 95 °/s | 160 °/s² | 1000 °/s³ |
| Gripper | 4 | 0–180° | 60° | 60° | 130 °/s | 260 °/s² | 1800 °/s³ |

Profile generation runs in its own FreeRTOS task pinned to core 1 at priority 3 with a fixed
period, while networking runs on the other core. Saturating the WebSocket does not change
how the arm moves. PWM generation itself is offloaded to a PCA9685 over I²C, so the pulse
train comes from dedicated hardware regardless of what the ESP32 is doing.

## Starting up without the jump

At boot ArmPilot emits no pulses at all. `ArmController::begin()` puts every PCA9685 channel
into full-off, the servos stay torque-free, and the arm physically cannot move until asked.

Two seconds after motion settles the joint angles are written to NVS. On the next boot they
are read back and shown as the assumed pose, and pressing engage writes that *stored* angle
to each channel in turn, 220 ms apart. The commanded angle already matches the physical one
so nothing moves, and staggering the channels keeps the inrush from pulling the supply down.
If the arm was moved by hand while off, drag the on-screen arm to match reality first.

What this cannot fix: if the ESP32 resets, outputs shut off and the arm drops under its own
weight. Hobby servos have no brake. Wiring `/OE` to a GPIO lets the firmware kill outputs in
hardware too, but it does not hold the arm up.

## Calibrating the joints

Servo horns are splined, so they mount at whatever offset lines up during assembly. "Servo
0° means the link is straight" is never true out of the box, and with gearing one degree of
servo is not one degree of link. Each joint carries a linear model instead of an assumption:

```
joint_angle = wRef + gain * (servo_angle - sRef)
```

`sRef` and `wRef` are a matched pair of servo angle and real joint angle at one pose. `gain`
is joint degrees per servo degree: its sign encodes direction, so a reversed mount comes out
negative, and its magnitude carries the ratio. Direct drive gives ±1.

Two unknowns need two samples. In the browser you engage the arm, move the real arm with the
panel sliders, drag the on-screen arm until it matches, and save two clearly different poses.
The firmware solves the fit and writes it to NVS.

The guardrails are where most of the work went. An axis that moved less than 8° between
samples is skipped and keeps its old value, since a second sample from the same pose teaches
nothing. Samples are rejected while the arm is moving or disarmed, because with no torque
there is no meaningful servo angle to record. Fits outside a gain of 0.05 to 20 are rejected
as physically impossible, which catches swapped channels before they become motion.

## The control page

The UI is a single page held in flash as `PROGMEM`, so there is no filesystem image to build
or upload. It gives two 2D views instead of five sliders, because sliders make you think in
servo angles and the shape of the robot is what you care about: a side view where dragging
the end point solves shoulder and elbow together through inverse kinematics, and a top view
dial for base rotation.

The solid arm is the real one, drawn from telemetry at 20 Hz. A dashed ghost shows the
commanded pose and fades as the arm converges, which makes it obvious whether the arm is
lagging or has stopped following. `Space` is emergency stop and resume, `H` is home, `P` is
park.

Commands go over a WebSocket at `/ws` as plain text:

```
a <a0> <a1> <a2> <a3> <a4>   set all axes
j <id> <deg>                 set one axis
s <scale>                    speed scale, 0.15 to 1.30
c engage|release|home|park|stop|resume|save
k ...                        calibration: capture, solve, flip direction, reset
```

The board answers with a `hello` frame carrying geometry, joint limits, IP and the reason for
the last reset, a `cal` frame re-broadcast whenever calibration changes, and a 20 Hz state
packet.

## Mechanics

<p align="center">
  <img src="cad/renders/assembly.png" height="300" alt="CAD assembly">
  <img src="cad/renders/assembly_section.png" height="300" alt="Section view">
</p>

Everything structural is printed in PETG rather than PLA, to keep flex under load down and to
survive the heat that builds up around servos in a sealed base. The section view was the
working drawing for servo clearance, cable routing and wall thickness. The base is a closed
cylinder carrying the electronics and battery, which puts the heaviest parts at the bottom
and keeps the arm planted when the elbow is fully extended.

<p align="center">
  <img src="docs/media/full_assembly_side.jpeg" height="330" alt="Printed arm, extended">
</p>

Print-ready STLs are in [`cad/print_files/`](cad/print_files/) (base housing and lid, J1–J4
links, the parallel gripper drivetrain, and a pre-arranged build plate), editable Fusion 360
sources in [`cad/source/`](cad/source/). Superseded v1 parts are kept in
[`cad/print_files/v1/`](cad/print_files/v1/).

The browser draws the arm from link lengths the board sends on connect, so changing the
mechanics means editing one struct: base radius 62 mm, floor to shoulder 105 mm, shoulder to
elbow 110 mm, elbow to wrist 95 mm, wrist to gripper tip 72 mm.

## Electronics

<p align="center">
  <img src="electronics/circuit_diagram.png" width="760" alt="Circuit diagram">
</p>

Nothing is driven from the microcontroller directly. Two separate buck converters run off the
same battery, an XL4015 feeding the ESP32 and an 8 A XL4016E1 feeding the servo rail, so
servo inrush never reaches the logic supply. 100 µF sits across the ESP32 rail and 1000 µF
across the servo rail to absorb the spike a stalled motor throws back. A variant with a 2S
BMS and USB-C charging is in
[`circuit_diagram_with_bms.png`](electronics/circuit_diagram_with_bms.png).

| Qty | Part | Notes |
|---|---|---|
| 1 | ESP32-WROOM-32 devkit, 30-pin | Wi-Fi and two cores |
| 1 | Adafruit PCA9685 | 16-channel 12-bit PWM, I²C `0x40` |
| 3 | MG996R servo | Base, shoulder, elbow |
| 2 | MG90S servo | Wrist and gripper |
| 1 | XL4015 buck converter | Logic rail |
| 1 | XL4016E1 8 A buck converter | Servo rail |
| 2 | 18650 Li-ion and holder | In series, about 7.4 V nominal |
| 1 each | 100 µF / 1000 µF electrolytic | ESP32 rail / servo rail |
| 2 | Toggle / slide switch | Master and logic cut-off |

Any 50 Hz hobby servo works in place of these; trim each axis with `usMin` and `usMax` in the
joint table.

`GPIO21` and `GPIO22` carry I²C to the PCA9685 `SDA`/`SCL`, `3V3` feeds its logic, and `VIN`
comes from the XL4015 through the switch and the 100 µF. Grounds are common across both
rails. Servo power reaches the `V+` terminal from the XL4016E1, not from the logic pins.

<p align="center">
  <img src="docs/media/electronics_base.jpeg" width="430" alt="Electronics stack in the base">
</p>

The whole stack sits on the base plate and closes inside the base cylinder.

<p align="center">
  <img src="docs/media/electronics_base_front.jpeg" height="230" alt="Front view">
  <img src="docs/media/electronics_base_top.jpeg" height="230" alt="Top view">
</p>

## Build and flash

You need [PlatformIO](https://platformio.org/). Dependencies resolve from `platformio.ini`.

```bash
git clone https://github.com/erayfazilordanuc/esp-5dof-manipulator.git
cd esp-5dof-manipulator/firmware

# Wi-Fi credentials are not kept in version control
cp include/secrets_example.h include/secrets.h
# then edit include/secrets.h and fill in WIFI_SSID and WIFI_PASSWORD

pio run              # build
pio run -t upload    # flash
pio device monitor   # the IP address is printed here
```

If the network is unreachable the board brings up its own access point, `ArmPilot-AP` with
password `armpilot1`, at `http://192.168.4.1`. On your own network `http://armpilot.local`
works over mDNS. The servos come up free by design; press engage in the UI to energise them.

Tuning is split between two files. `firmware/include/config.h` holds the AP fallback, I²C
pins, control rate and boot behaviour. `firmware/src/config.cpp` holds the joint table,
link geometry and calibration defaults. Both are pushed to the browser on connect, so there
is nothing to keep in sync on the UI side.

## When the servos go limp

Almost always the ESP32 has reset: outputs shut off, torque disappears, the arm sags. The
cause is captured on boot and shown on serial and in the UI as `BROWNOUT`, `PANIC` or
`WATCHDOG`. Brownout is the usual answer, and the fix is a separate 5–6 V supply for the
servos with grounds tied together and 1000 µF across the rail. `ENGAGE_STAGGER_MS` already
spreads out the start-up current, but peak current during a move is a question of how much
the supply can deliver.

## Repository layout

```
cad/          print_files/ (STLs + v1 archive), renders/, source/ (Fusion 360)
docs/media/   Build photographs
electronics/  Circuit diagrams, plus legacy/ for the v1 schematic
firmware/     include/, src/, and legacy/ experiments excluded from the build
```

## What's next

This firmware is meant to become the low-level hardware interface for an autonomous setup:
micro-ROS to bridge the board to a host, a URDF model accurate enough to simulate against,
and MoveIt 2 doing inverse kinematics and trajectory planning off-board.
