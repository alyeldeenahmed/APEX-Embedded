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
