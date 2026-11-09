# Quadrature encoder connection and test

9 November 2026. Procedure only; no hardware pass claimed.

Control requires measured direction. Exact sensor and target are not yet selected. This guide defines the required electrical interface and tests; it does not assign an unknown module pinout. Keep motor and ESC disconnected for hand-turn tests.

| Signal | Connection | Requirement |
| --- | --- | --- |
| Channel A output | GPIO5 | 0-3.3 V logic only |
| Channel B output | GPIO6 | 0-3.3 V logic only; phase offset from A |
| Both sensor grounds | ESP GND | Common return |
| Sensor supply | Specified regulated rail | Use actual sensor/module datasheet |
| Bare verified Allegro A3144 EU | Pin1 +5V; pin2 GND; pin3 OUT | Use package drawing for orientation. Each OUT: 4.7 kohm to ESP3V3; 100 nF across each supply. A to GPIO5, B to GPIO6. |
| TCRT5000 module alternative | VCC/GND as specified; conditioned DO to A/B | Do not assume 5 V DO is ESP-safe. AO unused. Bare optical parts need an LED/detector circuit. |

## Pattern and scale before purchase

Quadrature requires phase-shifted signals, not two colocated sensors. For an optical pattern with P full cycles per shaft revolution, offset channels by one quarter of the pattern pitch (90 electrical degrees). Hall target geometry must produce an equivalent sequence and be checked on a logic analyser. Do not assume sparse magnets can achieve the desired resolution.

Effective counts/wheel revolution = 4 x P x shaft-revolutions/wheel-revolution. Metres/count = measured rolling circumference / effective counts. Speed increment at 10 ms = circumference / (counts x 0.01). Example only: 0.19 m and 400 counts give 0.0475 m/s. The 400-count target remains a proposal; measure actual geometry. Keep both wheels on an axle rotating together when measuring differential ratios.

## Hand test and telemetry

1. Before attaching ESP inputs, power the sensor circuit and measure A/B high and low levels. Use a logic analyser to confirm a Gray sequence such as 00-01-11-10-00 and the reverse sequence backward. Double-bit jumps indicate phase/noise problems.

2. Set quadrature=true in vehicle_config.h. Leave outputs_enabled=false. Build/upload --vehicle. With wheel_confirmed=false and scale=0, inspect raw counts only; invalid wheel flags are expected.

3. Capture ten complete shaft turns forward and ten backward; repeat three times. Record signed count change and expected x4 counts. Confirm return near the starting count and repeatability without unexplained missed edges. Set encoder_sign from physical forward direction.

4. Measure loaded tyre circumference by rolling several complete turns on a straight surface. Measure shaft/wheel ratio with both wheels coordinated. Calculate metres_per_count and speed quantization. Only then set scale and wheel_confirmed for the sensor-only test.

5. Record 60 seconds stopped, then slow forward and backward hand turns. Fresh latch timestamps must continue at rest; wheel speed must be zero when stopped and signed on motion. Keep the actuator outputs disabled throughout.

```text
python tools/build_firmware.py --vehicle
python tools/capture_serial.py --port COM_NUMBER --seconds 60 --output captures/encoder-01
python protocol.py decode captures/encoder-01/raw.bin > captures/encoder-01/decoded.jsonl
```

## Pass and later work

Initial pass: clean phased A/B, correct sign, repeatable counts and continuous fresh zero-speed reports after calibration. A disconnected sensor at standstill may still look like zero; this test cannot prove idle sensor health. Later verify maximum edge rate and noise filtering under motor interference. A low-speed resolution failure needs a different target or agreed estimator method, not a fabricated scale.

## Record and send

Send sensor/target photos, P, ratio, circumference, effective counts/wheel, metres/count, three forward/reverse count trials, A/B trace and raw capture. Leave the drive/no-pulse threshold and timeout unset until motor startup and pulse spacing are measured. Do not force wheel_valid to satisfy the estimator.

## References and configuration

[Allegro A3144 package and electrical specification](https://www.allegromicro.com/~/media/Files/Datasheets/A3141-2-3-4-Datasheet.ashx?la=en)
