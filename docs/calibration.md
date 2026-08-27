# Calibration

Detail behind the calibration paragraph in the [main README](../README.md#the-control-page).
Constants live in [`config.h`](../firmware/include/config.h), the solver in
[`arm_controller.cpp`](../firmware/src/arm_controller.cpp).

## The problem

Servo horns are splined, so a horn mounts at whatever tooth lines up during assembly. "Servo
0° means the link points straight up" is never true out of the box, and where a joint is
geared, one degree of servo is not one degree of link. Hard-coding an offset means
re-flashing after every disassembly.

## The model

Each of the three arm joints carries a linear map instead of an assumption:

```
joint_angle = wRef + gain * (servo_angle - sRef)
```

- `sRef` / `wRef` — a matched pair: servo angle and real joint angle at one known pose.
- `gain` — joint degrees per servo degree. Its **sign** encodes direction, so a mirrored
  mount solves to a negative value with no code change. Its **magnitude** carries the
  reduction ratio; direct drive gives ±1.

Two unknowns per joint, so two samples are needed. The defaults shipped in
[`config.cpp`](../firmware/src/config.cpp) were measured with the arm held straight up, where
the shoulder's world angle is 90° and elbow and wrist are 0° relative to the previous link:

| Joint | `sRef` | `wRef` | `gain` |
|---|---|---|---|
| Shoulder | 53.4° | 90.0° | +1.0 |
| Elbow | 156.4° | 0.0° | +1.0 |
| Wrist | 101.5° | 0.0° | −1.0 (mounted reversed) |

The wrist is the case the sign exists for: the servo is installed facing the other way, and
that costs one character in a table rather than a branch in the code.

## The procedure

1. Engage the arm, so the servos hold torque and their commanded angle is meaningful.
2. Move the real arm to a clear pose using the panel sliders.
3. Drag the on-screen arm until its shape matches what you see, and save as sample 0.
4. Move to a second, clearly different pose and save as sample 1.
5. Solve. The firmware fits `gain` and the reference pair per joint and writes them to NVS.

Gripper open/close references and the serial/parallel kinematic mode are captured separately
from the same panel.

## Guardrails

The solver rejects more than it accepts, because a bad fit is worse than no fit — it turns
into motion.

| Check | Threshold | Reason |
|---|---|---|
| Sample span | `CAL_MIN_SPAN_DEG` = 8° | Two samples from nearly the same pose carry no information; that axis is skipped and keeps its previous value rather than being fitted from noise. |
| Arm state | Must be engaged and stopped | With no torque the servo is wherever gravity left it, so the recorded servo angle means nothing. Mid-move samples pair a stale angle with a settled drawing. |
| Gain range | `CAL_GAIN_MIN` 0.05 to `CAL_GAIN_MAX` 20 | Outside this, no plausible linkage explains the fit. In practice this catches swapped channels and mis-drawn poses before they become motion. |
| Numerics | `isfinite` on every solved value | A degenerate fit must not reach NVS. |

A rejected axis is reported back to the UI rather than failing silently, and the same range
check runs again when calibration is loaded from NVS at boot — so a corrupted store falls
back to defaults instead of driving the arm somewhere unexpected.

## WebSocket commands

All calibration goes over the same `/ws` text protocol as motion:

```
k p <0|1> <q0> <q1> <q2>   store a matching sample in slot 0 or 1
k f                        solve both samples and save
k x                        discard stored samples
k c                        take the current pose as the upright reference
k d <0..2>                 flip direction of shoulder / elbow / wrist
k m <0|1>                  kinematic mode: 0 serial, 1 parallel
k g o|c                    capture gripper fully-open / fully-closed reference
k r                        reset to factory defaults and save
```

Any change re-broadcasts a `cal` frame to every connected client, so a second browser tab
never shows stale calibration.
