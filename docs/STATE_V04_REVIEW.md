# State contract v0.4 review and Embedded changes

Reviewed 4 November 2026: supplied seven-page "APEX State Contract v0.4 draft IMU
dropout", dated 27 October 2026. This is a Perception-to-Control state contract,
not a replacement MCU wire specification. Its simulator results are not vehicle
measurements. The file labels the change draft and retains open questions; the
team leader previously reported Control approval, but the exact disposition of
those open questions must still be recorded by the teams.

## Required support and disposition

| Contract item | Embedded action / owner |
|---|---|
| Wheel-driven prediction during IMU dropout (pp. 1-3) | Preserve wheel telemetry, including zero speed, independently of IMU availability. Already implemented. |
| State stamp becomes newest fused sample, including wheel (pp. 1-2) | Added independent 64-bit acquisition/output timestamps in the timing companion below. The old IMU stamp plus a wheel age was ambiguous during dropout. |
| Held/invalid/returning gyro must not falsely restore freshness (pp. 2-3) | Added raw sensor-event adapter that never emits invalid or duplicate IMU observations. It does not mark returning samples trusted. |
| 200 Hz planned on car; wheel rate during dropout (p. 2) | Added selectable 200 Hz telemetry profile; wheel acquisition remains nominal 100 Hz. Default 100 Hz retained pending rate agreement. Adapter deduplicates wheel samples in 200 Hz packets. ESP32 replaces the document's STM32 reference. |
| 25 ms dropout, 50 ms LOST, 10-sample recovery (pp. 1-3) | Perception state machine, not MCU sensor-valid flags. No duplicate EKF/trust logic added to ESP32. |
| Fresh accepted AMCL after movement; no ZUPT before gyro validation (p. 3) | Perception responsibilities. The adapter supplies raw events only and never declares OK, rest or trusted gyro. |
| Commanded steering and servo lag (p. 6) | Preserve commanded-angle label; add actual successful driver-write timestamp. Physical steering lag/mapping remain to be measured. |
| Backlog distinction (pp. 3, 6) | Adapter retains source, report and host receipt times separately; never replaces source time with receipt time. Pi needs ordered callbacks and backlog-aware logic. |

## Timing companion: MCU protocol extension proposal

Integrated vehicle mode now queues the unchanged 54-byte type-01 data frame and
one 42-byte type-03 timing frame together (96 bytes). They have the same sequence.
Receivers must explicitly support this extension; do not infer deployment approval
from the fact that the old data-frame layout remains unchanged. Bench-only modes
retain their existing stream unless using integrated mode.

| Offset | Type | Meaning |
|---|---|---|
| 0 | 2 bytes | A5 5A |
| 2 / 3 | uint8 / uint8 | type 03 / version 01 |
| 4 | uint16 | paired data sequence |
| 6 | uint8 | presence: bit0 IMU acquired, bit1 wheel latched, bit2 physical steering write |
| 7 | uint8 | reserved zero |
| 8 | uint64 | report snapshot time, MCU microseconds |
| 16 | uint64 | last IMU data-ready sample time; retained if held/invalid |
| 24 | uint64 | wheel count-latch time (end of speed averaging window) |
| 32 | uint64 | last successful physical servo/ESC API write time; not servo measurement |
| 40 | uint16 | CRC-16/CCITT-FALSE over bytes 2-39 |

All fields little-endian. Presence is separate from the data frame's validity
bits. Absent source timestamps are zero. The report time is not wire-arrival time.
Pair by adjacent validated frames and sequence within a confirmed boot; discard
orphan/mismatched pairs. `protocol.Decoder` and `control_link.Stream` support the
new frame. `sensor_events.SensorEvents` is an optional pure-Python adapter for the
single-owner Pi bridge. It waits for boot status, resets on boot change, checks
pair consistency, preserves 64-bit source time and emits only fresh valid source
events. An IMU event is a candidate, not proof of model agreement or gyro trust.

Wheel/steering ages in the legacy frame are measured relative to report time and
saturate at 65535 us. The companion removes the need to reconstruct wheel time
from the held IMU timestamp, host arrival time, or saturated ages.

## Keep these layers distinct

- MCU bit1 means wheel-valid. State FLAG_IMU_STALE bit1 has a different meaning.
  Never copy the MCU bitmask directly into the Perception state flags.
- State stamp_us belongs to the fused estimate. Do not relabel raw IMU data with
  a wheel timestamp or replace the raw gyro with the bicycle-model yaw rate.
- The 50 ms state valid_for_ms / LOST rule is not the proposed <=100 ms MCU command
  expiry or 250 ms task watchdog. Control/Safety must gate commands using current
  state health; live raw sensor packets do not authorize degraded driving.
- Optional imu_age_ms counts time since the last *trusted* gyro, which only
  Perception can determine. If adopted, Perception/Control change state schema
  to 1.2. No state schema change is made by this MCU extension.
- A successful MPU read can restore sensor validity, but must not clear state
  IMU_STALE. Perception still requires ten consecutive model-consistent samples
  (the page-3 diagram resets the count for disagreement over 0.5 rad/s), then the
  appropriate AMCL recovery gate. At 200 Hz, sample-count recovery takes a different
  elapsed time from the simulator's 100 Hz; revalidate it rather than copying 90 ms.

## Integration decisions and hardware evidence still needed

1. Approve the type-03 extension and Pi demultiplexer; select 100 or planned 200 Hz.
   Build planned profile with `python tools/build_firmware.py --vehicle --telemetry-hz 200`.
   Nominal wheel rate remains 100 Hz, independent of telemetry rate. A freshness
   threshold of 10 ms has little margin at 100 Hz; measure actual interarrival and
   acquisition spacing before freezing thresholds.
2. Record decisions on stamp semantics, optional imu_age_ms/schema, 50 ms LOST,
   corner uncertainty, wheel-stale mode, fallback rate acceptance and operational
   adoption. These are the six questions on pp. 6-7, not unilateral MCU choices.
3. Measure wheel direction/scale, servo lag, actual sample spacing/jitter and filter
   delay. Fit wheel-model/understeer parameters using real data, not simulator values.
4. Exercise real IMU loss/recovery at standstill and controlled low speed; delayed
   processing/backlog; wheel loss; timestamp/sequence wrap; resets; intermittent and
   inconsistent returning gyro. Check Perception never restores OK from one sample
   and requires fresh AMCL when appropriate. Keep imu_gap_predict off on the car
   until the teams' adoption evidence and authorization are recorded.

Software tests cover a one-second IMU-invalid stream with continuing zero-speed
wheel samples, duplicate suppression, 200/100 Hz source cadence, orphan frames,
future/backward timestamps, low-32-bit wrap and boot changes. These tests do not
execute or validate Perception's EKF, model residuals, AMCL gate or uncertainty.
