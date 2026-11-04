# Proposed Control–Embedded protocol v0.2 (implementation proposal)

Baseline: supplied Control v0.1 and Embedded–Perception v0.1 PDFs. The team leader
authorized proposing missing details on 4 November 2026. This is not joint approval.
Control sends road-wheel steering radians (left positive) and requested longitudinal
force / modeled mass in m/s². Embedded converts these into actuator signals.

## Transport and ownership

921600 baud, 8N1, ESP UART0 via COM USB connector. Full duplex. One Pi process must
own this port and demultiplex telemetry (01), sync (02), and status (11 hex).
The existing 54-byte telemetry and 14-byte sync frames retain their formats.
No ROS dependency runs on the ESP32. Perception owns its ROS topics and clock map;
`control_link.Stream` provides a reusable mixed-frame decoder for that bridge.
The bench CLI is not an autonomous Safety supervisor and must not run concurrently
with another serial-port owner. Opening the port may reset the MCU.

## Request (64 bytes)

All multibyte values little-endian; float values IEEE-754 float32, finite only.

| Offset | Type | Meaning |
|---|---|---|
| 0 | 2 bytes | A5 5A |
| 2 | uint8 | 10 hex |
| 3 | uint8 | Version 1 |
| 4 | uint8 | INSTALL=1, ARM=2, SET=3, CANCEL=4, REVOKE=5 |
| 5 | 3 bytes | Reserved zero |
| 8 | uint32 | Boot nonce from MCU status |
| 12 | uint32 | Global request sequence for this MCU boot |
| 16 | uint32 | Session |
| 20 | uint32 | Authorization epoch |
| 24 | uint32 | Nonzero command ID; Pi must use unique IDs within session |
| 28 | uint32 | Cancellation target command ID |
| 32 | uint64 | Creation time mapped into MCU boot microseconds |
| 40 | uint64 | Exclusive command/output expiry in MCU microseconds |
| 48 | float32 | Steering radians |
| 52 | float32 | Acceleration request m/s² |
| 56 | 6 bytes | Reserved zero |
| 62 | uint16 | CRC-16/CCITT-FALSE over bytes 2–61 |

Only SET carries nonzero steering/acceleration. Arrival time must be at or after
creation and no more than 100 ms later. Deadline must be strictly after arrival,
at most 100 ms ahead, and after creation. These are provisional local bounds.
Pi must account conservatively for clock-map uncertainty and transport delay.
Future scheduled commands are rejected; no queue of trajectories is accepted.

Matching-boot requests consume a fresh sequence even when subsequently rejected.
Fresh means modular difference in [1, 2^31-1]; gaps are allowed. CRC-invalid frames
produce no acknowledgement or sequence change. A dropped ACK is an unknown outcome;
repeating its sequence gives REPLAY, not another execution. Status periodically
repeats the latest result and held ID. Command IDs correlate reports, while sequence
enforces replay rejection. CRC and boot nonce do not authenticate a sender.

## Lifecycle and application

1. Wait for status containing boot nonce and boot time (sent every 100 ms).
2. INSTALL a nonzero session/epoch. It disarms, clears pending output and held ID,
   then reports INSTALLED after the driver is serviced. Same-session epoch must
   strictly increase while installed. All session changes retain global sequence.
3. ARM explicitly at zero steering/acceleration. It requires confirmed calibration,
   valid forward-direction wheel feedback, driver readiness and task-watchdog setup.
4. SET replaces the requested output. The service loop applies it immediately after
   validation, using elapsed-time steering-rate limits. No future queue exists.
   Further ticks continue ramping steering toward the request until expiry.
5. CANCEL an unissued pending target yields CANCELLED. Cancelling the active issued
   target yields REVOKED and disarms; previous physical effects cannot be undone.
   Unknown or superseded targets yield UNKNOWN_TARGET. REVOKE disarms the whole
   installed context and requires INSTALL before rearming.

An accepted ARM/SET yields APPLIED only after successful driver writes, or SIMULATED
when outputs are disabled. ACCEPTED may be observed by tests before the service tick;
it is never proof of issuance. Driver failure yields FAULT. Protocol rejection does
not renew existing authority. Command expiry or invalid wheel feedback disarms and
requires explicit rearming. Once expired, a fresh SET cannot silently rearm.

## Status (64 bytes, type 11 hex)

Offsets 0–3 and CRC are as above. Offset 4 is result code, offset 5 flags:
bit0 context installed, bit1 armed, bit2 physical adapter available, bit3 output
enabled. Bytes 6–7 zero. Offsets 8/12/16/20 are boot/latest-consumed sequence/session/
epoch. Offset 24 is response command ID; 28 last issued command ID (historical when
disarmed). Offset 32 uint64 report time; 40 uint64 last initial command-issuance
time. Offsets 48/52 are current commanded angle and normalized effort. Offset 56
uint16 saturated parser-error count; offset 58 uint32 transport-drop count.

Codes in numeric order 0–16: IDLE, INSTALLED, ACCEPTED, APPLIED, SIMULATED,
CANCELLED, REVOKED, BAD_BOOT, REPLAY, BAD_CONTEXT, BAD_TIME, BAD_VALUE, NOT_READY,
NOT_ARMED, UNKNOWN_TARGET, FAULT, EXPIRED.

APPLIED means API writes succeeded, not pulse-edge timing, measured steering,
achieved acceleration or brake force. Angle may continue ramping after that time;
status shows the current requested driver output. Expiry/fault reports retain the
last response ID; inspect code/flags, not ID alone. Historical IDs do not imply
output is still enabled. Lost reports cannot be reconstructed as physical facts.

## Actuator policy

Forward-only drive/brake: negative effort means brake, never reverse. Measured
reverse shaft motion inhibits arming/output. This requires verifying that the ESC
is configured for brake-only behavior; a generic reversing ESC mode is unsupported.
Initial mapping is calibrated feedforward: effort = a_cmd times separately measured
drive/brake coefficients, bounded [-1,1]. This is not an acceleration feedback loop.
No integral of a_cmd is substituted for a desired-speed request.

Steering uses calibrated left/center/right pulse values and maximum road-wheel
angle, with an elapsed-time angle rate limit. ESC uses neutral/full-drive/full-brake
pulse endpoints. Zero/default calibration cannot arm. Both mappings assume an
adequate linear calibration over the intended operating envelope; hardware tests
must determine whether speed/battery-dependent tables are required.

When enabled and calibrated, fault/disarm requests neutral ESC and centered servo.
PWM continues at these values. A task watchdog requests MCU reset after 250 ms of
an unresponsive loop; this is separate from the <=100 ms command expiry checks.
PWM hardware can hold its previous signal during a stalled loop until reset.
No safe-stopping or reset-pin electrical behavior is claimed. Independent physical
stopping/power evidence is mandatory before vehicle operation.

References for implemented peripheral APIs:
- https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/wdts.html

## Bench client

`python control_link.py --port COM7` monitors status only. With `--action install
--session 1 --epoch 1 --id 1` it sends exactly one explicit request after observing
status. ARM/SET/CANCEL/REVOKE also require explicit actions. This CLI does not maintain
an arm lease, retry commands, or implement a Pi Safety supervisor. A persistent Pi
connection is required for a real multi-command session. Use the codec from that
single port-owning process, with a tested clock mapper, rather than opening a new
serial connection for every command.
