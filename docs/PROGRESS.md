# Progress — 2 November 2026

## Completed in this workspace

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

Evidence: https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41523561273

## Not yet completed

- ESP32 flashing or real-board testing.
- Actual MPU-6050 acquisition and data-ready interrupt timestamps.
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

1. Confirm data/sync interoperability with Perception using known binary examples.
2. Verify the purchased board against the compiled profile, upload and check serial timing.
3. Replace simulated IMU readings with real sensor acquisition.
4. Add shaft sensing and steering reporting after agreeing freshness semantics.
5. Agree the Control command interface, then implement command and actuator handling.
6. Run integration, timing and fault tests; publish measured results.

Mechanical fitting, calibration measurements and power-system selection are deferred
at the team leader's request while firmware work proceeds. They remain required for
eventual hardware integration.
