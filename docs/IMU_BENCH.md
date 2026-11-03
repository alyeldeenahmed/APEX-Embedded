# Standalone IMU bench test

Board: purchased ESP32-S3 DevKitC-1 N16R8. Sensor: purchased MPU-6050 breakout,
power circuit not identified. No Raspberry Pi, ESC, motor or servo needed.

## Connections

| MPU signal | ESP32 |
|---|---|
| SDA | GPIO8 |
| SCL | GPIO9 |
| INT | GPIO4 |
| GND | GND |
| AD0, if exposed | GND (address 0x68) |
| XDA/XCL | Unconnected |
| VCC/VIN | Confirm breakout power circuit first |

GPIO numbers are printed labels, not counted header positions. GPIO35–37 are
reserved for octal memory and are not used. Pins are in imu_config.h.
The MPU chip requires 2.375–3.46 V. A breakout may include a regulator and pull-ups:
send photos of both sides/product link before wiring power. Use 3.3 V signal levels;
do not feed ESP32 GPIOs 5 V. Power off before changing wires. Inspect existing
pull-ups and use short leads/common ground.

## Build and upload after wiring confirmation

Use the USB-to-UART connector and a data cable, not the native USB connector.
Install Arduino CLI 1.3.1 and ESP32 core 3.3.0 as described in BUILD.md.

```
python tools/build_firmware.py --bench
arduino-cli board list
arduino-cli upload --port COM_NUMBER --fqbn esp32:esp32:esp32s3 --input-dir build/imu-bench
```

Replace COM_NUMBER with the observed port. The build selects 16 MB QIO flash,
OPI PSRAM and app3M_fat9M_16MB partitions. Do not use the old N8R2 image.
The build script never uploads automatically. CI also provides bench binaries.

## Recording

```
python -m pip install -r requirements-bench.txt
python imu_monitor.py --list
python imu_monitor.py --port COM_NUMBER --seconds 60 --output captures/stationary-01.csv
python imu_monitor.py --analyze captures/stationary-01.csv
```

Close other serial monitors first. Port opening may reset ESP32. CSV is recorded
with host arrival times; JSON summarizes readings and device-report spacing.
Use a new filename per capture; existing files are not overwritten. Ctrl+C saves
the summary. CSV is a separate bench protocol, without CRC; binary Perception
traffic is never mixed into it. Malformed lines are counted.

## Interpreting diagnostics

States: 0 retry; 1 reset settling; 2 wake settling; 3 configuration; 4 ready.
Faults: 0 none; 1 bus; 2 identity; 3 configuration; 4 missing data.
present=0 means no measured sample; numerical zeros then are placeholders.
Bench valid=1 means fresh/unclipped acquisition, not confirmed vehicle orientation.
Binary telemetry remains invalid until APEX_IMU_AXES_CONFIRMED is enabled.
Readings are in sensor axes for the standalone test.

Startup is nonblocking. Failed setup, three failed reads, missing captures >100 ms,
or failed configuration audit initiate a retry after 1 s. Retry restarts the I2C
peripheral and resets/configures the MPU. One register is audited every 100 ms.
A physically stuck or shorted bus may still require wiring/power intervention.
Each successful setup advances generation; the first associated sample starts a
two-second calibrating flag window. This does not estimate bias or establish stillness.

Counters show attempts, recovery, read/configuration errors, clipping, late reads,
ambiguous reads, missed edges and transmit drops. Investigate increases. Reads
serviced >=1 ms after data-ready or spanning another observed interrupt are rejected.
Freshness expires at 5 ms (bench proposal); physical latency still needs measurement.

## Test order and evidence

1. Detection: observe ready state, present=1 and valid readings, not just an LED.
2. Stationary 60 s: acceleration norm near gravity; initial screening roughly
   9–11 m/s², not a calibrated tolerance. Record gyro mean/noise; no bias is subtracted.
3. Six orientations, 10 s each: each marked sensor axis up/down; check dominant
   component near gravity and sign reversal.
4. Gentle rotations: check right-hand-rule signs around the marked axes. Record
   mounting rotation before enabling vehicle-frame validity.
5. Stationary 10 min: inspect spacing p99.9/max, sample age, held readings, invalidity
   and counters. Report spacing is not physical sample accuracy; USB arrival buffering
   and sensor filter delay require separate analysis.
6. ESP reset: press RESET; verify automatic return to ready and recorded restart.
7. Failure/reconnect: use a bench switch/test fixture for controlled interruption.
   Do not pull random live wires. Without a fixture, power down for wiring changes
   and report separate restart tests, not runtime reconnect acceptance.

Software tests inject clipping. Do not violently shake the board to force it.
Save board/module IDs, wiring photo, firmware commit, settings, captures, observed
signs, timing and failures. Review them before integration. No hardware pass is
claimed until these tests have actually run.

Sources:
- https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html
- https://product.tdk.com/en/search/sensor/mortion-inertial/imu/info?part_no=MPU-6050
