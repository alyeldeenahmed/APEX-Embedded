# Offline software handover

No hardware connection is required. These tools do not implement ROS, Pi clock
synchronization, the state estimator or gyro trust recovery.

## Validation

Run `python tools/validate_software.py --firmware` from the repository root.
Python tests, C++/Python packet comparisons, three host C++ suites and five firmware
profiles are checked. Missing compilers are BLOCKED and produce a nonzero exit code;
they are never reported as passing. Without --firmware, target builds are not requested.
The JSON evidence includes revision, dirty paths, individual output and hardware=false.
Do not publish a dirty-tree report as proof of a clean commit's CI results.

## Binary replay

Run `python tools/replay_capture.py captures/input.bin captures/events.jsonl`.
The output must not already exist. Input must contain validated boot-status frames
and data/type-03 timing pairs to produce raw sensor events. Sync frames alone do not
establish a boot identity. Replay keeps MCU source/report time and sets host receipt
time to null: raw binary files have no original arrival timestamps.

The printed summary includes record/event counts, CRC/header errors, incomplete tail,
rejected pairs and missing boot/pair state. Nonzero exit flags a damaged/incomplete or
unusable handover capture; output may still contain recovered events for inspection.
A clean replay proves decoding only, not sensor health or scheduling performance.

Tests include standstill wheel continuity during one second of invalid IMU data,
fragmentation invariance, corruption recovery, truncated tails and MCU restarts.
These use synthetic packets; they are not vehicle measurements.

## Team exchange contents

1. Repository revision plus local-change status and validation evidence.
2. Packet vectors in tests/packet_vectors.json and protocol definitions.
3. Synthetic fixture tests and, later, labelled real captures with original timing logs.
4. PRE_HARDWARE_CHECKLIST.md decisions P1-P4 and C1-C4.
5. Decoded comparison results from the actual Perception bridge / Control client.

Keep raw MCU validity separate from Perception state flags. A returning IMU event is
a candidate measurement, not trusted gyro; a zero wheel estimate is not proof of rest.
