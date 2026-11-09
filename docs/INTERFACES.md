> Update 9 November 2026: see [accepted decisions and proposed wheel v2](PERCEPTION_DECISIONS_2026_11_09.md). Sync CRC and timing v1 are accepted; 100 Hz is selected.

# Interface decisions and open questions

## Source basis

The supplied State Contract v0.4 IMU-dropout draft has now been reviewed.
STATE_V04_REVIEW.md records required Embedded changes and Pi-owned decisions;
its timing extension supersedes the earlier ambiguous wheel-age reconstruction.

- Control–Embedded preliminary interface v0.1, 26 October 2026.
- Embedded–Perception interface agreement v0.1, 1 November 2026.

These supplied PDFs are references, not signed approvals. Their original files are
not redistributed here. The current implementation covers a subset of their drafts.

## Implemented data encoding

54 bytes, little-endian, 100 Hz target. Field layout is in the root README.
CRC-16/CCITT-FALSE covers bytes 2–51; the checksum is serialized little-endian.
The stream decoder resynchronizes after bad CRCs and supports partial input chunks.
It does not replace Perception's ROS bridge or clock/freshness logic.

## Confirmed sync encoding

14 bytes: A5 5A, type 02, version 01, uint64 boot-relative microseconds, uint16 CRC.
Perception confirmed CRC coverage of bytes 2–11 in the Rev 2 response recorded
on 9 November 2026. Report time remains distinct from acquisition and receipt time.

## Perception acceptance and remaining decisions

Accepted: timing v1 pairing, CRC, 100 Hz stream, count-latch wheel timestamp,
continuous zero-count windows, source-time freshness, repeated steering-write
freshness and boot/sequence handling. The bridge owns ROS publication and mapping.
See PERCEPTION_DECISIONS_2026_11_09.md for the authoritative disposition.

Remaining: exact proposed timing-v2 diagnostics assignment; forward-only operation
with Control; encoder resolution/scale; shared geometry/axes; startup calibration
evidence; real timing/clock validation; and the single vehicle-message header stamp
when wheel and steering have distinct source times.

## Decisions needed with Control

The model outputs steering angle in radians and longitudinal acceleration request
in m/s². This does not establish an ESC pulse, throttle percentage or desired-speed
wire interface. On 3 November 2026 the Embedded team leader confirmed that
**Embedded owns converting the acceleration request into motor/ESC commands**.
The integrated implementation now provides configurable feedforward conversion,
limits and driver adapters. Physical calibration remains unset and unverified.
This ownership decision does not imply Control will send desired speed.

CONTROL_PROTOCOL.md specifies the implemented proposal for command fields,
identity/session rules, expiry, replacement, acknowledgements and restart handling.
INTEGRATED_TEST_PLAN.md clarifies wheel/steering age semantics. Joint approval and
physical validation remain outstanding; these are proposals, not signed contracts.

An 'applied' report means the firmware issued an output; it is not proof of actual
steering position, acceleration or braking. No safe-stopping claim is made here.

## Current simulation behavior

All sensor-valid bits remain false. First data packet has reset_seen set. Synthetic
stationary values are test fixtures, not hardware observations or calibration data.
Actuators are not driven. Physical timing and fault thresholds remain unvalidated.
