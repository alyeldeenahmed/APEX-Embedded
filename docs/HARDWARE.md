# Planned hardware baseline

Recorded from the team leader's selections on 2 November 2026. This is not a
procurement confirmation or an electrical compatibility sign-off.

| Item | Planned part / arrangement | Outstanding detail |
|---|---|---|
| MCU | ESP32-S3 N8R2 development board | Not purchased when discussed; exact board/vendor and pinout pending |
| Main computer | Raspberry Pi 5 | RAM and power configuration unconfirmed |
| IMU | MPU-6050 | Breakout version, settings and final wiring pending |
| Speed sensing | Central longitudinal drivetrain shaft | TCRT5000 optical or A3144E Hall; no final sensor choice |
| ESC | THW-1060-RTR, as supplied by team leader | Verify exact model, control pulse requirements and electrical levels |
| Motor | Tamiya Torque Tuned 54358 | Installation and operating measurements deferred |
| Steering servo | Carson metal gear CS 6 Reflex Racing | Exact specifications and position-feedback availability unverified |
| Chassis | Tamiya TT-02 | Sensor fitting deferred |
| Battery / regulators | Undecided | Power system design pending |

The shaft-to-wheel ratio, tyre circumference, pulse count per revolution and
direction measurement are unknown. Firmware must not substitute invented values.
One pulse channel with repeated identical targets does not independently establish
rotation direction. Signed speed needs a direction-capable arrangement or an
explicitly agreed limitation.

Steering is provisionally expected to use commanded angle until actual position
feedback is confirmed; it must not be labeled measured without a measurement.
