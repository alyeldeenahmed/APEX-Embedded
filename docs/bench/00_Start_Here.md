# Fast bench test sequence

9 November 2026. Procedure only; no hardware pass claimed.

APEX Embedded | Aly and the component test operator | 9 November 2026Goal: obtain useful measurements quickly without bypassing calibration gates. These are procedures, not completed tests.

| Order | Do this | Evidence / dependency |
| --- | --- | --- |
| 1 | ESP32 then MPU-6050 | Start with laptop USB only. Send raw IMU CSV, summaries and axis photos to Perception. |
| 2 | Encoder by hand | Requires selected two-channel sensor and target. Send signed counts, resolution and zero-speed capture. |
| 3 | Servo alone | Use a manual pulse tester; motor disconnected. Send wheel-angle/PWM map and measured response. |
| 4 | ESC and motor | Identify actual ESC mode and neutral first. Unloaded tests precede controlled loaded driving. |
| 5 | Pi, fan, camera and LiDAR | Use a known-good Pi supply; test one peripheral at a time. Run compute benchmark before combined load. |
| 6 | Power and integrated faults | Final battery/regulator/fuses still unselected. Do not use the provisional carrier as a released design. |

## Have these ready

Soldered headers and short leads; USB data cable; multimeter; correct power supplies; a manual RC pulse tester with readable pulse width or an oscilloscope; logic analyser rated for the measured signal voltage; wheel-angle scale; high-frame-rate camera; secure chassis stand; accessible power disconnect. No automatic sweeping tester mode on first use.

## Use the existing software

All repository commands in the guides run from the embedded folder. Replace COM_NUMBER with the actual Windows port. Python commands assume Python 3 and the bench requirements are installed. WSL builds and Windows USB access are separate: do not guess a Linux serial device from a Windows COM number.

```text
python -m pip install -r requirements-bench.txt
python imu_monitor.py --list
```

## One record for every run

Create a new run ID. Save date, operator, firmware/branch commit, component label, wiring photo, supply voltage, setup/load, instrument sample rate and raw logs. Record failures too. Blank measurement cells mean unknown; never enter a nominal setting as a measured result.

Use the supplied CSV templates. Send first useful results immediately; do not wait for all tests. Bench steering/motor tests are preliminary. Loaded limits and driving-delay measurements follow after the vehicle and power system are ready.
