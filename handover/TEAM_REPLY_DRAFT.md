# Draft for Perception and Control - not sent

We have recorded Perception's Rev 2 decisions: timing v1, CRC coverage, 100 Hz
stream, latch-time wheel timestamps, continuous zero-speed telemetry and the
single Perception-owned serial bridge.

We implemented a proposed timing VERSION 2 for the additional wheel semantics;
its length/offsets/CRC stay unchanged. Byte 7 bit0=direction measured,
bit1=motion feedback suspect, bit2=monitor configured. Type-01 is unchanged.
Please confirm this exact assignment before we enable it; default remains v1.
Both reference Python decoders support it. Simulated examples and expected
raw events are in handover/wheel-v2-simulated. These are NOT real captures.

V2 wheel_valid accepts calibrated magnitude without direction; motor authorization
remains separately gated. A calibrated sustained positive physical drive with no
observed valid encoder edges latches suspect and invalidates wheel data. It invokes
the existing neutral/disarm path; no claim of active braking. Threshold/timeout
are uncalibrated and disabled. This detects missing expected motion feedback,
not uniquely an unplugged encoder; stationary disconnection is not observable.

Proposed resolution target: >=400 effective counts/wheel revolution, pending
mechanical ratio/target feasibility and your resolution requirement. At illustrative
0.19 m circumference and 10 ms this gives 0.0475 m/s/count. No sensor purchase or
exact shaft target count is claimed yet.

Control confirmation needed: forward-only operation/rollback handling, motion-fault
response/recovery, and command/status relay topic/type/QoS through apex_esp_bridge
without refreshing deadlines or replaying stale commands. Perception confirmation
needed: exact v2 semantics, and the single /vehicle/measured header stamp when wheel
and steering have different source times.

IMU vehicle validity still requires verified axes; raw bench capture remains
available first. 100 Hz is stream rate; internal MPU sampling remains nominal
1 kHz with the 42 Hz gyro filter. Real timing, noise/bias, axes, geometry, encoder
scale and steering response remain hardware measurements. Shared geometry template
is config/vehicle_geometry.json with unknown fields null. imu_gap_predict stays OFF.
