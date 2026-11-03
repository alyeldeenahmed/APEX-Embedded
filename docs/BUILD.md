# Reproducible build and packet interoperability

Milestone 2 checks the firmware build and packet representation without requiring
a board. It does not validate electrical signals, sensor acquisition or real timing.

## Build target

The authoritative settings are in `firmware/build-config.json`:

- Arduino CLI 1.3.1 and Espressif Arduino core 3.3.0.
- Generic ESP32-S3, 8 MB flash, QSPI PSRAM enabled (planned N8R2).
- 8 MB partition layout, QIO flash.
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
This still leaves physical pins unassigned and orientation unconfirmed. See IMU.md
before preparing a hardware build. Simulation remains the default.

Outputs go into ignored `build/esp32s3-n8r2/`. Firmware compilation is also run in
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

The sync CRC coverage remains provisional. Host execution validates serialization
on the host; the ESP32 build checks target compilation. Neither executes the binary
on a real ESP32 or validates scheduling, resets, fresh sensors or motor behavior.

References:
- https://github.com/espressif/arduino-esp32/blob/3.3.0/boards.txt
- https://github.com/arduino/setup-arduino-cli
