# Change history

## 0.4.0 — 2026-11-03 — standalone IMU bench package

- Updated board to purchased ESP32-S3 DevKitC-1 N16R8; selected GPIO8/9/4.
- Nonblocking sensor startup, retry/reinitialization, missing-data watchdog and
  periodic configuration audit, with diagnostic counters and fault reporting.
- Separate CSV bench mode, live recorder, offline summaries and monitor tests.
- Standalone wiring/test guide; hardware verification remains pending.

## 0.3.0 — 2026-11-03 — MPU-6050 bench acquisition

- Optional MPU-6050 mode with verified-register setup, burst reads, data-ready
  timestamps, SI conversion, clipping/freshness checks and axis-map validation.
- Fake-bus tests and separate ESP32 compilation for sensor mode.
- Unassigned pins and unconfirmed orientation keep hardware use explicit.
- Recorded Embedded ownership of acceleration-to-motor conversion.
- Hardware timing, sensor recovery and full startup integration remain pending.

## 0.2.0 — 2026-11-02 — build and interoperability checks

- Pinned ESP32-S3 N8R2 bench build configuration (Arduino core 3.3.0).
- Eight reusable fixed packet examples and 200 seeded C++/Python comparisons.
- Host encoder compilation and target firmware compilation in GitHub Actions.
- Build guide with explicit USB/UART settings and hardware-validation boundaries.

## 0.1.0 — 2026-11-02 — initial prototype

- Data packet encoder and stream decoder.
- ESP32-S3 Arduino simulation sketch and explicit C++ serialization.
- Provisional time-sync packet.
- Eight computer-side tests.
- Hardware baseline, responsibility boundaries, open interface decisions and
  validation limitations documented for the other teams.
# 4 November 2026 — integrated vehicle software

- Integrated shaft capture and count-window speed, with signed validity gated by
  quadrature and confirmed scale; zero-pulse reports continue at standstill.
- Added proposed command/status frames, bounded parser, boot/session/epoch checks,
  replay protection, explicit arming, expiry, cancellation and revocation.
- Connected configurable acceleration feedforward and steering-rate logic to PWM
  adapters; defaults leave all actuator outputs disabled and calibration unset.
- Added task watchdog, nonblocking telemetry writes, host codec/bench client,
  endpoint tests, integrated ESP32 build and hardware handover plan.
- No hardware validation or joint interface approval is claimed.

# 4 November 2026 — State v0.4 alignment

- Reviewed supplied IMU-dropout state contract; kept EKF/trust/AMCL policy on Pi.
- Added paired type-03 independent sensor timestamps for unambiguous wheel-driven
  prediction during IMU dropout; new wire extension requires bridge agreement.
- Added raw event adapter suppressing invalid/duplicate IMU observations, preserving
  source and receipt provenance, and resetting on boot identity changes.
- Added selectable 200 Hz telemetry build, retaining independent 100 Hz wheel capture.
- Added dropout, cadence, rollover, orphan and timestamp tests and timing byte comparisons.


## 9 November 2026 - Perception follow-up (local implementation)

- Recorded accepted Rev 2 interface decisions and outstanding Control agreements.
- Added opt-in timing v2 direction/motion diagnostics, preserving v1 as default.
- Separated magnitude validity from direction authorization; added calibrated,
  physical-drive no-edge monitor with latched fault and existing disarm response.
- Updated both Python stream decoders and event metadata; backwards report time
  requires boot confirmation, overwritten orphan pairs are counted.
- Added C++ safety/protocol tests, Python cases, simulated v2 capture and geometry
  template. Added sixth ESP32 build profile for v2. Hardware remains untested.
