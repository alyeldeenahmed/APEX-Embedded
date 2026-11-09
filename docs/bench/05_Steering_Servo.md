# Steering servo connection and calibration

9 November 2026. Procedure only; no hardware pass claimed.

Carson CS-6 metal gear servo. Verify the exact article number and allowed supply on the unit/manual. Use a manual RC pulse tester for initial calibration; integrated firmware cannot arm before its calibration is known. Motor disconnected; start with the servo horn/linkage detached.

| Servo lead by function | Initial bench connection | Later ESP connection |
| --- | --- | --- |
| Positive supply | Regulated supply within verified servo rating | Verified ESC BEC +6 V only if servo rating permits |
| Ground | Supply negative and tester ground | BEC return; branch to ESP signal ground |
| Signal | Manual tester signal only | GPIO15 via verified interface/buffer |
| ESP power | Not needed for tester-only test | USB COM only; never BEC6V |

Identify connector cavities by documentation/continuity; colour alone is not proof. The tester and ESP must never drive the signal together. A tester must allow fixed manual pulse widths without an automatic full sweep. Measure its actual pulse with a scope if its display is unverified.

## Static mapping first

1. Set the tester to documented neutral before powering the servo. A typical RC centre is 1500 us, but verify it with the actual unit; do not assume pulse endpoints. Start at the lowest permitted supply. Stop on sustained buzzing, heating or supply collapse.

2. With power removed, fit the horn close to centre and connect steering. Secure chassis, front wheels free. Adjust mechanical alignment before electronic trim. Record supply voltage.

3. Move in small pulse increments from centre toward each side. Measure LEFT and RIGHT road-wheel angles from straight ahead. Stop short of binding or the mechanical stop. Record pulse, angles and approach direction at centre plus at least five points on each side.

4. Convert degrees to radians using degrees x pi/180. Steering left is positive. Servo shaft angle is not road-wheel angle; retain both wheel readings because steering geometry differs between inner and outer wheels.

## Measure response without confusing command and motion

1. Use high-frame-rate video with a fixed wheel-angle scale. Include a visible pulse-change marker synchronized to the actual output, or use a scope plus angle sensor on a common time base. A tester knob movement or PC send timestamp is not an exact pulse-change timestamp.

2. At safe small and medium steps, record at least 20 leftward and 20 rightward repetitions for preliminary bench response. Keep supply/load and step size in the log. Start with wheels lifted; repeat under representative ground load later.

3. Define delay as pulse change to first angle change above baseline noise. Record 10-90% rise time, settling within 5% of final step, overshoot and peak/sustained angular rate. Keep failed or censored trials. At 240 fps one frame is 4.17 ms; report this resolution rather than claiming sub-millisecond precision.

4. For the effective bicycle angle, preserve both wheel angles for Control. A geometry approximation is cot(delta_eff) = [cot(delta_left) + cot(delta_right)]/2 away from centre, with common signed convention. Do not silently use the inner wheel angle as the model angle.

## Optional 5 V signal buffer

Only after actuator input compatibility is established: SN74AHCT125N pin14 = regulated5V, pin7 = GND, 100 nF across14/7; GPIO15 to pin5, 10 kohm pin5 to GND; pin4 to GND; pin6 through220 ohm to servo signal. Unused channels must have OE high and inputs tied low. See Power guide for all IC pins. Never supply this IC from the 6 V BEC. It is level translation, not a safety interlock.

## Record and send

Control: neutral/left/right pulse endpoints, measured road-wheel map with hysteresis, conservative symmetric usable angle and rate, delay/rise/settling distributions and raw video/trace. Perception later requested 20 steps in each direction at about1,2,3m/s: 120 driving steps, only when the car is ready. Bench tests do not replace that driving evidence. Stock firmware uses a symmetric angle limit and piecewise-linear mapping; flag asymmetry/nonlinearity for a later mapping change.

## References and configuration

[Carson CS-6 article identification](https://www.carson-modelsport.com/carson_en/categories/rc-systems/rc-servos/servo-cs-6-rc-servo-waterproof-metal-gear-500502041-en.html)

[TI AHCT125 electrical and pin specification](https://www.ti.com/lit/ds/symlink/sn74ahct125.pdf)
