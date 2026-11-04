# Embedded pre-hardware checklist

Updated 4 November 2026. Work order: software and design first, physical tests later.
Checked means the stated deliverable exists, not vehicle acceptance. Joint decisions
remain proposals until the named teams record acceptance. No hardware tests have run.

## 1. Software baseline and repeatable verification

- [x] IMU initialization/readback, SI conversion, clipping, freshness, retries and recovery.
- [x] Shaft capture, count-window speed estimator and invalid-transition handling.
- [x] Continuing periodic telemetry at zero speed and independently of IMU validity.
- [x] Independent source timestamps, boot identity and deduplicated raw-event adapter.
- [x] Control command lifecycle, expiry/replay checks, cancellation and status reporting.
- [x] Embedded acceleration-to-effort conversion and steering/PWM adapters; defaults disabled.
- [x] Host tests and five firmware build profiles maintained in CI.
- [x] Add offline binary-to-JSONL raw-event replay for team handover.
- [x] Add one validation entry point with machine-readable PASS/FAIL/BLOCKED evidence.
- [x] Run current Python suite: 26 test methods passed locally on 4 November.
- [ ] Rerun host C++ tests and all five target builds for the current working tree.
      Local compiler/Arduino CLI are unavailable. Previous CI is historical evidence,
      not evidence for uncommitted changes. CI will run after publication.

Implementation references: IMU.md, CONTROL_PROTOCOL.md, STATE_V04_REVIEW.md.
Verification command: `python tools/validate_software.py --firmware`.
Default evidence: `build/software-validation.json`; nonzero exit means incomplete/failure.
No option uploads firmware or opens a hardware port.

## 2. Perception handover and agreement

- [x] Provide packet definitions, independent sample times and standstill/dropout tests.
- [x] Provide offline replay and explicit missing-receipt-time handling.
- [ ] P1: Perception accepts type-03 pairing, sync CRC coverage and boot reset behavior.
- [ ] P2: Jointly select 100 or 200 Hz telemetry; wheel acquisition remains 100 Hz.
- [ ] P3: Resolve direction sensing. Current single sensor cannot validate signed speed.
- [ ] P4: Perception names ROS topics/frame IDs and owns clock mapping, calibration,
      EKF/trust, dropout/recovery and AMCL gating. Embedded supplies raw measurements.
- [ ] Run the same binary fixtures through the team's actual Pi bridge offline.

Completion evidence: named reviewer, dated decision, agreed protocol/version and
matching decoded fixture outputs. Do not implement Pi state-estimator policy on MCU.

## 3. Control handover and agreement

- [x] Acceleration conversion belongs to Embedded; feedforward mechanism implemented.
- [x] Separate APPLIED (driver API write) from achieved physical motion and SIMULATED.
- [x] Define boot/session/epoch, cancellation, deadlines and explicit rearming behavior.
- [ ] C1: Control accepts longitudinal command meaning, units, sign and envelope.
- [ ] C2: Control/Safety accepts command cadence, <=100 ms expiry proposal and authority lifecycle.
- [ ] C3: Agree neutral/steering behavior on each fault; physical stopping remains unproven.
- [ ] C4: Jointly review simulated commands/results against the actual Control client.
- [ ] Populate measured actuator coefficients later; no invented calibration constants.

Completion evidence: written acceptance plus shared command/result fixtures and logs.
Existing tests exercise the real C++ endpoint; a Python codec alone is not endpoint evidence.

## 4. Electrical and mechanical design release

- [x] Draft 2S whole-vehicle energy budget and 9000 mAh design target.
- [x] Draft deck zones, pin allocations, harness connector map and logic buffer circuit.
- [x] Record Egypt/local sourcing constraint and distinguish candidates from available parts.
- [x] Screen preferred supplier catalogs; see LOCAL_SOURCING.md for rejected/held candidates and enquiry specification.
- [x] Define remaining protection/monitoring architecture and release gates in DESIGN_RELEASE.md.
- [ ] Select an obtainable converter and protected Pi carrier with manufacturer documentation.
- [ ] Select per-cell monitor, load disconnect and Pi shutdown interface; allocate pins after selection.
- [ ] Select exact U3 overvoltage protection, main disconnect and compatible connectors.
- [ ] Resolve 8.4 V full-pack compatibility of the motor and exact servo voltage rating.
- [ ] Select direction-capable wheel sensor arrangement or secure a revised operating contract.
- [ ] Obtain chassis/board/battery dimensions before final deck CAD and print files.
- [ ] Freeze schematic/BOM only after selected parts have reviewed pinouts and ratings.

A diagram with unselected protection blocks is a design proposal, not assembly release.
Rated part selection can precede tests; fitted fuse values, thermal margins and battery
runtime still require measurement. Never equate a successful software build with these.

## 5. Prepared now, executed only in the later hardware phase

- [x] Standalone IMU procedure: IMU_BENCH.md.
- [x] Integrated sensor/actuator/power sequence: INTEGRATED_TEST_PLAN.md.
- [ ] First ESP/IMU wiring inspection and standalone IMU acquisition.
- [ ] Wheel scale/direction, sample timing, IMU axes/noise and reconnect tests.
- [ ] Logic-analyser PWM checks with actuators disconnected.
- [ ] Servo/ESC calibration, motor conversion fit and controlled stopping tests.
- [ ] Regulator, cutoff, brownout, interference, thermal and energy measurements.
- [ ] Physical integration and representative 30-minute vehicle run.

## Execution priority

1. Complete software verification in a toolchain-equipped environment.
2. Exchange the offline handover package and record P1-P4 / C1-C4 decisions.
3. Resolve component selection and finish the electrical drawing/BOM revision.
4. Obtain dimensions and finish mechanical CAD.
5. Release a tagged software/configuration/design baseline, then begin standalone IMU tests.

Items requiring other teams, component identification or measurements stay visibly open.
No team response or physical value can be inferred from elapsed time.
