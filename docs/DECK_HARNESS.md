# Two-deck harness and protection purchase proposal D1

4 November 2026. Companion to POWER_DESIGN.md and PIN_WIRING.md. This document
does not release the vehicle for powered operation. Deck layout is a placement
concept, not dimensioned CAD. Preserve actual TT-02 mounts and moving clearances.

## Regulator count

- Buy one candidate 5 V electronics converter: Pololu D42V55F5 #5571. Qualify
  at depleted 2S input, hot ambient, 5 A load and motor interference.
- Existing ESC SBEC 6 V / 3 A supplies the servo, subject to servo verification.
  No separate UBEC purchase unless tests show the existing BEC is insufficient.
- ESP board provides 3.3 V for checked low-current sensors. Do not purchase an
  additional regulator until the actual MPU breakout power circuit is verified.

## Protection procurement

Buy three covered ATO/ATC blade fuse holders: main with 12 AWG leads and at least
30 A rating, branch holders matched to their circuit. Genuine 32 V DC fuse test
stock: two each of 5, 7.5, 20, 25 and 30 A. These are NOT final installation values.

- F1: main battery harness protection. Candidates 20/25/30 A, selected using
  measured acceleration/inrush, fuse time-current curves, ambient temperature
  and wire/connector ampacity. Never simply match the ESC's advertised rating.
- F2: upper-deck input protection; 7.5 A candidate. At 5 V / 5 A output,
  6.4 V input and assumed 88% efficiency, converter input is about 4.44 A.
- F3: output fuse/carrier protection; 5 A is only test stock. Full-load thermal
  derating and added voltage drop require checking. Coordinate with U3 current
  limit and downstream conductors; do not increase a nuisance-blowing fuse
  without checking the complete circuit.

U3 is a new explicitly unselected assembled 5 V overvoltage/current protection
stage between U1 and protected 5V_ELEC. It must carry at least 5 A with low drop,
and its overvoltage disconnect tolerance must suit the Pi supply limit. A
TPS25947-family design is a reference, not a plug-in purchase recommendation.
Do not buy a bare eFuse IC as though it were a completed module. The Pi power
carrier also requires a rated, secured power connection; no loose Dupont leads.

A 2S per-cell alarm such as DYNF0002 can be purchased for supervised testing.
It does not disconnect the pack or shut down Pi. Automatic whole-system cell
protection and stop/shutdown/disconnect remain to be designed. A generic 2S BMS
must not be assumed compatible with motor peaks or ESC regenerative behavior.
Fuses protect wiring from current; they do not provide Pi overvoltage protection.

## Harness assignments

All cavity numbers below are OUR assignments, not inferred manufacturer pinouts.
Label both mating halves; confirm mating-face orientation and continuity.

| Connector / cavity | Lower deck | Upper deck |
|---|---|---|
| J_PWR 1 / + | F2 output, switched 2S | U1 VIN |
| J_PWR 2 / - | Battery return star | U1 GND |
| J_ACT 1 | ESC signal | U2 pin 3 through 220 ohm |
| J_ACT 2 | Servo signal | U2 pin 6 through 220 ohm |
| J_ACT 3 | ESC/servo signal reference | U2/ESP common GND |
| J_IMU 1 | MPU VCC (conditional) | ESP 3V3 |
| J_IMU 2 | MPU GND, AD0 local ground | ESP GND |
| J_IMU 3 | MPU SDA | ESP GPIO8 |
| J_IMU 4 | MPU SCL | ESP GPIO9 |
| J_IMU 5 | MPU INT | ESP GPIO4 |
| J_IMU 6 | Unused | Unused |
| J_WHEEL 1 | Bare A3144 EU pin 1, if selected | Protected 5V_ELEC |
| J_WHEEL 2 | Bare A3144 EU pin 2 | ESP sensor GND |
| J_WHEEL 3 | Bare A3144 EU pin 3 | ESP GPIO5, 4.7 kohm pull-up to 3V3 |
| J_WHEEL 4 | Unused today | Reserve GPIO6 for true quadrature B |

J_PWR uses an XT30 paired power connector with 18 AWG conductors. J_ACT uses
a keyed 3-way signal connector; J_IMU a keyed 6-way; J_WHEEL a keyed 4-way.
Use locking, differently identified harnesses and strain relief. Main power
distribution uses suitable keyed connectors such as genuine XT60; confirm
actual battery mating connector before purchasing adapters.

Servo +6 V and high-current return remain on the lower deck between BEC and
servo. No BEC positive is present in any inter-deck signal connector. All GND
nets are common, but motor/servo current must not use sensor or signal returns.

## Placement

Lower: battery in low stock tray, motor and servo at stock mounts, ESC near
motor, F1 near battery positive, accessible S1, F2 near the upper-deck supply
exit, shaft sensor on a fixed bracket and MPU near CG on a rigid support.

Upper: camera forward, Pi with cooler and accessible ports, ESP with antenna at
an outer edge, LiDAR on a raised rigid bridge with an unobstructed scan plane,
and converter/protection/connector carrier with cooling and service access.
Keep camera, USB and sensor wiring away from motor leads. Measure all holes,
body/suspension clearance, battery removal route and connector bend space before
printing. The component arrangement is not a physical fit confirmation.

## Other buy-now items

- SN74AHCT125N DIP-14 plus spare and sockets; circuit in PIN_WIRING.md.
- Ten each: 10 kohm, 4.7 kohm and 220 ohm 1% 0.25 W resistors.
- Ten 100 nF ceramic capacitors, >=25 V, X7R preferred.
- Red/black 12 AWG and 18 AWG wire, 1 m each colour/gauge; signal wire assortment.
- Heat-shrink, insulating boots, mounting standoffs, cable ties and strain relief.
- Correct USB data, original LiDAR adapter and 22-to-15-pin camera cables if missing.
- Borrow/buy a multimeter, current-limited bench supply and suitably rated
  battery wattmeter/logger. Do not measure motor peaks through an underrated meter.

Exact local substitutions, U3, carrier, cutoff, disconnect and fitted fuse values
remain open. No complete automatic electrical protection is claimed.
