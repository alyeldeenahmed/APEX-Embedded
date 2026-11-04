# Single-battery power proposal P1

Updated 4 November 2026. This is a design proposal, not a hardware acceptance
record. Current owner instruction selects one 2S pack for the whole vehicle and
supersedes the older separate 3S electronics UPS proposal.

## Confirmed inventory and evidence

- Owner transcribed the ESC label: THW-1060-RTR, waterproof brushed, 2-3S LiPo,
  5-9 cell NiMH, SBEC 6 V at 3 A. Do not substitute a Hobbywing WP-1060 manual
  as proof of this unit's behavior.
- Pi 5, Camera Module V2 and Pi cooling fan are included. The earlier local
  equipment BOM identifies the LiDAR as RPLIDAR A1M8 and Pi as the 8 GB variant.
- ESP32-S3 N16R8, MPU-6050 breakout, one shaft sensor, Carson CS-6 servo and
  Tamiya 54358 motor remain in the inventory.
- Intended use: moderate driving and acceleration, 30 minutes. Printed deck
  spans between front and rear wheel areas; exact dimensions are not measured.

## Power budget

Electrical consumption, not mechanical output. Every value other than the
motor manufacturer's reference operating point is a planning allocation or
assumed loss; none is an on-car measurement.

| Load | Planning power |
|---|---:|
| Motor at Tamiya's 7.2 V, 10 A reference load | 72 W |
| ESC traction losses, excluding BEC | 3 W |
| Servo at assumed 6 V, 0.4 A average | 2.4 W |
| Pi 5 board | 15 W |
| RPLIDAR A1M8 including scan motor | 2 W |
| Camera V2, equivalent allowance via CSI | 1.25 W |
| Pi cooler | 1 W |
| ESP development board | 1 W |
| MPU breakout | 0.05 W |
| One shaft sensor | 0.10 W |
| Miscellaneous interfaces | 0.05 W |

Electronics output = 20.45 W. Assuming 88% converter efficiency and 85% BEC
efficiency, whole-battery input = 72 + 3 + 20.45/0.88 + 2.4/0.85 = 101.062 W.
Battery current at nominal 7.4 V is 13.66 A. These efficiencies are assumptions,
not measurements. Include actual sensor supply and onboard regulator losses when
the boards and monitor are finalized.

Runtime = 60 * 7.4 * capacity_Ah * 0.80 / power_W. The 80% factor is an energy
planning reserve; it is not a cell-voltage cutoff setting.

| Standard 2S capacity | Runtime at planning load |
|---|---:|
| 7000 mAh | 24.6 min |
| 8000 mAh | 28.1 min |
| 9000 mAh | 31.6 min |

**Select 2S 9000 mAh as the design target.** The 30-minute theoretical minimum
is 8.54 Ah under these assumptions. A 9000 mAh pack requires average battery
input <=106.56 W to meet 30 minutes with this energy reserve. At motor reference
currents of 5/10/15 A and other assumptions held fixed, runtime is 49.1/31.6/23.3
minutes. Moderate driving alone does not establish actual average current.

## Battery and converter candidates

- Standard 2S LiPo: 7.4 V nominal, 8.4 V full. VANT's 9000 mAh 7.4 V model is
  listed at 138 x 47 x 25 mm, 345 g, Deans main connector and JST-XH balance
  connector. Candidate only: check actual fit, authenticity, usable capacity,
  connector quality and measured peak current. Advertised C-rating is not a
  verified wiring/fuse rating. Do not substitute a LiHV charging profile.
- Pololu D42V55F5 #5571 is the 5 V converter candidate. Its nominal current
  rating does not prove 5 A at depleted 2S voltage in the installed thermal
  environment. Qualify at 6.4-8.4 V, hot ambient and motor transients; also
  establish low-cell shutdown before converter dropout. Output is fixed 5 V
  with 3% accuracy; do not substitute 5.3 V.

## Wiring architecture

1. Battery positive -> source fuse F1 -> DC-rated main disconnect S1 -> branches.
2. Traction branch -> ESC battery input -> motor. ESC's BEC 6 V and return power
   the servo, subject to verifying servo voltage and transient current.
3. Electronics branch -> F2 -> converter VIN. F2 7.5 A is a provisional class,
   requiring coordination with wire, connector and inrush measurements.
4. Converter 5 V -> protected, secured Pi power carrier with F3 -> Pi physical
   pins 2 and 4; return to pins 6 and 9. Use rated parallel contacts, not loose
   Dupont jumpers. Do not simultaneously supply Pi USB-C power. Carrier design,
   protection and F1/F3 ratings remain open; this is not a released PCB design.
5. Pi USB-A -> ESP COM USB-C; another USB-A -> A1M8 original adapter. Camera
   uses the 22-to-15-pin CSI ribbon; cooler uses Pi FAN connector. No USB hub.
6. Grounds are common. Give ESC and converter separate battery return paths.
   Servo current returns through the BEC harness, not through ESP headers.
7. ESC lead BEC+ is routed only to servo distribution. Never feed 6 V into Pi,
   ESP or logic pins. ESP receives power only through its Pi USB link.

GPIO power avoids relying on USB-C PD advertising but requires a genuinely
capable power path. After electrical qualification, configure Pi supply/current
policy for 5 A capability and verify downstream USB/fan allowance (up to 1.6 A).
Do not override limits to hide an inadequate supply. Initial peripheral startup
test envelope: LiDAR 0.8 A, ESP 0.5 A, fan 0.2 A; these are provisional allowances,
not measured peak limits. Verify simultaneous startup and full compute load.

ESP pins remain IMU SDA8/SCL9/INT4; speed A5, optional B6; steering15; ESC16.
GPIO is 3.3 V. Confirm PWM input thresholds or add appropriate buffers. Confirm
the MPU breakout regulator rail before applying supply. Sensor module selection
and pulse conditioning remain open, including A3144 supply and pull-up levels.

## Placement

Keep the battery low in the original chassis tray with secure retention. Leave
motor and servo in their intended chassis positions. Place ESC near motor with
airflow. Upper deck: camera forward, Pi forward/side with cooler clearance, ESP
opposite with antenna at an outer edge, converter near Pi, fuse/disconnect at
an accessible edge. Mount LiDAR on a rigid raised central bridge with clear
360-degree scan. Mount IMU rigidly near CG, away from motor, fan and converter.
Keep shaft, suspension and steering travel clear. Separate motor harnesses from
sensor/CSI wiring; secure connectors and provide strain relief. Measure board
holes, body clearance, mounting points and battery access before printing.

## Remaining release work

- Final battery fit and charger; exact converter/carrier/protection hardware.
- Full-system per-cell monitoring and shutdown/disconnection. ESC motor cutoff
  alone does not stop electronics discharging the battery. This proposal does
  not implement battery monitoring in firmware or automatic Pi shutdown.
- Fuse coordination, harness/connector sizing, supply polarity and rail testing.
  Short 12-14 AWG traction and 18 AWG electronics are starting points only.
- Validate motor operation at full 8.4 V pack: Tamiya publishes 6.6-7.2 V.
  Confirm ESC control modes, logic levels, braking and command-loss behavior.
- Measure servo transients, motor acceleration current, thermal performance,
  USB stability and actual average Wh consumption. Avoid uncontrolled stall tests.
- Complete mechanical measurements, print fit, then representative 30-minute run.

## Primary references

- [Tamiya motor 54358](https://www.tamiya.com/japan/products/54358/index.html)
- [Pi hardware and power](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html)
- [VANT battery candidate](https://www.vantbattery.com/pro.asp?bascode=101&keyno=1032&name=9000mAh-120C-7.4V)
- [Pololu converter candidate](https://www.pololu.com/product/5571)
- [SLAMTEC documentation](https://www.slamtec.com/en/support) (current PDF fetch
  blocked; LiDAR power is retained as a prior-project planning allowance).

## Design release tracking

Procurement is Egypt/local shops; candidate availability is unverified. See
DESIGN_RELEASE.md for full-system cutoff, protection selection and CAD input gates.
