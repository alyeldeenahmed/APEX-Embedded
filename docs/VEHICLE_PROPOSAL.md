# Vehicle implementation proposal — 4 November 2026

Authority: the Embedded team leader requested use of the two v0.1 PDFs and
proposals for unspecified behavior. These choices are not joint team approval.

## Perception

Continue scheduled 100 Hz transport even if the IMU has never produced a sample.
Before its first sample, IMU-valid is false and its timestamp is report time;
after acquisition, retain the acquisition timestamp on held invalid data.
Perception must ignore IMU payload/timestamp as a measurement when invalid.
Transport absence is never interpreted as standstill.

Proposed wheel estimator: count-latch intervals, nominally 10 ms, with average
speed over the actual interval. Timestamp the end of that interval; effective
delay is approximately half the interval plus transport. Metres/count must be
measured from target count, shaft-to-wheel ratio and rolling circumference.
An interval containing zero pulses reports zero quantized speed; this is not
proof of physical rest or sensor health. Do not set standstill from this alone.
One-channel shaft sensing cannot measure direction. The current Wheel core gives
unsigned speed only; signed-speed validity requires a direction-capable sensor
or explicit agreement on a limited forward-only operating regime.

## Control

The model input a_cmd is requested longitudinal force divided by modeled mass,
not necessarily measured dv/dt. Initial conversion proposal is calibrated
feedforward drive/brake effort versus a_cmd, with saturation. Real calibration
will likely need speed and battery dependence. Do not substitute an integrator
of a_cmd as a desired-speed controller without Control agreement.

The pure Controller core uses boot identity, monotonic modular sequence,
exclusive absolute MCU deadline (maximum 100 ms ahead, provisional), explicit
neutral arm, setpoint and disarm actions. Invalid requests do not extend the
deadline. Expiry or invalid local feedback disarms; fresh explicit arming is
required. Steering rate uses elapsed update time. A negative normalized effort
means braking, never reverse. No conversion coefficients or limits are invented.

The core is now used by the integrated command endpoint. Session/epoch installation,
wire parsing, acknowledgements, cancellation and PWM adapters are implemented in
`control_endpoint.h`, `command_protocol.h` and `vehicle_board.h`. See
CONTROL_PROTOCOL.md for the authoritative current proposal.

## Hardware boundary

Vehicle actuator outputs are disabled by default. Keep all calibration confirmation
flags false until measurements establish steering mapping/rate, ESC pulse format,
neutral behavior, brake/reverse semantics, speed scaling and valid operating
limits. A neutral output or disabled signal is not a demonstrated safe stop.

## Verification

tests/test_vehicle.cpp exercises stationary count windows, count-to-speed scaling,
invalid calibration, arm, replay rejection, steering rate, acceleration mapping,
expiry, rearm requirement, local-feedback loss, NaN and wrong boot identity.
Synthetic coefficients in tests are fixtures, not vehicle calibration.
