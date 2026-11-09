> Update 9 November 2026: see [accepted decisions and proposed wheel v2](PERCEPTION_DECISIONS_2026_11_09.md). Sync CRC and timing v1 are accepted; 100 Hz is selected.

# Reproducible build and packet interoperability

Milestone 2 checks the firmware build and packet representation without requiring
a board. It does not validate electrical signals, sensor acquisition or real timing.

## Build target

The authoritative settings are in `firmware/build-config.json`:

- Arduino CLI 1.3.1 and Espressif Arduino core 3.3.0.
- ESP32-S3 DevKitC-1 N16R8 (purchased): 16 MB flash, OPI PSRAM enabled.
- app3M_fat9M_16MB partition layout, QIO flash.
- USB CDC on boot disabled: `Serial` uses UART0 at 921600 baud.

This is the initial bench build profile, not a claim that the final USB transport
has been agreed with Perception. Use the USB-to-UART bridge port for this profile.
The board/vendor pinout, actual memory and connector routing must be checked before
upload. No upload step is automated. The sketch does not need PSRAM for its packets.

Install Arduino CLI, then run from the repository root:

```
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.0 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
python tools/build_firmware.py
```

To compile the optional sensor mode, use `python tools/build_firmware.py --imu`.
Pins are GPIO8/9/4; orientation is unconfirmed. Use `--bench` for the standalone
CSV test. See IMU_BENCH.md before wiring/upload. Simulation remains the default.

Outputs go into ignored `build/simulation/`, `build/imu/` or `build/imu-bench/`. Compilation runs in
GitHub Actions on pushes and pull requests. See the run for the relevant commit;
a configuration file alone is not build evidence.

## Packet compatibility

```
python -m unittest -v
python tools/check_cpp_packets.py
```

The second command requires a host `g++` compiler (or executable path in `CXX`).
It compiles a small test adapter that includes the actual firmware `packet.h`.
It compares emitted bytes with Python and the fixed examples, then feeds each
firmware packet into the Python decoder in seven-byte chunks.

`tests/packet_vectors.json` contains reusable input values and expected complete hex
frames. Reference bytes were constructed separately using explicit field offsets
and Python's standard-library CRC implementation, not the project encoder. They
are implementation test fixtures, not Perception-approved normative vectors.

Cases cover stationary invalid simulation, forward and reverse values, count/age
boundaries, sequence/time wrap, and 64-bit sync boundaries. A further 200 seeded
data cases exercise the C++/Python comparison reproducibly. Valid flags in fixtures
are test inputs only; the on-board simulation continues to mark all readings invalid.

The sync CRC coverage is confirmed by Perception. Host execution validates serialization
on the host; the ESP32 build checks target compilation. Neither executes the binary
on a real ESP32 or validates scheduling, resets, fresh sensors or motor behavior.

References:
- https://github.com/espressif/arduino-esp32/blob/3.3.0/boards.txt
- https://github.com/arduino/setup-arduino-cli

## Local WSL verification (9 November setup)

A dedicated Ubuntu cache at ~/.cache/apex-embedded-toolchain holds the pinned
ESP32 core; Arduino CLI 1.3.1 and downloaded archives are under ignored
build/toolchain. Windows/OneDrive extraction was slow, so installed compiler
files live in Linux storage. From this repository in Ubuntu, run:

`bash tools/validate_wsl.sh`

This runs host checks and all six profiles (including proposed wheel v2), never
uploads, and writes build/software-validation.json. The helper assumes this local
cache is installed; on another machine follow the pinned installation steps above.
