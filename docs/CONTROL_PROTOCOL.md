# Proposed Control–Embedded protocol v0.2 (implementation proposal)

Baseline: supplied Control v0.1 and Embedded–Perception v0.1 PDFs. The team leader
authorized proposing missing details on 4 November 2026. This is not joint approval.
Control sends road-wheel steering radians (left positive) and requested longitudinal
force / modeled mass in m/s². Embedded converts these into actuator signals.

## Transport and ownership

Integrated telemetry now also includes a paired 42-byte type-03 timing companion.
See STATE_V04_REVIEW.md. Receivers must support this extension. Raw-sensor validity
does not authorize driving in state DEGRADED/LOST; Control/Safety must enforce its
accepted state contract independently of the MCU command deadline.

921600 baud, 8N1, ESP UART0 via COM USB connector. Full duplex. One Pi process must
own this port and demultiplex telemetry (01), sync (02), and status (11 hex).
The existing 54-byte telemetry and 14-byte sync frames retain their formats.
No ROS dependency runs on the ESP32. Perception owns its ROS topics and clock map;
`control_link.Stream` provides a reusable mixed-frame decoder for that bridge.
The bench CLI is not an autonomous Safety supervisor and must not run concurrently
with another serial-port owner. Opening the port may reset the MCU.

## Telemetry receipts and link loss · 27 November 2026

Use the updated ESP firmware and PySerial client together. A complete report is a
type 01 data frame followed by a CRC-valid type 03 timing frame with the same
16-bit sequence in the same confirmed boot. The Pi replies once per matched pair,
including pairs reporting invalid sensors. At 100 Hz this typically means 100
receipts per second. Sync, status, commands, CRC failures and orphans generate no receipt.

| Pi → ESP offset | Encoding | Value |
|---|---|---|
| 0–3 | bytes | A5 5A 12 01 |
| 4–7 | uint32 LE | Boot nonce from current receipt-capable status |
| 8–9 | uint16 LE | Pair sequence |
| 10–17 | uint64 LE | Exact timing frame report_us |
| 18–19 | uint16 LE | CRC-16/CCITT-FALSE over bytes 2–17 |

The ESP admits only full UART writes of the 96-byte pair to a four-entry window.
A matching fresh receipt acknowledges all outstanding entries through its match.
The boot nonce and report time distinguish sequence wrap and old traffic. Entering
the fourth outstanding entry trips locally. Complete failure to queue a pair for
four configured report periods also trips. At the 100 Hz default the separate
transmit-stall guard is 40 ms. Partial writes increment the drop counter.

On loss the ESP revokes the session, sends calibrated ESC neutral, suppresses
steering pulses and attempts to send code 17 (link_lost). Periodic status continues
to expose the fault and window depth while the link is lost. An acknowledgement
of a recent pair at or after the trip can restore link health. A fresh authority
context and explicit ARM are required for motion. No receipt extends a command
deadline. Pi integration must service the serial port continuously while MPC or
ROS is idle. Use the single-owner instructions in INTEGRATION_START_HERE.md.

## Optional wheel-speed detail

The serial owner can also request wheel-speed diagnostics by writing a 6-byte
frame: bytes 0–3 are A5 5A 13 01, bytes 4–5 are the little-endian CRC over
bytes 2–3. The MCU accepts this request only with a healthy receipt link. It
enables optional type 04 frames for two seconds. The updated viewer renews
the request once per second. This request cannot acknowledge a pair, extend an
actuator deadline, install a session or arm. Link loss clears the subscription.

When there is UART space, a 54-byte type 04 frame immediately precedes the
regular 01/03 pair. A receiver matches boot, sequence and 64-bit report time.
All quantities below are little-endian. CRC covers bytes 2–51.

| Offset | Type | Wheel-detail field |
|---|---|---|
| 0–3 | bytes | A5 5A 04 01 |
| 4, 8, 10 | uint32, uint16, uint64 | Boot nonce, pair sequence, report time in µs |
| 18–19 | uint8, uint8 | Bit 0 short measurement present, bit 1 encoder suspect. Second byte reserved zero. |
| 20, 28 | uint64, uint64 | Start and end of the contiguous measurement in ESP µs |
| 36 | float32 | Signed nominal speed in m/s, meaningful with bit 0 set |
| 40, 44, 48 | uint32 each | I²C read failures, excessive acquisition intervals, ambiguous half-turns since boot |
| 52 | uint16 | CRC-16/CCITT-FALSE |

The original 01/03 pair retains its existing meaning. Receivers use the type 04
measurement only as a separately labelled commissioning observation. See
[speed states and freshness](../INTEGRATION_START_HERE.md#reading-speed-on-the-pi).

## Control request (64 bytes)

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
Control's reviewed tracker is nominally 100 Hz. A tracker-aligned SET stream at
nominal 100 Hz is the adapter proposal; it is not an approved wire schedule.
The MCU receives commands independently of its 50 Hz physical PWM refresh.
Its <=100 ms receive-relative deadline bound is not Control's trajectory horizon.
Omar must resolve source authority/expiry and physical clock mapping; see
INTERFACE_AUDIT_2026_11_09.md. The MCU does not invent commands when the Pi falls silent.

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
   strictly increase even after revocation. New session numbers must increase
   (no session/epoch wrap within one boot). All session changes retain global sequence.
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
enabled, bit4 receipt link lost and bit5 receipt-watchdog capability. Byte 6 is
the outstanding-pair count (0–4) and byte 7 is zero. Legacy status lacks bit5.
Offsets 8/12/16/20 are boot/latest-consumed sequence/session/
epoch. Offset 24 is response command ID; 28 last issued command ID (historical when
disarmed). Offset 32 uint64 report time; 40 uint64 last initial command-issuance
time. Offsets 48/52 are current commanded angle and normalized effort. Offset 56
uint16 saturated parser-error count; offset 58 uint32 transport-drop count.

Codes in numeric order 0–17: IDLE, INSTALLED, ACCEPTED, APPLIED, SIMULATED,
CANCELLED, REVOKED, BAD_BOOT, REPLAY, BAD_CONTEXT, BAD_TIME, BAD_VALUE, NOT_READY,
NOT_ARMED, UNKNOWN_TARGET, FAULT, EXPIRED, LINK_LOST.

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

When physically enabled and calibrated, disarm requests a calibrated neutral ESC
pulse and holds the servo signal low. ARM at zero requests steering centre.
A task watchdog requests MCU reset after 250 ms of
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
