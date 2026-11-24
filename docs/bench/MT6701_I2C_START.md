# MT6701 I2C first test — motor power OFF

This replaces the old Hall/optical encoder test for the current vehicle. Do not use old SSI firmware or the earlier integration ZIP.

## Connect with USB unplugged

| Module pin | ESP32-S3 |
|---|---|
| VDD / VCC | 3V3 (module must support 3.3 V) |
| GND | GND |
| SDA | GPIO10 |
| SCL | GPIO11 |

GPIO12 is unused: remove the old CS wire. Leave the module's existing I2C mode strap intact. Pin positions are not known; use printed labels. SDA/SCL need pull-ups to 3.3 V; if absent use 4.7 kOhm on each. Do not connect these lines to 5 V pull-ups. The IMU stays on GPIO8/9/7 and its own I2C controller.

## Arduino IDE

1. Leave the ESC supply OFF. Connect ESP USB to the computer.
2. Open firmware/mt6701_i2c_bench/mt6701_i2c_bench.ino.
3. Select ESP32S3 Dev Module, ESP32 core 3.3.0, COM3, USB CDC On Boot disabled (USB-to-UART connector).
4. Upload. Open Serial Monitor at 115200 baud.
5. Expect read_ok=1 and angle_counts between 0 and 16383. Read failures print NA, never a valid zero.
6. Turn the motor magnet slowly by hand. Angle should change smoothly and wrap once per motor revolution. Reverse rotation should reverse the angle trend. A constant angle is not a pass.
7. Hold still for 30 seconds and save the displayed output. Send 10 stationary rows and some rows while rotating.

This 10 Hz display test confirms basic reads and angle changes only. It does not measure high-speed RPM, delay, gearing or motor acceleration. The integration firmware samples much faster and rejects tracking gaps; test that next using CAR_INTEGRATION.md.

If read_ok=0: check pins, common ground, 3.3 V supply, pull-ups and module I2C mode. Default address is 0x06; the programmable alternate is 0x46. Do not reprogram the sensor or substitute an AS5600 driver. I2C angle reads expose neither SSI CRC nor magnetic-status flags.

Source: MagnTek MT6701 Rev 1.8 section 7.7.2, https://www.magntek.com.cn/upload/pdf/202407/MT6701_Rev.1.8.pdf
