# SIMULATED wheel-v2 decoder handover

Not a real ESP32 capture. No physical timing, bias, noise, sensor health or axes
are demonstrated. Peer approval of timing version 2 is still required.

100 data/timing pairs at synthetic 100 Hz: 40 valid unsigned zeros, 40 valid
unsigned moving values, 20 suspect/invalid wheels with continuing valid IMU.
Sequence wraps 65535->0; 64-bit time exceeds 2^32. Status boot=42 every 100 ms.
Expected 80 wheel and 100 IMU events, no CRC/header errors. Gyro/accel values are
synthetic. Host receipt time is null; do not use this file to measure serial jitter.
Run tools/generate_wheel_handover.py to reproduce. Exact byte/bit proposal:
docs/PERCEPTION_DECISIONS_2026_11_09.md.
