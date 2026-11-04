# Detailed pin wiring proposal W1

4 November 2026. Companion to POWER_DESIGN.md. Hardware qualification remains
pending. Firmware pins were checked against imu_config.h and vehicle_config.h.

| ESP label | Destination |
|---|---|
| GPIO8 | MPU SDA, pull-up to 3.3 V only |
| GPIO9 | MPU SCL, pull-up to 3.3 V only |
| GPIO4 | MPU INT |
| 3V3 | MPU VCC only after breakout regulator verification |
| GND | MPU GND and AD0 |
| GPIO5 | Conditioned speed channel A |
| GPIO6 | Reserved optional quadrature B; unused today |
| GPIO15 | Steering PWM buffer input |
| GPIO16 | ESC PWM buffer input |
| COM USB-C | Pi USB-A via data cable; ESP power and serial |

MPU XDA/XCL are unused. ESP header numbers above are GPIO silkscreen labels,
not physical positions. Native USB is unused; do not simultaneously supply ESP
5V externally. The Pi uses the protected GPIO power carrier in POWER_DESIGN.md:
physical pins 2/4 for 5 V, 6/9 for return; Pi USB-C power remains disconnected.

## New proposed PWM buffer (not purchased or tested)

Use TI SN74AHCT125N, 14-pin N package, powered from protected 5V_ELEC, not the
6 V BEC. This avoids relying on unverified 3.3 V actuator input thresholds;
5 V input compatibility and reset behavior must still be checked on the devices.

| Pin / part | Connection |
|---|---|
| 14 VCC, 7 GND | 5V_ELEC and common GND |
| C1 100 nF | Between 14 and 7, close to IC |
| 2 (1A), 3 (1Y) | GPIO16 input; output through R3 220 ohm to ESC signal |
| 5 (2A), 6 (2Y) | GPIO15 input; output through R4 220 ohm to servo signal |
| 1 (1OE), 4 (2OE) | GND; used channels enabled |
| R1, R2 10 kohm | Pins 2 and 5 respectively to GND |
| 10 (3OE), 13 (4OE) | 5V_ELEC; unused channels disabled |
| 9 (3A), 12 (4A) | GND |
| 8 (3Y), 11 (4Y) | No connection |

This is not an independent safety interlock. No-pulse and supply-transition
behavior must be verified. Do not substitute an HC125 or a different package
without checking thresholds and pinout. Buffer outputs never connect to ESP inputs.

ESC BEC +6 V feeds only servo positive; servo return goes directly to ESC BEC
return. Signal grounds join the common ground; servo current must not flow
through ESP header pins. ESC plug cavity orientation is unverified: identify
signal/+6V/GND on the actual device rather than trusting wire colour alone.

## Speed alternatives

For a verified bare Allegro A3144 EU part only: pin 1 to 5V_ELEC, pin 2 GND,
pin 3 to GPIO5; 4.7 kohm from pin 3 to ESP 3V3 and 100 nF between pins 1/2.
Use the package drawing to establish orientation. This is not a generic breakout
module pinout. A single channel does not establish signed speed.

For a TCRT5000 module: VCC must match its actual supply specification, GND to
sensor ground, DO to GPIO5 only after output level is verified at 0-3.3 V; AO
unused. A 5 V module may require output conditioning. Bare TCRT5000 parts need
a separate LED/detector circuit and cannot use module pin labels.

## Pi peripherals

Use separate factory USB cables for ESP and A1M8 original adapter. Retain the
LiDAR adapter-to-sensor/motor harness; its revision/pin order is unverified.
Camera V2 uses a Pi 5-compatible 22-to-15-pin camera adapter cable from CAM/DISP0.
This is not a straight pin-number mapping. Install with power off.

Official Pi 5 FAN connector: 1 +5V, 2 PWM, 3 GND, 4 tach. Use the keyed matching
cooler plug; do not apply a generic PC fan pinout. Factory cable bundles are not
individual electrical nets and must not be spliced together.

## Sources

- https://www.raspberrypi.com/documentation/computers/raspberry-pi.html
- https://www.pololu.com/product/5571
- https://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/151/sn74ahct125.pdf
- https://www.allegromicro.com/~/media/Files/Datasheets/A3141-2-3-4-Datasheet.ashx

No fuse ratings, battery monitor/cutoff, unknown connector orientations or sensor
module pinouts have been invented. These items remain assembly release holds.
