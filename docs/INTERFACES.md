# Interface decisions and open questions

## Source basis

- Control–Embedded preliminary interface v0.1, 26 October 2026.
- Embedded–Perception interface agreement v0.1, 1 November 2026.

These supplied PDFs are references, not signed approvals. Their original files are
not redistributed here. The current implementation covers a subset of their drafts.

## Implemented data encoding

54 bytes, little-endian, 100 Hz target. Field layout is in the root README.
CRC-16/CCITT-FALSE covers bytes 2–51; the checksum is serialized little-endian.
The stream decoder resynchronizes after bad CRCs and supports partial input chunks.
It does not replace Perception's ROS bridge or clock/freshness logic.

## Provisional sync encoding

14 bytes: A5 5A, type 02, version 01, uint64 boot-relative microseconds, uint16 CRC.
**CRC coverage of bytes 2–11 is an implementation proposal, not explicit in the PDF.**
Perception must confirm the CRC coverage and timestamp sampling semantics.

## Decisions needed with Perception

1. Wheel timestamp: count-latch instant versus centre of the speed-estimation window.
2. Freshness when shaft pulses are sparse at low speed or absent at standstill.
3. Freshness of an unchanged commanded steering angle.
4. Age-field saturation and invalid-value handling, including steering ages.
5. Normal sequence/time wrap versus restart detection, including loss of the first
   reset-marked packet.
6. Startup initialization and the start of the two-second calibration interval.
7. Sensor settings, timestamp mapping tests and ownership of shared calibration.
8. Measured direction versus inferred direction for signed shaft-derived speed.

## Decisions needed with Control

The model outputs steering angle in radians and longitudinal acceleration request
in m/s². This does not establish an ESC pulse, throttle percentage or desired-speed
wire interface. Agree conversion ownership before implementing motor control.

Agree command fields/units, transmission rate, identity/session rules, expiry,
queue/replacement behavior, acknowledgements, cancellation and restart handling.
Define local watchdog and held-output behavior jointly and validate on hardware.

An 'applied' report means the firmware issued an output; it is not proof of actual
steering position, acceleration or braking. No safe-stopping claim is made here.

## Current simulation behavior

All sensor-valid bits remain false. First data packet has reset_seen set. Synthetic
stationary values are test fixtures, not hardware observations or calibration data.
Actuators are not driven. Physical timing and fault thresholds remain unvalidated.
