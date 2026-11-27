# APEX ESP32–Pi integration · 27 November 2026

The ESP32-S3 sends motor-encoder and IMU telemetry. The Pi confirms receipt of each complete report over the same USB cable. The included Windows client can stand in for the Pi during bench testing.

## 1. Upload to the ESP32

1. Extract the ZIP. Open `firmware/telemetry_demo/telemetry_demo.ino` in Arduino IDE. Keep all the tabs in that sketch folder together.
2. Use Espressif ESP32 core **3.3.0**, **ESP32S3 Dev Module**, **COM3**, USB CDC On Boot **Disabled**, **16 MB** flash, **QIO**, **OPI PSRAM**, and the **3 MB APP / FATFS** 16 MB partition. Use the USB-to-UART connector marked **COM** on your board.
3. Disconnect motor power and click **Upload**. Close Arduino Serial Monitor afterwards. The default vehicle firmware streams sensor data with physical PWM disabled.

| Module | Connections to the ESP32 |
|---|---|
| MPU-6050 | VCC → 3V3, GND and AD0 → GND, SDA → GPIO8, SCL → GPIO9, INT → GPIO7 |
| MT6701 I²C, motor-mounted | VDD → 3V3, GND → GND, SDA → GPIO10, SCL → GPIO11. Leave ANALOG/PWM open. Address 0x06. |
| ESC and servo | ESC signal → GPIO4, servo signal → GPIO5, common signal ground. Keep ESC BEC positive separate from ESP USB power. |
| Host | USB data cable to ESP COM connector. Binary serial, 921600 baud, 8N1. |

## 2. Test both directions now on Windows

1. With COM3 free, double-click **Check_Receipts_COM3.cmd**. The test acknowledges normal traffic, deliberately withholds replies and resumes them on the same open port. Its PASS line means the ESP reported four outstanding pairs, reported recovery on the same boot and kept outputs disarmed. The test refuses an active or PWM-enabled setup.
2. Close that window. Double-click **Start_Slow_Monitor_COM3.cmd**. Look for the receipt count to increase, the ESP receipt fault to stay **clear**, and the arm state to stay **NO**. The display refreshes once per second. All decoded events are recorded in a new JSONL file next to the launcher. Press Ctrl+C to finish.
3. Turn the motor shaft **by hand**. Continuous readings should show positive nominal speed for forward rotation and negative speed in reverse. An encoder gap shows **UNAVAILABLE**. The IMU reports acceleration including gravity in m/s² and angular velocity including stationary bias in rad/s. The estimator validity flags remain gated by installation checks.

The ZIP includes pySerial. If using a source checkout, install `requirements-bench.txt` in the Python environment running the scripts.

## 3. Run on a working Pi

Copy the extracted **APEX-Embedded** folder to the Pi and open a terminal inside it. Connect the USB cable to the ESP COM port. Use one serial owner at a time. The standalone client and Perception’s ROS bridge run separately.

```sh
ls -l /dev/serial/by-id/
```

Use the ESP USB-to-UART device listed above. If the team has installed its udev alias, these commands work directly. Otherwise replace `/dev/apex_esp` with the actual by-id path. Run them one at a time.

```sh
python3 tools/check_live_link.py --port /dev/apex_esp
python3 slow_monitor.py --port /dev/apex_esp --display-hz 1
```

Press Ctrl+C to close the viewer. For a quiet 60-second full-rate capture, choose a new filename each run.

```sh
python3 car_link.py --port /dev/apex_esp monitor --quiet --seconds 60 --output integration_01.jsonl
```

**For Perception:** use `CarLink.poll()` continuously in the sole serial owner, `apex_esp_bridge`, or feed every decoded record to `protocol.TelemetryReceipts.feed()` and write each nonempty return value back to the ESP. Send the receipt even when sensor validity is clear. Keep ROS callbacks and disk work outside that time-critical path. ROS topic publication, source-time mapping and estimator validity remain with the bridge.

## 4. Understand the stop rule

- One complete message is the 54-byte data frame (0x01) plus its matching 42-byte timing frame (0x03). The Pi acknowledges each CRC-checked pair after learning the ESP boot identity. Sync and status packets do not count.
- The ESP counts fully queued pairs. A current cumulative receipt retires the matching pair and any older outstanding pairs. On reaching **four outstanding pairs**, the ESP revokes the session, requests calibrated ESC neutral, suppresses servo pulses and sends a link-loss status if the port can carry it. Neutral can allow coasting. Suppressing pulses leaves steering position and holding torque dependent on the servo.
- At 100 Hz four reports span about four 10 ms cycles from the previous acknowledged report. This is a scheduling estimate. A separate 40 ms guard detects a stalled telemetry transmitter. Sensor faults and the command deadline can disarm sooner.
- Restored replies allow communication again. Motion requires a fresh INSTALL and explicit ARM with valid feedback and completed actuator calibration. Sending receipts alone never commands motion.

## 5. Sensor settings and remaining bench evidence

The MPU-6050 uses ±4 g, ±1000 °/s and hardware DLPF setting 3. This selects **42 Hz gyro** and **44 Hz accelerometer** bandwidth. The ESP checks the registers at startup and during operation. Internal sample rate is nominally 1 kHz and paired telemetry is nominally 100 Hz. See the [manufacturer register map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf).

The MT6701 uses a dedicated I²C controller and an acquisition task. The installed forward sign is −1, stock gearing is 8.27:1 and nominal tyre diameter is 65 mm. A failed read breaks continuity. Recovery starts from a new angle baseline, so movement through the gap stays unknown. One isolated error no longer imposes the former double 100 ms wait. The earlier electrical NACKs still require a capture from this build.

Before powered driving, verify encoder continuity and loaded rollout, installed IMU axes, ESC neutral/drive/brake calibration, steering rate, and physical motor/servo response to the stop rule. The firmware retains the existing calibration and feedback gates. The commissioning results on Windows and a working Pi should be saved separately.

See [wire format and status bits](docs/CONTROL_PROTOCOL.md) and [vehicle wiring and calibration](CAR_INTEGRATION.md).
