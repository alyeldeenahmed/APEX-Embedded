# Validation — 2 November 2026

Environment: Python 3.12 on Windows. Standard library only.

Command: `python -m unittest -v`

| Test | Result |
|---|---|
| CRC known vector `123456789` → `0x29B1` | PASS |
| Fixed field offsets, signed values and independent CRC comparison | PASS |
| Fragmented byte stream and leading garbage | PASS |
| Corrupted frame rejection and recovery | PASS |
| Sequence and 32-bit timestamp encoding wrap | PASS |
| Bad header and truncated frame handling | PASS |
| Provisional sync layout and independent CRC comparison | PASS |
| Invalid encoder inputs | PASS |

Additional generated capture: 100 simulated data frames plus one sync frame decoded
with zero CRC errors. Data timestamps were spaced by exactly 10,000 microseconds,
and sensor-valid flags were false throughout.

This capture is generated without real-time pacing. It proves neither serial timing
nor sensor accuracy. No actuator or vehicle acceptance test has been executed.

## Milestone 2 — verified build and interoperability

Run: https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41523561273

Tested implementation commit: `ad5058628a42b46fee35e6ed34fa7873e2eb2e56`.

- Nine Python test methods passed, including all eight fixed reference vectors.
- Host C++ encoder compiled with C++11, warnings enabled and treated as errors.
- Eight fixed vectors plus 200 seeded data cases matched Python byte for byte;
  the Python decoder accepted every C++ frame in fragmented input.
- ESP32-S3 N8R2 UART profile compiled successfully with Arduino CLI 1.3.1 and
  Espressif Arduino core 3.3.0 on the GitHub Linux runner.
- Build report: 303,687 bytes program storage, 20,276 bytes static/global RAM.
  These are compiler figures, not runtime peak-memory measurements.

The target binary was not executed on an ESP32. These results establish buildability
and tested host serialization compatibility, not physical timing or sensor behavior.
The sync CRC coverage still needs Perception agreement. Host fixtures with valid
flags are synthetic test inputs; the firmware demo still marks all readings invalid.

GitHub Actions repeats Python tests, the host C++ comparison and the ESP32 build
on pushes and pull requests. See BUILD.md for the settings and local commands.

## Milestone 3 — MPU-6050 bench software, 3 November 2026

Run: https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41682300906

Tested implementation commit: `eef0df3d32cddea15261cbb27c91a0b8be907697`.

- Existing nine Python tests and 208 C++/Python packet cases passed.
- New host C++ fake-bus suite passed: initialization/readback, signed SI conversion,
  temperature exclusion, timestamps and freshness boundaries, proper rotations,
  read failure/recovery, each gyro/accel clipping rail and initialization failures.
- Both default simulation and optional MPU-6050 firmware modes compiled for ESP32-S3.

No real MPU-6050 was connected. Fake-bus tests do not exercise ESP32 interrupts or
the physical Wire transport. Pin defaults remain unassigned; axis confirmation
defaults to false. Runtime reinitialization, clock accuracy and vehicle tests remain
pending. Details and provisional bench policies are documented in IMU.md.

## Milestone 4 — standalone bench implementation, 3 November 2026

Run: https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41684429013

Tested implementation commit: `77a214ed6fe30d2ec71d7527d67dadffef41310d`.

- 13 Python test methods passed, including recorder parsing and summary handling
  of invalid values, resets, counter rollover, missing and held records.
- 208 cross-language packet cases passed.
- Expanded host IMU suite passed: startup timing, delayed retry, three-read-failure
  recovery, initially missing device recovery, no-data watchdog and runtime audit,
  in addition to prior conversion, clipping, axis and initialization checks.
- Simulation, binary IMU and standalone CSV modes compiled for purchased N16R8.
- CI uploaded the compiled standalone bench files as an artifact.

GPIO8/9/4 now selected; vehicle orientation remains unconfirmed. No live sensor or
COM-port capture ran. Monitor tests cover parsing/statistics, not physical USB.
No measurements of interrupt latency, I2C timing, actual bias, reconnect, power or
vehicle operation are claimed. Hardware tests and power identification are next.
