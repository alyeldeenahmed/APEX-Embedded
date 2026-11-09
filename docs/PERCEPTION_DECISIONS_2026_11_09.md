# Perception integration decisions and remaining gates

> Later Control reply on 9 November rejects unmeasured wheel direction. Keep
> forward_only_control_confirmed=false; quadrature is the integration plan. The
> v2 magnitude-only option below remains an offline development facility, not an
> approved driving mode. See INTERFACE_AUDIT_2026_11_09.md for current owners,
> repository comparison and still-pending ROS/clock/recovery decisions.

9 November 2026. Source: team leader supplied Perception Rev 2 response; this
supersedes their earlier response. No message has been sent on the team's behalf.

## Accepted by Perception

100 Hz first; type-03 v1 layout/pairing; sync CRC bytes 2-11 (data 2-51,
timing 2-39). Pair within confirmed boot and discard/count mismatches. Wheel
source time is the actual count-latch time; Perception applies half-window delay (nominal 5 ms; actual latch spacing can vary).
A zero-count window continues at 100 Hz. Freshness requires source time advancement.
MCU intervals >25 ms are invalid; Perception wheel stale/DEGRADED at 50 ms and
LOST at 1 s without fresh valid data. No standstill exception. IMU LOST remains
50 ms. These Pi state transitions are not implemented as MCU estimator logic.

Perception owns apex_esp_bridge and /dev/apex_esp. /imu/data contains raw SI gyro
and acceleration with bias/gravity retained; /vehicle/measured contains speed
and commanded road-wheel angle. Source/report/receipt times remain separate.
Clock mapping is Perception's responsibility; one-way delay and uncertainty
require hardware validation. imu_gap_predict remains OFF. Schema 1.2 age fields
are agreed in principle, implementation pending; no operational adoption of
corner uncertainty or 50 Hz fallback is claimed.

## Implemented extension, awaiting peer acceptance

To avoid silently changing the already-accepted v1 reserved byte, type-03 VERSION
2 is proposed. Length remains 42 bytes; offsets, source times, pairing and CRC
coverage unchanged. Byte 7 becomes:

| Bit | Meaning |
|---|---|
| 0 | direction_measured: confirmed quadrature configuration |
| 1 | motion_feedback_suspect: latched commanded-drive/no-edge fault |
| 2 | motion monitor configured/enabled (not proof of sensor health) |
| 3-7 | reserved zero |

Type-01 remains version 1. Under a v2 pair, wheel_valid means calibrated speed
magnitude available; speed is signed only when direction_measured is true. A
negative speed without that bit is rejected by the event adapter. A suspect
flag suppresses wheel publication even if a contradictory valid bit arrives.
A v1 wheel event exposes direction_measured=null: do not infer direction support
from older captures. Both Python decoders accept timing v1/v2; old decoders need
updating before v2 use. Default firmware still emits timing v1. Build opt-in:
`python tools/build_firmware.py --vehicle --wheel-diagnostics-v2`.

## Exact validity and actuator gates

IMU: axes confirmation AND driver-valid, finite/non-clipped, fresh sample (current local freshness ceiling: 5 ms; late acquisition read >=1 ms invalidated). Physical
orientation is still unknown, so vehicle-frame imu_valid remains false by default.
Use bench raw recordings before axes acceptance. Driver timestamp/late-read checks
are retained. No bias removal or false validity is added to satisfy a publisher.

Wheel magnitude: initialized previous latch, increasing timestamp, interval <=25 ms,
no new encoder errors, confirmed scale with finite positive metres/count, and no
latched suspect fault. Finite computed speed is required. Zero counts are allowed.
Legacy v1 additionally requires quadrature. V2 allows single-channel magnitude.
A valid magnitude is not proof that an idle sensor is connected.

Motor health remains separate: a configured monitor is required in BOTH modes,
plus quadrature OR explicit forward-only Control confirmation with v2. Existing calibration, watchdog,
wheel freshness and output gates still apply. Default outputs remain disabled,
scale zero, wheel unconfirmed, forward-only unconfirmed. No automatic ARM added.

Motion monitor examines the PREVIOUS successfully applied physical ESC effort,
not merely a requested acceleration. Only positive effort strictly above the
configured normalized threshold starts the timer. Neutral/braking/disabled output
resets a pending timer; valid observed edges restart it. Net-zero quadrature counts
can still contain observed edges. At timeout the suspect latch clears wheel validity
and the existing endpoint faults/disarms. The latch survives neutral and new edges;
only reboot clears it in this initial proposal. Reboot still requires authorization
and calibration to ARM. It detects missing expected feedback, not which part failed.
Threshold=0 and timeout=0 disable the diagnostic until calibrated. Monitor timeout
must include startup/deadband and pulse resolution; no unmeasured defaults adopted.
Fault response is the existing neutral/disarm path, NOT validated active braking.
Control must accept response/recovery before powered deployment.

## Encoder proposal (not purchased or mechanically approved)

Propose >=400 effective counts per wheel revolution for <=0.0475 m/s increments
at nominal 10 ms, using Perception's illustrative circumference 0.19 m. 800 counts
would give 0.02375 m/s. These are requirements proposals, not measured wheel size.
Counts = shaft cycles/revolution * shaft/wheel ratio * decoding multiplier.
Quadrature x4 requires correctly phased A/B, not simply two sensors. Determine
ratio, target geometry, minimum speed and max edge rate before choosing hardware.
If infeasible, jointly review period/hybrid estimation and its delay semantics.

## Remaining agreements

- Perception: accept exact v2 version/bit assignment and update bridge decoder.
- Control: command/status ROS relay through the sole port owner; topic/type/QoS,
  unchanged deadlines/IDs, bounded queues, no stale replay after reconnect.
- Control: forward-only policy, rollback/reverse handling, fault action and recovery.
- Perception: /vehicle/measured has one header stamp but two source times. Agree
  wheel stamp and separate steering time/validity representation; do not imply
  the steering write occurred at the wheel latch.
- Teams: encoder resolution target and acceptable low-speed quantization/delay.

## Hardware evidence checklist

[ ] IMU axes and rigid mounting checked; shared reference frame measured.
[ ] IMU/LiDAR/front/rear axle positions recorded in config/vehicle_geometry.json.
[ ] Wheel circumference, shaft ratio, counts, sign and filtering measured.
[ ] Real binary capture, including standstill and hand-turned shaft.
[ ] Host receipt timing AND device timestamps logged; assess 12/20 ms packet targets.
[ ] Stationary gyro bias/noise measured; no synthetic capture presented as evidence.
[ ] ESC arming, servo centre, angle/PWM and minimum drive effort measured.
[ ] No-edge timeout tested: motion, blockage, unplug, restart; safe fault response.
[ ] At least 20 steering steps each direction at each of 1/2/3 m/s (120 minimum),
      only after basic safe running. Record write time, gyro z, speed, configuration.
[ ] Clock delay/jitter, brownout/reset and sensor dropout/recovery measured.

Software verification evidence is build/software-validation.json. Host tests do
not validate physical acquisition, ESP scheduling or Perception's estimator.

## Sampling clarification and capture tooling

The 100 Hz agreement is the stream profile. Current MPU configuration remains
nominal 1 kHz internal acquisition with CONFIG=3 (gyro DLPF 42 Hz); the stream
reports latest readings. This is not a claim of 1 kHz delivery to the Pi or of
lossless internal sampling. Perception should not assume SMPLRT_DIV=9.

A read-only capture helper is ready; it has NOT been run against hardware:
`python tools/capture_serial.py --port COM5 --seconds 30 --output captures/first_real`
Replace COM5 with the verified port. On Pi use /dev/apex_esp ONLY with the bridge
stopped/releasing ownership. Install requirements-bench.txt first. Outputs are raw
bytes, host read start/end timestamps with byte offsets, and completion metadata.
Opening a USB serial device can reset the ESP even without commands. No ARM/write
commands are sent. Preserve original raw capture and metadata together. Host read
times include USB/OS batching, so cannot establish true wire jitter alone. Replay
raw.bin with tools/replay_capture.py; record receipt timing separately for backlog
analysis. The existing imu_monitor.py bench path provides stationary statistics.

## Concrete relay proposal for joint review (not implemented here)

- Control -> bridge: /apex/embedded/command_frame, std_msgs/msg/UInt8MultiArray,
  containing exactly one complete 64-byte type-0x10 frame; layout field empty.
- Bridge -> Control: /apex/embedded/status_frame, same ROS type, exactly one
  complete CRC-checked 64-byte type-0x11 frame; layout field empty.
- Proposal: reliable, volatile durability, bounded depth 10; no retained commands
  on startup, no application retry/replay across reconnect/boot/session changes.
- Validate type/length/CRC and preserve bytes, command identifiers and original MCU
  deadlines. A bounded queue is not deadline safety: expired frames must be dropped,
  and the existing MCU deadline check remains authoritative. Queue-full rejection
  must be visible to Control, never silently reported as applied.
- Control authorizes and creates frames; Perception owns serial I/O, mappings and
  relay. Motor response is confirmed only by status, not successful ROS publication.
- The optional Embedded Python client is a standalone bench tool; it must not open
  the device concurrently with apex_esp_bridge. No live ROS node was added by this work.
