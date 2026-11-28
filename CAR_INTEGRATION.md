# TT-02 integration | ESP32-S3 and Raspberry Pi 5

**Integration update · 27 November 2026.** Start with the [ESP and Pi receipt check](INTEGRATION_START_HERE.md). This supersedes older wiring PDFs for signal assignments: ESC GPIO4, servo GPIO5, IMU INT GPIO7, MT6701 SDA/SCL GPIO10/11 (GPIO12 unused). Never combine the old bench pin map with this firmware.

## 1. Wire the vehicle

This vehicle profile uses the November engineering test schematic for PWM and IMU signals. Replace the former speed-sensor wiring with one direct digital harness to the motor-mounted MT6701. Keep the encoder PCB stationary and secure the diametrically magnetized magnet concentrically to the motor rotor. Verify there is an accessible motor-end mounting point and clearance on your RS-540 before spinning. The propeller shaft is unused.

| MT6701 I2C module marking | ESP32-S3 destination |
|---|---|
| VDD | 3V3 |
| GND | GND |
| SDA | GPIO10, I2C data |
| SCL | GPIO11, I2C clock |
| ANALOG/PWM | Leave open |

Use printed pin labels, not connector position. The encoder uses Wire1 at 400 kHz on GPIO10/11; the IMU stays on Wire at GPIO8/9. Disconnect the former GPIO12 chip-select wire. The module must already be configured for I2C; do not guess exposed mode-strap connections. Use 3.3 V power and SDA/SCL pull-ups to 3.3 V only. If pull-ups are absent, fit one 4.7 kOhm resistor from each line to 3.3 V. Default 7-bit address is 0x06; use 0x46 only if confirmed programmed. Firmware does not program EEPROM.

| Other signal | Vehicle assignment |
|---|---|
| ESC command | GPIO4 |
| Steering servo command | GPIO5 |
| Optional buffer enable transistor input | GPIO6, high enables the November buffer circuit |
| MPU6050 SDA / SCL / INT | GPIO8 / GPIO9 / GPIO7, AD0 low for 0x68 |
| Pi to ESP32 | Pi USB-A to ESP USB-to-UART/COM port with a data cable, 921600 baud |
| Servo power | ESC BEC and its return. Common signal ground with ESP. Keep BEC positive separate from ESP 5V/USB. |

Bench with drivetrain power disconnected for initial USB and encoder tests. For direct PWM wiring, first verify the ESC and servo accept 3.3 V signal levels. For the buffer circuit, check its ground, output enable polarity and neutral on a scope.

The Tamiya 54358 motor is specified for 6.6-7.2 V; do not feed this motor branch directly from the 12 V bench supply. A step-down must be checked for motor startup current and braking-energy behavior before testing. The ESC 2-3S label does not establish the motor voltage rating.

## 2. Upload the integration firmware

Open firmware/telemetry_demo/telemetry_demo.ino in Arduino IDE. Install Espressif ESP32 boards **3.3.0**. Choose **ESP32S3 Dev Module**, the board's COM/USB-to-UART port and **USB CDC On Boot disabled**. Match flash size, partition and PSRAM to the module marking. The project's saved CLI profile assumes N16R8, 16 MB QIO flash, OPI PSRAM. Verify the actual module before selecting it. Click Verify, then Upload. The sketch opens in the real vehicle sensor profile with physical PWM disabled. Close Serial Monitor before the Pi takes over.

Alternatively, with that same board and Arduino CLI installed, run from this extracted folder:

    python3 tools/build_firmware.py --vehicle

The build helper writes build/vehicle/. The Arduino IDE upload above compiles from source. Older build directories and older binary images predate the encoder update.

## 3. Bring up USB and check the motor encoder

The November 28 build provides a signed speed and age on each matched Pi report.
See [speed states and setup](INTEGRATION_START_HERE.md#reading-speed-on-the-pi)
for full-window measurements, short contiguous measurements and the 50 ms hold limit.

On the Pi, from this extracted folder:

    sudo apt install python3-serial
    ls -l /dev/serial/by-id/
    export CAR_PORT=/dev/ttyUSB0
    python3 car_link.py --port "$CAR_PORT" monitor --seconds 60 --output integration_capture.jsonl

Replace /dev/ttyUSB0 with the ESP's by-id path or its actual device. The LiDAR may also appear as ttyUSB. If access is denied, add your login to dialout and log in again. Run only one serial reader. The monitor emits decoded newline-delimited JSON and records it to a new file. It never arms. Use a different output filename for each run. This is a decoded capture, not raw serial bytes. Ctrl+C closes it.

1. USB alone should produce status and data. The physical PWM bit (flags & 4 on **status**) is clear in the commissioning build.
2. Turn the motor slowly by hand. On **data** records enc_ticks must change by about 16384 per complete motor turn and count back when reversed. One complete forward turn of the car's wheels should increase the count. The confirmed forward sign in this build is −1.
3. Paired **timing** records include motor_rpm when consecutive samples have usable timestamps. A clean quarter motor turn in 10 ms corresponds to 1500 RPM. I2C NACK/short reads and long acquisition gaps invalidate the affected interval. Angle-register reads do not include SSI CRC or magnetic-field status: a responsive sensor does not prove a healthy magnet. Counts omit unknown movement during those gaps. If turning produces no change, inspect magnet position, ground, I2C mode, address, SCL and SDA before enabling drive. A frozen or incorrect angle can still look like rest; verify actual rotation and the no-motion diagnostic.
4. The scale is now configured from Aly's confirmed standard TT-02 original gears and 65 mm tyre diameter: published total ratio 8.27:1 (22T pinion / 70T spur), nominal circumference 0.20420352 m. With 16384 motor-angle counts/revolution this gives 135495.68 counts/wheel revolution and approximately 0.000001507085 m/count. Firmware reports nominal wheel_speed in m/s on continuous valid read intervals even before the commissioning confirmation. The forward sign is configured. Set wheel_confirmed=true only after the fitted sensor tracks continuously and its scale has been checked. Until then **data.flags & 2 remains clear** and Perception must not publish the speed as valid estimator feedback. Loaded rollout, slip and differential action can change accuracy. Do not infer whole-car travel from spinning only one raised wheel.
5. The firmware requests an angle every 400 microseconds and rejects intervals of 1000 microseconds or longer, assuming the motor stays below 30000 RPM. Verify that upper bound at the actual pack voltage and check RPM continuity with the IMU and USB link active. Shorten the allowable gap if the motor can run faster. Physical drive disarms on detected feedback loss. Check stationary noise as well, since the controller requires forward or zero speed.
6. Verify the MPU axes against the vehicle frame in mpu6050_esp32.h before setting APEX_IMU_AXES_CONFIRMED. **data.flags & 1** qualifies IMU samples. The LiDAR and camera remain Pi peripherals.

## 4. Enable physical commands after calibration

The complete vehicle firmware includes both outputs: steering PWM on GPIO5 and ESC PWM on GPIO4, at 50 Hz, alongside IMU and I2C encoder acquisition. Select the explicit actuator profile with:

    python tools/build_firmware.py --vehicle --enable-outputs

Its files go to build/vehicle-outputs/, separate from the sensor-only build/vehicle/. In Arduino IDE, change the default `#define APEX_ENABLE_OUTPUTS 0` to `1` in vehicle_config.h instead. A build request is not an ARM command. Current ESC pulse widths, acceleration mapping, steering rate and feedback calibration are incomplete, so even the actuator profile currently produces no PWM. Do not substitute guessed values merely to clear these checks.

With calibrated outputs enabled, startup and disarm request ESC neutral and hold the servo signal low. Explicit ARM at zero commands centre. Four outstanding telemetry pairs revoke the session. Communication recovery requires a fresh INSTALL and ARM to move again. Neutral can allow coasting. Status flag 4 confirms physical PWM initialization, flag 2 identifies the armed state. The GPIO6 buffer-enable signal is unused in Aly's direct GPIO wiring; common ground and verified 3.3 V signal acceptance are still required.


In firmware/telemetry_demo/vehicle_config.h retain the actual static servo values (1250 us left, 1500 us centre, 1750 us right), fill in measured ESC neutral/drive/brake pulse widths, and complete the remaining fields in calibration(). The equivalent steering limit is +/-0.33161256 rad (19 degrees, the mean of the two front-wheel angles). Intermediate angles use an approximation; steering rate and response delay remain unmeasured. Use radians and m/s². Set a measured motion_effort_threshold (0 to 1, exclusive) and motion_timeout_us that distinguish sustained drive from stationary noise. Verify forward/brake-only mode on the THW-1060-RTR, signal polarity and steering endpoints using your RC tester/receiver with wheels raised. The two actuator confirmation switches, calibration.confirmed and outputs_enabled are explicit. Rebuild and upload after changing them. An uncalibrated image refuses physical commands.

First test neutral with the wheels clear, an operator at the power disconnect, and the Pi monitor closed:

    python3 car_link.py --port "$CAR_PORT" bench --accel-mps2 0 --steering-rad 0 --seconds 1

For the first powered motion, use a small positive acceleration within your measured range, keep steering at zero and use a short duration. The Pi explicitly installs a session, arms at neutral, sends setpoints about every 20 ms and revokes at completion. A rejected command terminates the run. Stale commands expire at the ESP within their 80 ms validity window and return the outputs to neutral. Verify link loss, neutral, brake direction and steering on stands before a floor test. Neutral allows the vehicle to coast.

## 5. Connect the MPC on the Pi

Perception's apex_esp_bridge is the sole serial owner during team integration, using /dev/apex_esp after its udev rule is installed. Control must use the jointly agreed ROS relay, not open USB. CarLink is a standalone commissioning client or a transport helper inside that sole owner; the example below is not a second production process. on_event receives decoded IMU, motor counts, wheel speed, motor RPM, timestamps and status as dictionaries. Forward the events to the estimator without blocking. Positive steering means left; negative acceleration requests braking. Only fresh planner outputs should reach set(). If the planner stops, allow the MCU deadline to expire. A session that has expired needs a new explicit arm.

```python
import serial
from car_link import CarLink

# Application hooks below must return promptly. The USB owner polls continuously.
with serial.Serial(port_path, 921600, timeout=.003, write_timeout=.04,
                   exclusive=True) as port:
    car = CarLink(port, on_event=handle_feedback, speed_details=True)
    try:
        car.arm()  # call only on operator authorization
        while integration_running():
            car.poll()  # Validates telemetry and sends its receipt on this port.
            command = take_fresh_setpoint_without_waiting()
            if command is not None:
                car.set(*command)  # (road-wheel radians, acceleration in m/s²)
    finally:
        car.stop()
```

If the planner uses a separate process, relay its command/status frames through apex_esp_bridge. ROS command/status topic names, message types and reconnect behavior still require agreement with Omar; this package does not implement Perception's bridge. Opening USB a second time can reset the board. The included bench command exercises the link without requiring the planner.

## Engineering references

- Ekostra EK1895 product and board photo: https://ekostra.com/product/mt6701-14-bit-magnetic-angle-sensor-module-as5600-alternative/
- MagnTek MT6701 Rev. 1.8, I2C registers and timing: https://www.magntek.com.cn/upload/pdf/202407/MT6701_Rev.1.8.pdf

Wheel gearing source: Tamiya TT-02 manual, https://www.tamiya.com/cms/english/rc/rcmanual/tt02.pdf (standard gears confirmed by Aly).
