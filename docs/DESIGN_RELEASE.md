# Electrical and mechanical release specification

4 November 2026. Design requirements only; not an implemented protection circuit.
Read with POWER_DESIGN.md, PIN_WIRING.md and DECK_HARNESS.md. Procurement: Egypt/local
shops. Named imported candidates are not confirmed local stock.

## Proposed protection behavior to implement after parts are selected

Battery -> source fuse -> service disconnect -> separate traction/electronics branches.
Keep high-current returns separate to the battery return point. ESC BEC supplies only
the verified servo load. The protected 5 V electronics branch supplies Pi and downstream
USB devices. Converter output protection must precede the Pi header carrier.

A per-cell monitor is needed: total pack voltage cannot reveal one weak cell. Its
battery connections, input protection, accuracy, fault behavior and connector pinout
must come from the selected device documentation. Do not wire a balance tap directly
to ESP GPIO or invent ADC divider values before choosing the architecture.

Proposed power supervisor states (not firmware implemented):

| State | Required behavior | Release evidence |
|---|---|---|
| Normal | Allow qualified electronics supply; MCU actuation still needs explicit arm | Startup and power-good specification |
| Low-cell warning | Latch warning, reject new autonomous arm; request controlled stop and Pi shutdown | Threshold/hysteresis based on selected pack and monitor |
| Shutdown pending | Keep a reserved electronics energy budget while Pi shuts down; prevent traction rearm | Measured shutdown time and bounded reserve |
| Disconnect | Remove all discharging loads except rated low-quiescent monitor; no automatic reboot loop | Switching topology and worst-case leakage |
| Critical electrical fault | Hardware cutoff independent of Pi/ESP; immediate cutoff may lose data | OVP/short/critical-cell thresholds and trip-time design |

The warning-to-disconnect interval and thresholds are intentionally unset. They must
account for load sag, cell chemistry, tolerances, regulator dropout and shutdown energy.
The existing ESC cutoff is not a full-system battery disconnect. A buzzer is an alarm,
not cutoff. A fuse is not overvoltage protection. MCU software is not a substitute for
hardware fault interruption. Pi shutdown signal wiring/protocol must be selected with
the supervisor/carrier; no spare GPIO is claimed as allocated yet.

## Required selection record for each purchased module

Record vendor link, exact part/revision, manufacturer datasheet, input/output limits,
continuous and transient ratings, temperature/derating, quiescent draw, fault/restart
behavior, pin orientation, connector current rating and local availability. For a
substitute, check the complete record rather than matching the advertised ampere value.

| Item | Required design closure before assembly |
|---|---|
| Converter U1 | Whole 2S operating envelope, full electronics startup/load, dropout and cooling |
| Protection U3/carrier | Output accuracy and trip thresholds compatible with Pi; no backfeeding; rated contacts |
| Battery monitor/disconnect | Individual cells, shutdown reserve, fail behavior, low leakage and manual recovery |
| F1/F2/F3 and S1 | Wire/connector protection, DC interrupt rating, inrush and overload coordination |
| Battery/charger | Standard 2S LiPo, balance charging, dimensions, connector and retained capacity evidence |
| Servo/ESC/motor | Exact variants, supported voltages, PWM/brake semantics and transient current limits |
| Shaft sensing | Measured direction or explicitly revised validity contract; conditioning and pulse-rate envelope |

## Mechanical drawing inputs

Need wheelbase region available for deck, chassis mounting-point coordinates, hole
sizes/heights, battery envelope/removal route, steering and suspension travel, body
clearance, actual PCB holes, connector/cable bend space and LiDAR scan-plane clearance.
Produce dimensioned deck, separate sensor bracket, raised LiDAR support and fastener
BOM only after inputs are recorded. The current top views are placement concepts.

## Release gate

Electrical assembly release requires reviewed parts, no unresolved power net/pin,
connector polarity drawings and a current-limited bring-up procedure. Vehicle release
additionally requires measured fuse/thermal/transient margins, validated calibration,
physical fault behavior and cross-team acceptance. These are separate milestones.
