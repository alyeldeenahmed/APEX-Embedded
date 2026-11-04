# APEX Embedded

Embedded firmware and interface development for the APEX TT-02 graduation project.

**Current milestone:** integrated embedded software implementation, pending physical
validation and joint interface approval. `--vehicle` includes IMU acquisition,
shaft interrupts, independent telemetry, Control request parsing, session/epoch
handling, replay/expiry enforcement, cancellation, status, acceleration-to-effort
mapping, steering/ESC PWM adapters and task watchdog. Outputs remain disabled and
calibration unset. The original three bench modes remain available.

- [Proposed command wire contract and lifecycle](docs/CONTROL_PROTOCOL.md)
- [Configuration inventory and hardware test plan](docs/INTEGRATED_TEST_PLAN.md)
- `control_link.py`: mixed-stream decoder and explicit one-shot bench client.
- `python tools/build_firmware.py --vehicle`: compile integrated N16R8 firmware.

No physical readings, motor response, timing guarantees or safe-stopping claims
are established by software tests. The single-channel sensor selection cannot
provide measured direction; signed validity requires the documented quadrature setup.

[Milestone 4 passing builds, tests and bench binary artifact](https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41684429013)

[Milestone 3 passing build and tests](https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41682300906)

[Milestone 2 passing build and tests](https://github.com/alyeldeenahmed/APEX-Embedded/actions/runs/41523561273)

## Team handover

- [Progress and next milestones](docs/PROGRESS.md)
- [Planned hardware baseline](docs/HARDWARE.md)
- [Interface decisions and open questions](docs/INTERFACES.md)
- [Validation results](docs/VALIDATION.md)
- [Build settings and packet compatibility checks](docs/BUILD.md)
- [MPU-6050 acquisition prototype and remaining limitations](docs/IMU.md)
- [Standalone IMU wiring and test procedure](docs/IMU_BENCH.md)
- [Change history](CHANGELOG.md)

Related team repositories:

- Control: https://github.com/Maxrwan/MPC_Controller_v1_IronManStyle
- Perception: https://github.com/FatmaAshraf23/grad-perception

The linked teams' software and documents are external references. This repository
does not claim their interfaces have been jointly approved or physically integrated.

## Historical milestone 1: telemetry packet

Status: software prototype. No hardware tests or actuator outputs. The default mode
uses simulated values; the optional IMU mode is documented in docs/IMU.md.

This milestone implements the 54-byte data frame in the Perception v0.1 agreement.
The ESP32 sketch sends simulated values for bench development only. All sensor-valid
bits are deliberately zero, so a real estimator must not treat the data as valid.
Do not connect this prototype to the vehicle's autonomous operating pipeline.

## Files

- `firmware/telemetry_demo/telemetry_demo.ino`: Arduino ESP32-S3 bench sketch.
- `firmware/telemetry_demo/packet.h`: explicit byte serialization and CRC.
- `protocol.py`: computer-side encoder, stream decoder and simulated capture.
- `test_protocol.py`: independent CRC reference and packet/parser tests.

## Computer-only use

Run from this directory with Python 3 (no extra packages):

```
python -m unittest -v
python protocol.py generate demo.bin
python protocol.py decode demo.bin
```

The generated capture represents one second of sample timestamps; generation itself
is not paced in real time and does not establish hardware timing performance.

## ESP32 bench use (later)

Install Arduino IDE and Espressif's ESP32 board support. Open the `telemetry_demo`
sketch, select the exact ESP32-S3 board and its N16R8 memory settings, then build/upload.
The sketch uses `Serial` at 921600 baud and schedules data at 100 Hz and sync at 1 Hz.
The USB/UART route depends on the board configuration and must be verified before use.
The reproducible build profile now selects UART0 via a USB-to-UART bridge; see
`firmware/build-config.json`. Native USB is not enabled by that profile.
There is no debug text on the data port. Loop scheduling is a prototype, not a verified
real-time guarantee; delayed releases are skipped rather than sent as a catch-up burst.

## Data frame (54 bytes, little-endian)

| Offset | Bytes | Field |
|---|---|---|
| 0 | 2 | Sync A5 5A |
| 2 | 1 | Type 01 |
| 3 | 1 | Version 01 |
| 4 | 1 | Flags |
| 5 | 1 | Reserved, zero |
| 6 | 2 | Sequence, unsigned, wraps modulo 65536 |
| 8 | 4 | IMU sample time in microseconds, low 32 bits |
| 12 | 12 | Gyro x/y/z, float32 rad/s, no bias removal |
| 24 | 12 | Acceleration x/y/z, float32 m/s², includes gravity |
| 36 | 4 | Cumulative signed encoder ticks |
| 40 | 4 | Wheel speed float32 m/s |
| 44 | 2 | Wheel sample age, microseconds |
| 46 | 4 | Road-wheel angle float32 radians, positive left |
| 50 | 2 | Steering sample age, microseconds |
| 52 | 2 | CRC-16/CCITT-FALSE over bytes 2–51 |

CRC parameters: polynomial 0x1021, initial 0xFFFF, no reflection, xor-out zero.
Flags: bit 0 IMU valid, 1 wheel valid, 2 steering valid, 3 calibrating,
4 standstill, 5 steering measured, 6 reset seen; bit 7 reserved.

## Explicit provisional choices

- The PDF specifies a 14-byte sync frame but not its CRC coverage. This prototype
  uses the same CRC algorithm over bytes 2–11 (type, version, uint64 timestamp).
  Confirm this with Perception before freezing the interface.
- Sync timestamps use the same boot-relative microsecond clock as data timestamps.
- The simulated first packet has reset_seen set. Validity flags remain false.
- The decoder checks framing/CRC and displays values; it is not Perception's ROS
  bridge, clock mapping, freshness monitor or EKF.
- Real sample timestamps must come from sensor acquisition. The sketch's timestamps
  represent synthetic sample generation only.

## Remaining physical and cross-team work

Run on the selected board; measure timing, sensor behavior and all calibration
parameters; confirm the proposed age/sync/command semantics with the teams;
integrate the Pi-owned ROS bridge and Safety supervisor. Actuator adapter software
exists in integrated mode, but outputs cannot be enabled with default calibration.
