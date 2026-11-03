# MPU-6050 acquisition prototype — 3 November 2026

This is bench firmware, not an accepted vehicle interface. Simulation remains the
default. MPU mode compiles separately with `python tools/build_firmware.py --imu`.
No hardware pins are selected, no orientation is claimed and no actuators are driven.

## Implemented behavior

- Reset and wake the device; verify WHO_AM_I = 0x68 (also when AD0 selects address
  0x69). Address defaults to 0x68 and can be overridden by APEX_IMU_ADDRESS.
- Disable FIFO/DMP and standby axes. Configure and read back key registers.
- SMPLRT_DIV=0, CONFIG=3: nominal 1 kHz sample rate with gyro 42 Hz and accel 44 Hz
  bandwidth. Datasheet filter delays are about 4.8/4.9 ms, not measured here.
- GYRO_CONFIG=0x10: +/-1000 deg/s, 32.8 LSB/(deg/s), converted to rad/s.
- ACCEL_CONFIG=0x08: +/-4 g, 8192 LSB/g, converted using 9.80665 m/s² per g.
- 400 kHz I2C, one 14-byte burst for acceleration, temperature and gyro; temperature
  bytes are skipped in telemetry. No gyro-bias or gravity subtraction, no extra filter.
- Active-high pulsed data-ready interrupt captures boot-relative microseconds.
  I2C runs only in the main loop. A late service (>=1 ms) is rejected, as is a read
  during which the observed interrupt counter changes. This conservative association
  check still needs validation against actual hardware and interrupt latency.
- Mark rail clipping on any acceleration/gyro axis invalid. A failed/short read
  invalidates the held reading rather than replacing it with plausible zeros.
- A subsequent successful read can recover validity. A 5 ms freshness ceiling is a
  bench-only proposal, separate from Perception's packet-gap thresholds.
- Optional signed-axis permutation maps readings into vehicle axes. Only proper
  rotations are accepted. Identity is a placeholder and APEX_IMU_AXES_CONFIRMED
  defaults to 0, keeping wire imu_valid false until orientation is verified.

## Integration behavior and limitations

Set APEX_USE_MPU6050=1 to select real acquisition code. APEX_IMU_SDA,
APEX_IMU_SCL and APEX_IMU_INT default to -1, so initialization is disabled until
wiring is explicitly configured. CI compiles this mode without assigning pins.

The intended acquisition is 1 kHz; the newest available reading is included at the
100 Hz packet schedule. A timestamp represents data-ready, not transmission time;
sensor filtering still introduces physical delay. The main-loop acquisition path
does not guarantee that every internal sample is read under load.

No data packet is fabricated before a first burst reading. Sync frames may still
be sent. After a first reading, telemetry continues even if it becomes invalid or
stale; held values keep their original sample timestamp. Wheel and steering validity
remain false. This bench behavior is not the final startup contract requiring all
sensors to be initialized. The Pi must not treat sync-only traffic as valid IMU data.

A two-second calibrating flag window starts at the first unambiguous successful
burst read. It does not estimate bias, check standstill or enforce physical immobility.
Startup timing and motion prevention need agreement and implementation later.

Initialization occurs once at boot. Failed initialization or lost configuration does
not yet trigger automatic sensor reinitialization. Runtime configuration auditing,
bus recovery, sensor reconnect tests and robust restart detection remain outstanding.
There is no FIFO, measured timing evidence, live ROS bridge or fault acceptance claim.

## Verification

Host tests use a fake register bus and the actual sensor logic. They check configured
registers/readback, signed conversion and SI units, temperature exclusion, all gyro
and accelerometer clipping rails, freshness boundaries, proper axis rotations,
failed reads, recovery on a later read and failed initialization.

CI builds both simulation and MPU-6050 modes for the planned ESP32-S3 N8R2 profile.
Results are recorded in VALIDATION.md after the checks finish. Compiling the adapter
does not prove interrupt/data alignment, I2C electrical behavior or real sampling.

## Next bench acceptance steps (when hardware is available)

Confirm board pins and 3.3 V signal compatibility; verify device identity/settings;
check stationary gravity and turn signs; confirm mounting rotation; measure data-ready
to-burst timing and packet rate; test disconnect/reconnect, clipping and restart.
Keep motor/servo control disabled during these tests.

Sources: TDK/InvenSense MPU-6000/MPU-6050 Register Map (RM-MPU-6000A-00,
revision 4.2), sections for sample divider, configuration, ranges, interrupts,
sensor outputs, power management and identity; Product Specification revision 3.4.

- https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
- https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet.pdf
