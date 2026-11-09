# Embedded software verification - 9 November 2026

Current local source verified after the interrupted sessions. All checks below PASS.
35 Python test methods; 316 C++/Python packet comparisons; three C++ suites;
six ESP32-S3 N16R8 profiles. No compile errors in the final run.
The initial clean SDK compilation emitted an upstream touch-driver deprecation
warning. Own-source volatile counter warnings were corrected and all checks rerun.

| Check | Result |
|---|---|
| Python unit tests | PASS |
| C++/Python packet compatibility | PASS |
| mpu6050 compile | PASS |
| mpu6050 execution | PASS |
| vehicle compile | PASS |
| vehicle execution | PASS |
| endpoint compile | PASS |
| endpoint execution | PASS |
| Firmware simulation | PASS |
| Firmware --imu | PASS |
| Firmware --bench | PASS |
| Firmware --vehicle | PASS |
| Firmware --vehicle --telemetry-hz 200 | PASS |
| Firmware --vehicle --wheel-diagnostics-v2 | PASS |

Additional checks: 17 Python files compiled for syntax, JSON configuration parsed,
Linux helper line endings checked, git diff whitespace check passed. Simulated
capture replay: 100 pairs, 80 wheel events, 100 IMU events, no CRC/header errors.

No hardware tested. No measured timing, stopping distance, sensor-health guarantee,
axes, bias/noise or physical calibration is claimed. Outputs remain disabled and
calibration unconfirmed. V2 remains opt-in pending Perception acceptance.

## Remaining gates

- [ ] Perception accepts timing v2 version and diagnostics bit assignments.
- [ ] Control accepts forward-only/rollback rules, relay contract and fault recovery.
- [ ] Measure shaft ratio/geometry and select exact target pattern; >=400 counts per
      wheel revolution is a proposed target, not a completed hardware selection.
- [ ] Confirm IMU axes, shared geometry, wheel scale and actuator calibration.
- [ ] Tune/validate drive-no-edge monitor; obtain real captures and packet timing.
- [ ] Run sensor dropout/restart, power, actuator and integrated vehicle tests.

Repeat software verification in Ubuntu: bash tools/validate_wsl.sh.
Machine-readable full logs: build/software-validation.json (local, not in source control).
