# Motion control

Detail behind the [motion control section](../README.md#motion-control) of the main README.
Implementation: [`motion_axis.h`](../firmware/include/motion_axis.h), limits in
[`config.cpp`](../firmware/src/config.cpp).

## Why a profile generator and not a PID

A PID loop needs a measurement to subtract from the setpoint. The servos give none, so
there is nothing to close a loop around. The alternative is to make the reference itself
followable: shape every command into a curve that stays inside limits the mechanism is known
to handle, and hand the servo an intermediate setpoint every 10 ms rather than a distant
target.

This is not equivalent to feedback control. It removes the failure modes that come from
commanding steps into an inertial system, and it does nothing about disturbance, load or a
servo that stalls. Those stay invisible to the firmware.

## The curve

```
velocity
   ^      ______________
   |     /              \     jerk limit smooths the take-off
   |    /                \    v = sqrt(2*a*distance) brake curve
   +---/------------------\--> time
```

Each 10 ms step, per axis:

1. Compute the highest velocity from which the axis could still stop within the remaining
   distance: `vBrake = sqrt(2 * aMax * |error|)`.
2. Command `min(vMax, vBrake)`, signed toward the target.
3. Take the acceleration that reaches it, clamped to `±aMax`.
4. Rate-limit that acceleration by `jMax * dt` **only while speeding up**. While braking,
   full acceleration authority is available immediately.
5. Integrate.

Step 4 is the asymmetry that matters. Jerk limiting on departure is what stops a printed arm
wobbling as it leaves; jerk limiting on arrival would delay the brake and let the axis run
long. Because velocity is capped by the brake curve at every step, the commanded position
converges on the target instead of passing it — a property of the generator, not a
correction applied afterwards.

Retargeting mid-move keeps the current velocity and re-solves from there, so a new command
during motion blends rather than restarting from zero.

## Profile output for a 90° move

At the default 0.85x speed scale:

| Axis | Duration | Peak velocity | Travelled in first 100 ms |
|---|---|---|---|
| Base | 3.52 s | 34 °/s | 2 % |
| Shoulder | 4.39 s | 25 °/s | 2 % |
| Elbow | 2.27 s | 60 °/s | 3 % |
| Wrist | 1.82 s | 81 °/s | 4 % |
| Gripper | 1.41 s | 111 °/s | 5 % |

These describe the setpoint stream the servos are handed, not the motion of the physical
link. With nothing reporting back from the arm, only the first is observable. The small
figures in the last column are the jerk limit working: the axis eases off the mark rather
than snapping.

## Per-axis limits

| Axis | Channel | Range | Home | Park | v max | a max | jerk max |
|---|---|---|---|---|---|---|---|
| Base | 0 | 0–180° | 90° | 90° | 40 °/s | 55 °/s² | 300 °/s³ |
| Shoulder | 1 | 5–175° | 90° | 125° | 30 °/s | 42 °/s² | 220 °/s³ |
| Elbow | 2 | 0–180° | 90° | 155° | 70 °/s | 110 °/s² | 700 °/s³ |
| Wrist | 3 | 0–180° | 90° | 120° | 95 °/s | 160 °/s² | 1000 °/s³ |
| Gripper | 4 | 0–180° | 60° | 60° | 130 °/s | 260 °/s² | 1800 °/s³ |

Base and shoulder are slowest on purpose: they carry the most inertia and the most load.

## Speed scaling

The UI slider spans 0.15x to 1.30x and scales the whole set together:

```
vMax * s        aMax * s²       jMax * s³
```

Those exponents keep the shape of the curve invariant — a slow move is the same move
stretched in time, not a differently shaped one. Scaling only velocity would produce a move
that still departs abruptly and merely crawls in the middle.

## Task structure

Profile generation runs in its own FreeRTOS task, created with:

```c
xTaskCreatePinnedToCore(motionTask, "motion", 6144, nullptr, 3, nullptr, 1);
```

Priority 3 puts it above the Arduino `loopTask` (priority 1), which is where telemetry
broadcast and WebSocket housekeeping live. So when the network is busy, the scheduler
preempts the web work and runs the control step on time — not the other way round. A control
loop whose period depends on network load is not a control loop.

Two things make the timing forgiving beyond that. The task sleeps to a fixed period rather
than a fixed delay, so its own work does not accumulate drift. And servo pulses come from the
PCA9685, which keeps emitting clean 50 Hz output on its own — a late task writes a slightly
stale pulse width, it does not produce a malformed pulse.
