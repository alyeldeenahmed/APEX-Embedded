# ESC and motor connection and calibration

9 November 2026. Procedure only; no hardware pass claimed.

THW-1060-RTR brushed ESC and Tamiya Torque Tuned 54358. Owner label: 2-3S LiPo, 5-9 cell NiMH, SBEC6V3A. This does not establish neutral, calibration procedure, brake mode or signal thresholds. Do not substitute a Hobbywing manual as proof for this unit.

| Connection | Destination / rule |
| --- | --- |
| Battery positive | Source fuse and accessible DC disconnect, then ESC battery positive; fuse/wiring selection must suit actual setup |
| Battery negative | ESC battery negative using rated power wiring |
| ESC motor pair | Motor two terminals; polarity establishes forward, verify with drive disconnected |
| ESC receiver signal | Manual tester signal initially; later GPIO16 via verified level interface |
| ESC receiver ground | Tester ground and common signal ground |
| ESC BEC positive | Leave insulated unless deliberately powering compatible tester/servo; never Pi/ESP |
| USB / ESP | Not required for first tester-only test; never feed6V to ESP |

1. Obtain the exact mode/arming procedure from the unit documentation or supplier. Record switch/jumper positions. Initial test is blocked until neutral and intended forward/brake-only behavior can be established. Do not use full-throttle power-up calibration from a different ESC manual.

2. Secure motor/chassis and remove the pinion or otherwise disconnect drivetrain for the first direction test. Disconnect servo. Connect a manual pulse tester at verified neutral; use only one signal source. Keep hands, loose leads and clothing clear.

3. Power the setup with a correctly protected compatible supply. Confirm BEC voltage and neutral behavior. Increase command in small controlled increments until first rotation; record pulse and direction. Return neutral immediately.

4. Measure drive deadband and repeatable start threshold. Verify brake-versus-reverse mode using the actual procedure before any negative-force command. If reverse cannot be disabled, the existing brake-only adapter is not qualified; stop integration and resolve the mode/driver design.

## Measurements for Control

## First delivery from the bench

Record neutral and usable pulse bounds, start/deadband threshold, motor direction, BEC voltage, command pulse rate, pulse-to-first-shaft-edge delay and supply current. Repeat at least10 safe starts at the same pulse and supply, then several usable drive levels. First-edge delay includes angular target phase and encoder resolution; report that uncertainty. Do not stall the motor to measure current or call unloaded spin-up vehicle acceleration.

## Loaded measurements after the chassis is ready

1. Use a clear controlled straight test area, independent stop/disconnect and a second operator. Begin at low speed. Record actual mass, battery voltage, surface and tyre condition. Verify direction sensing and stop behavior first.

2. Log pulse/write time, signed wheel speed and timestamps, gyro z, acceleration, battery voltage/current and command ID where available. Apply bounded drive steps over multiple initial speeds; repeat braking from known low forward speeds. Do not start with maximum throttle or reverse.

3. Use timestamped speed change to estimate dvx/dt. For comparison to dynamic MPC a_cmd, account for r*vy: a_cmd = dvx/dt - r*vy. Straight tests reduce this coupling. Gravity/bias-correct IMU acceleration if used for analysis; preserve raw IMU data separately.

4. Fit drive/brake mapping over the tested envelope and record residuals. Current linear coefficients are only suitable if the data supports them; voltage/speed dependence may require a table/model. Choose conservative limits within the tested envelope, not the largest isolated result.

## No-pulse monitor calibration

Select normalized effort threshold above repeatable drive deadband. Timeout must exceed measured worst startup-to-first-edge delay and normal edge gaps at the minimum commanded speed, with stated margin. Evaluate false alarms and a controlled signal-loss fixture. Do not mechanically stall the car. The current fault is latched until reboot; team agreement is pending. Zero defaults block motor arming.

## Record and send

Control receives max usable drive/brake acceleration, mapping, neutral/deadband, delay distributions and variation with speed/voltage/load. Include raw trials and method. Distinguish serial-to-write, write-to-PWM edge, and PWM-to-motion delays. No response-time value can be inferred from50Hz PWM alone.

## References and configuration

[Local implementation and calibration fields](https://github.com/alyeldeenahmed/APEX-Embedded/blob/814dfce258f0046f8fe4f426be563c7bf64c7ba7/firmware/telemetry_demo/vehicle_config.h)
