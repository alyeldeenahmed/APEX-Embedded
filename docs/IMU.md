# MPU-6050 acquisition prototype — 3 November 2026

This is bench firmware, not an accepted vehicle interface. Simulation remains the
default. MPU mode compiles separately with `python tools/build_firmware.py --imu`.
GPIO8/9/4 are selected for SDA/SCL/INT on the purchased DevKitC-1 N16R8.
MPU breakout power wiring is awaiting identification. No vehicle orientation is
claimed and no actuators are driven. See IMU_BENCH.md for the standalone workflow.

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
APEX_IMU_SCL and APEX_IMU_INT use GPIO8/9/4 respectively, configured in imu_config.h.
CI compiles with these pin assignments; their physical wiring remains to be tested.

The intended acquisition is 1 kHz; the newest available reading is included at the
100 Hz packet schedule. A timestamp represents data-ready, not transmission time;
sensor filtering still introduces physical delay. The main-loop acquisition path
does not guarantee that every internal sample is read under load.

Telemetry continues before the first reading with IMU-valid false; its legacy
timestamp is then report time. After acquisition, held values retain their original
sample timestamp. Integrated vehicle mode pairs data with independent 64-bit source
times and presence flags (STATE_V04_REVIEW.md). Wheel data can continue independently.
The Pi must never treat invalid or duplicate held IMU values as fresh observations.

A two-second calibrating flag window starts at the first unambiguous successful
burst read. It does not estimate bias, check standstill or enforce physical immobility.
Startup timing and motion prevention need agreement and implementation later.

Initialization and recovery now use a nonblocking state machine. Failed setup,
three consecutive burst-read failures, >100 ms without capture, or configuration
mismatch trigger a 1 s retry, I2C peripheral restart and sensor reconfiguration.
One register is audited every 100 ms. Generation increments after each successful
initialization, restarting the calibration flag window on the first associated sample.
Physical recovery and reconnect behavior still require bench verification.
There is no FIFO, measured timing evidence, live ROS bridge or fault acceptance claim.

## Verification

Host tests use a fake register bus and the actual sensor logic. They check configured
registers/readback, signed conversion and SI units, temperature exclusion, all gyro
and accelerometer clipping rails, freshness boundaries, proper axis rotations,
failed reads, recovery on a later read and failed initialization.

CI builds simulation, binary IMU and standalone CSV modes for N16R8.
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
