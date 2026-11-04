# Progress — integrated implementation, 4 November 2026

## Current software scope

State v0.4 review added independent timestamp companions, raw-event deduplication
and an optional 200 Hz build. See STATE_V04_REVIEW.md for changes, compatibility
requirements and Pi-owned dropout/trust behavior. Hardware validation is still pending.

Integrated `--vehicle` mode connects sensor acquisition, shaft GPIO capture,
wheel estimator, 100 Hz telemetry, mixed serial command transport, boot/session/
epoch checks, replay/expiry handling, explicit arm, cancel/revoke, result/status
frames, acceleration-to-effort and steering mappings, disabled-by-default PWM
drivers and task watchdog. Python codec/bench client and host tests accompany it.
The v0.1 gaps are implementation proposals authorized by the team leader, not
cross-team approvals. See CONTROL_PROTOCOL.md and INTEGRATED_TEST_PLAN.md.

Hardware measurements, sensor direction selection, calibration, Pi-owned bridge,
joint contract approval and vehicle tests remain. No hardware testing was performed.

The entries below are historical milestone records; their incomplete-software
lists describe the 3 November baseline and are superseded by the scope above.

## Historical progress — 3 November 2026

## Completed in this workspace

- Milestone 4: purchased N16R8 profile with GPIO8/9/4; nonblocking IMU startup,
  retry/reinitialization, missing-data recovery and periodic register audit.
- Standalone CSV mode, live recorder/offline summary and step-by-step bench guide.
- All three N16R8 builds, 13 Python tests, 208 packet comparisons and expanded
  host sensor/recovery tests passed. Compiled bench files available in CI artifacts.

Milestone 4 evidence: https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41684429013

- Reviewed the Control–Embedded preliminary interface v0.1 (26 October 2026)
  and Embedded–Perception interface v0.1 (1 November 2026).
- Identified the Embedded team's scope: sensor acquisition, timestamps, transport,
  actuator execution, local agreed checks and hardware validation.
- Recorded the planned hardware and unresolved choices in HARDWARE.md.
- Implemented a Python encoder and stream decoder for the 54-byte sensor frame.
- Implemented a C++ packet serializer and Arduino ESP32 simulation sketch.
- Added a provisional 14-byte time-sync frame implementation.
- Ran eight Python tests and a 100-data-frame simulated capture check successfully.
- Milestone 2: compiled the ESP32-S3 N8R2 bench profile using Arduino core 3.3.0.
- Passed nine Python tests, eight fixed reference packets and 200 additional
  host C++/Python packet comparisons in GitHub Actions.
- Published reusable packet vectors and a reproducible build guide.
- Milestone 3: added MPU-6050 register setup/readback, burst acquisition, data-ready
  timestamps, SI conversion, clipping/freshness logic and optional axis rotation.
- Fake-bus sensor tests and both ESP32 firmware modes passed in GitHub Actions.

Milestone 3 evidence: https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41682300906

Evidence: https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41523561273

## Not yet completed

- ESP32 flashing or real-board testing.
- MPU-6050 hardware validation (new software prototype described in IMU.md).
- Shaft pulse capture, speed calculation and direction detection.
- Steering measurement or commanded-angle reporting from a real actuator path.
- Control command schema, parsing, acknowledgement or actuator execution.
- Watchdog and command-loss behavior, calibration or physical stopping tests.
- Joint interface approval, ROS bridge integration and end-to-end vehicle tests.

## Scope by team

| Team | Responsibility in the current draft division |
|---|---|
| Embedded | MCU firmware, sensor I/O, sample timestamps, wire packets, actuator signals and hardware evidence |
| Perception | Pi sensor bridge, clock mapping, state estimation and sensor-data fault reactions |
| Control | MPC/planning, higher-level safety decisions and interpretation of execution reports |

## Next milestones

Milestone 4 software passed its host tests and three firmware builds. Actual
sensor operation, interrupt timing, wiring and orientation remain unverified.
The team leader confirmed Embedded ownership of acceleration-to-ESC conversion;
the motor-control implementation remains outstanding.

1. Confirm data/sync interoperability with Perception using known binary examples.
2. Verify the purchased board against the compiled profile, upload and check serial timing.
3. Identify MPU breakout power circuit, wire selected pins and execute standalone
   IMU_BENCH.md tests. Recovery code is implemented; physical recovery is unverified.
4. Add shaft sensing and steering reporting after agreeing freshness semantics.
5. Agree the Control command interface, then implement command and actuator handling.
6. Run integration, timing and fault tests; publish measured results.

Mechanical fitting, calibration measurements and power-system selection are deferred
at the team leader's request while firmware work proceeds. They remain required for
eventual hardware integration.
