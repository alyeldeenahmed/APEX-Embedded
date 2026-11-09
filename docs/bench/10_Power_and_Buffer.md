# Power protection and logic buffer checks

9 November 2026. Procedure only; no hardware pass claimed.

Final battery, converter, protection module and fuse ratings remain unselected. Start component tests with known-good bench supplies and Pi USB-C power. This procedure qualifies selected parts later; it is not a released vehicle-power schematic.

| Rail / path | Connection and separation |
| --- | --- |
| Initial ESP supply | Laptop USB to COM; later Pi USB to COM. No simultaneous external5V/3V3 power. |
| Initial Pi supply | Suitable USB-C supply. Leave GPIO5V injection disconnected. |
| ESC BEC6V | Compatible servo/tester only; never Pi, ESP or AHCT buffer VCC. |
| Vehicle battery later | 2S battery positive -> source fuse -> DC disconnect -> separately fused ESC/electronics branches. Negative returns separated for high current, joined at distribution. |
| Electronics converter later | Protected regulated5V; qualify loaded voltage/ripple and inrush before connection. GPIO carrier remains unreleased. |
| Ground routing | Servo/motor current through rated power returns, not ESP headers, USB shield or solderless breadboard. |

## AHCT125N optional signal buffer pin map

| Pin | Connection |
| --- | --- |
| 14 / 7 | Regulated5V / common GND;100nF directly between them |
| 2 / 3 | GPIO16 input /220ohm series to ESC signal;10kohm pin2 to GND |
| 5 / 6 | GPIO15 input /220ohm series to servo signal;10kohm pin5 to GND |
| 1 / 4 | GND enables the two used channels |
| 10 / 13 | 5V disables unused channels |
| 9 / 12 | GND holds unused inputs low |
| 8 / 11 | Unconnected unused outputs |

This map is for TI SN74AHCT125N14-pin DIP only. Count pins using notch/dot and the datasheet. Do not substitute HC125 or another package. Output5V compatibility of the actual actuator inputs still needs verification. No buffer is needed if3.3V compatibility is established over the intended conditions.

## Qualify before combining the system

1. With power off, verify polarity, connector retention and absence of a persistent short on each rail. Capacitor charging can briefly look low-resistance. Disconnect loads and set the converter before connecting electronics.

2. Use a current-limited supply and dummy load first. Record selected part numbers, fuse type/time-current curve, wire gauge, connector rating, input/output voltages and load current. Choose fuses to protect the weakest downstream wire/connector while tolerating measured normal startup; no universal fuse value is specified.

3. For the proposed2S system, qualify regulator operation across the allowed pack discharge range and8.4V full charge, including transients. Establish cell protection/cutoff from battery specifications; total pack voltage alone can hide an imbalanced low cell. Never deliberately over-discharge a LiPo to test protection. Use a programmable supply/fixture.

4. Load electronics incrementally: Pi+fan, ESP+IMU, camera, LiDAR. Record current, minimum5V at the Pi and temperature. Use a scope for short dips/ripple; a multimeter alone can miss reset-causing events.

5. Finally apply servo/drive transients in a controlled fixture and observe all rails. Confirm no unwanted PWM/motion on reset, loss of signal or power sequence. Validate disconnect behavior before driving. Do not short the supply to test a fuse.

## Runtime evidence

The9000mAh2S pack is a planning target, not a proven30-minute result. Log pack voltage/current under the intended moderate-driving workload and integrate watt-hours. Report usable energy, average/peak current, elapsed runtime, cell balance and endpoint condition. Do not infer capacity from the label alone. Use a compatible2S balance charger under its instructions.

## Buffer bench test

Actuators disconnected: verify5V VCC,3.3V GPIO input and output high/low levels with a scope. Observe pulse width, boot/reset behavior and power sequencing. The AHCT device is not an independent output interlock. Do not probe with an analyser whose input rating is below5V.

## Record and send

Keep the selected BOM, measured voltage/current/temperature tables, rail traces, fuse/wire rationale and power-sequence observations. Power-related motor delay trials are invalid if the supply is current limiting or browning out; retain and label them instead of fitting them into the normal actuator model.

## References and configuration

[TI SN74AHCT125 pin and voltage limits](https://www.ti.com/lit/ds/symlink/sn74ahct125.pdf)

[Pi supply requirements](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html)
