# Integrated firmware handover and deferred hardware tests

Build: `python tools/build_firmware.py --vehicle`. This compiles the actual sensor,
command, watchdog and driver code for ESP32-S3 N16R8, with hardware outputs disabled
and calibration unconfirmed by default. No upload is performed by the build tool.
Keep the standalone `--bench` IMU mode for the first physical test.

## Configuration inventory

Edit `firmware/telemetry_demo/vehicle_config.h` only after measuring values:

| Parameter | Required evidence |
|---|---|
| encoder A GPIO5, optional B GPIO6 | wiring, levels and clean pulse edges |
| quadrature, encoder_sign | physical direction and transitions |
| metres_per_count | tyre rolling circumference, shaft ratio, counts/revolution |
| min_edge_us | noise rejection without losing highest-rate valid pulses |
| wheel_confirmed | scale, direction, interval and fault limitations reviewed |
| servo GPIO15, ESC GPIO16 | electrical levels, connector polarity |
| PWM frequency and pulse endpoints | actual servo/ESC requirements |
| angle/rate and acceleration/deceleration bounds | physical calibration and Control limits |
| drive/brake conversion coefficients | achieved response over operating envelope |
| pulse_mapping_confirmed / brake_only_mode_confirmed | scoped actuator bench tests |
| calibration.confirmed / outputs_enabled | explicit hardware-validation decision |

No credentials or vehicle parameters are loaded silently. The default firmware
rejects ARM. A single-channel sensor can populate raw ticks and unsigned speed for
inspection, but cannot set the signed wheel-valid flag. Quadrature counts every
valid transition, so metres/count must include the x4 factor. Illegal transitions
invalidate the affected interval. Unplugging a stationary encoder may be
indistinguishable from zero speed; valid count sampling is not a sensor-health proof.

## Perception contract clarification

State v0.4 review: integrated frames now carry a type-03 companion with independent
report/IMU/wheel/steering timestamps; see STATE_V04_REVIEW.md. Test this extension
with Perception before deployment. Add real dropout/recovery/backlog tests and
confirm the ten-sample/AMCL trust gates on the Pi. A 200 Hz telemetry build is
available; the following 100 Hz timing refers to the retained default profile.

Telemetry target remains 100 Hz at standstill and during sensor failure; missed
serial writes increment sequence/drop counters rather than blocking execution.
IMU timestamps stay acquisition-relative after the first sample. Before acquisition,
the IMU is invalid and timestamp is report time. Wheel speed averages the actual
count-latch interval (normally 10 ms); wheel age is from latch to report time.
Wheel delay is approximately half the estimator interval, plus transport.
This report-time interpretation of age fields needs Perception approval because
the primary packet timestamp is the independently captured IMU timestamp.

Wheel ages saturate at 65535 µs. Intervals over 25 ms are invalid. Steering reports
the commanded road-wheel angle, never measured servo angle. Steering valid is set
only when a confirmed physical adapter is enabled; measured-steering bit stays 0.
Standstill bit stays 0 because no independent rest detector is provided.
Perception must not infer rest from silence or a lone zero-count interval.

## Hardware sequence (not performed)

1. Verify ESP board/MPU wiring and 3.3 V logic; run standalone IMU bench.
2. Record stationary and manual-motion captures; confirm axes, units, gyro bias,
   noise, clipping, effective delay, recovery and 100 Hz serial jitter.
3. Test encoder manually in both directions, standstill, low speed and highest
   expected pulse rate; measure scale and invalid-transition behavior.
4. Use a logic analyser with actuators disconnected: verify pulses, bounds,
   command expiry, revocation, MCU reset and deliberate loop stalls.
5. Calibrate servo mechanically and ESC neutral/drive/brake with wheels lifted,
   then measure motor force/acceleration response under controlled load.
6. Validate battery/regulator brownout behavior and independently demonstrate
   fault stopping behavior. Software tests cannot certify these outcomes.
7. Integrate the single-owner Pi bridge with Perception clock/ROS mapping and
   Control's Safety-authorized commands. Test dropped/corrupt messages, stale
   clocks, restart, cancelled requests, lost ACKs and end-to-end latency.

Store real captures under ignored `captures/`; publish only reviewed measurements
and configuration provenance. Do not report fabricated physical completion percentages.
