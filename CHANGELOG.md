# Change history

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
