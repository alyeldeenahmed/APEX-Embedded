# Integration handoff — 24 November 2026

Confirmed profile: ESP32-S3 N16R8, MPU6050, motor-mounted MT6701, THW-1060-RTR, Carson CS6, Pi 5. See CAR_INTEGRATION.md for pin-to-pin connections and commands.

**I2C correction:** use this revision, not the previous SSI ZIP. For Arduino Serial Monitor testing, start with [MT6701 I2C first test](docs/bench/MT6701_I2C_START.md).

## What is ready in software

- Vehicle firmware defaults to the real sensor profile, 100 Hz paired data/timing, and disabled physical PWM.
- MT6701 I2C reads provide signed accumulated motor counts. Wheel speed requires the actual motor-to-wheel ratio and loaded tyre circumference.
- Actual static servo endpoints are 1250/1500/1750 us. Positive road-wheel angle is left; equivalent limit is +/-0.33161256 rad. The mean-wheel-angle approximation is not a measured intermediate steering curve.
- car_link.py provides read-only decoded recording and explicit short commissioning commands. It rejects old status anchors and clears RPM history across detected restarts or report-clock reversal.
- The existing telemetry wire layouts stay unchanged. Timing v2 diagnostics remain opt-in; team approval is needed before enabling them.

## First integration session — drivetrain power disconnected

1. Rewire using the confirmed profile: ESC 4, servo 5, IMU SDA/SCL/INT 8/9/7, MT6701 SDA/SCL 10/11 (GPIO12 unused). Numbers are GPIO labels. Old wiring PDFs use different pins.
2. Compile/upload the vehicle profile as explained in CAR_INTEGRATION.md. Do not upload a simulation binary.
3. Close Arduino Serial Monitor and stop apex_esp_bridge before using the standalone monitor. Select the ESP serial device, not the LiDAR.
4. Run `python car_link.py --port COM3 monitor --seconds 60 --output integration_capture.jsonl` on Windows, or replace COM3 with the Pi ESP serial path.
5. Confirm status physical-PWM flag is clear. Turn the motor by hand: about 16384 accumulated counts per motor revolution, reversing sign when reversed. Check valid continuous intervals and gaps.
6. Confirm IMU mounting axes, continuous data-ready timestamps and fault-free operation with the final soldered harness. Earlier bench captures included configuration faults/recovery, so reliability is still unqualified.
7. Close this monitor before starting Perception's apex_esp_bridge. That process alone owns /dev/apex_esp. Share the decoded capture; raw byte capture and ROS bridge integration are separate Perception tasks.

## Required before powered driving

- Verify MT6701 I2C mode, magnet mounting, direction, ratio, loaded circumference and reliable acquisition under the maximum actual motor speed. A 1000 us maximum read interval assumes less than 30000 RPM; faster motors need a tighter bound.
- Confirm installed IMU axes and resolve wiring faults before enabling its vehicle-valid flag. Gyro remains in rad/s without bias subtraction; acceleration includes gravity in m/s2.
- Calibrate ESC neutral, forward and brake PWM, prove brake-only operation, measure acceleration/deceleration limits and effort mapping. Do not assume a Hobbywing manual describes this THW unit.
- Measure or explicitly approve provisional steering rate and actuator response delays for MPC. Static endpoints do not establish dynamic response.
- Calibrate drive-without-motion threshold/timeout; verify stop, loss of feedback, stale-command expiry and reset behavior with wheels raised.
- Verify power branch under load, grounds and PWM signal levels. Software verification does not qualify the Pi crowbar, fuses, regulator or battery wiring.
- Only then complete each confirmation gate in vehicle_config.h and select the physical-output build: `python tools/build_firmware.py --vehicle --enable-outputs`. It writes build/vehicle-outputs/. With current missing calibration, that build still refuses physical PWM and ARM. See CAR_INTEGRATION.md.

## Agreements to close with other teams

Perception: accept 16384 counts per MOTOR revolution, the measured ratio/scale, signed direction and gap handling; validate paired frames from the live build. Confirm final sensor geometry and source-time mapping.

Control and Perception/Omar: finalize command/status ROS relay types, topics, reconnect and explicit rearming behavior through apex_esp_bridge. Supply Control with measured actuator limits and delays when obtained. No second serial-port owner is permitted.

## Verification evidence

Run `python tools/validate_software.py --firmware` for host tests and eight compile-only profiles (including the read-only encoder bench). The JSON report distinguishes PASS, FAIL and BLOCKED. These tests do not constitute hardware acceptance. No firmware upload, physical actuation, GitHub push or team message is performed by verification.
