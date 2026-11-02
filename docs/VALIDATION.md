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
nor sensor accuracy. The C++ serializer and ESP32 sketch have not been compiled or
cross-checked on hardware. No actuator or vehicle acceptance test has been executed.

GitHub Actions is configured to repeat the Python tests on pushes and pull requests;
no remote run is claimed until the repository has been published and CI has run.
