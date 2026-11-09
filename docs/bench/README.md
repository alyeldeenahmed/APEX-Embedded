# Component test guides

9 November 2026. Procedures only; no physical results have been recorded.

Download the [printable PDF pack with blank measurement sheets and copyable commands](../../handover/APEX_Bench_Test_Pack.zip).

1. [Start here](00_Start_Here.md)
2. [Interface review and remaining peer decisions](01_Interface_Review.md)
3. [ESP32 USB and serial](02_ESP32.md)
4. [MPU6050](03_MPU6050.md)
5. [Quadrature encoder](04_Encoder.md)
6. [Steering servo](05_Steering_Servo.md)
7. [ESC and motor](06_ESC_and_Motor.md)
8. [Pi and cooling](07_Pi_and_Cooling.md)
9. [Camera V2](08_Camera_V2.md)
10. [LiDAR](09_LiDAR.md)
11. [Power and logic buffer](10_Power_and_Buffer.md)

All standalone sensor tests retain disabled motor/servo outputs. Actuator
calibration starts with a separate manual pulse source; do not bypass integrated
firmware calibration gates. Exact encoder hardware, ESC operating mode, servo
rating and final power components still require the checks stated in each guide.

The interface review is complete on Embedded's side. Joint closure still requires
Omar's transport/authority decisions and Perception's timestamp/schema/recovery
decisions. The exact messages are in the interface guide. Nothing has been sent
to other team members automatically.

Validation of this delivery: 35 existing Python tests passed; git diff whitespace
check passed; all 22 PDF pages rendered and visually reviewed. Runtime firmware
was not changed, flashed or hardware-tested. Existing six-profile build evidence
belongs to the earlier runtime revision; a fresh CI run may provide new evidence.
