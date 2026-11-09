# Embedded interface review and closure record

9 November 2026. Embedded-owned repository comparison is complete. Joint ROS,
clock/authority and recovery decisions remain open; this is not a signed integration
release. No physical tests were run. Control's eight-point reply supplied by Aly
supersedes earlier unsigned-direction proposals.

## Revisions inspected

All 23 published Control branch tips were checked by commit date. Newest:
`codex/pi-compute-benchmark-v0` at `2a6597410309a187ddbfd1ce393dcc09af71a7bb`
(9 November). Also reviewed `codex/nmpc-lab-v1` at
`8ee2b868e653d687383d6c4e2d38a3bece31319a` (9 November), and the relevant
`codex/pre-int-w1` at `19073ddb318d136f88b0fe45ca122da48fa0e535` (4 November).
These are different branches: the newest benchmark does not contain the W1 local
interface file. Main remains `48adbdf64d492592d15e23c917aca71fa58aa8a5`.
Perception main reviewed at `857559b4bf3e23f7e24fb173f7a7be921f122eaf`.
Embedded runtime baseline: `814dfce258f0046f8fe4f426be563c7bf64c7ba7`.

## Compatibility findings

| Item | Evidence and disposition |
|---|---|
| Command quantities | Control uses [delta, a_cmd], radians and m/s2; delta positive left. Dynamic a_cmd = Fx_total/m = dvx/dt - r*vy. Embedded force-to-effort interpretation agrees. Do not integrate acceleration into a desired speed or subtract r*vy from an already corrected a_cmd. |
| Negative command | Negative force request is braking in forward operation. Current Embedded assumes a verified brake-only ESC mode and inhibits measured reverse. ESC mode/physical braking still unverified. Rejection of unmeasured direction does not authorize reverse driving. |
| Rate | Control benchmark has nominal 100 Hz TVLQR and 100 ms planning timing threshold. Embedded telemetry is 100 Hz; commands are serviced on receipt; actuator PWM is 50 Hz. Previous 50 Hz SET text was an Embedded proposal, not Control's rate. Proposed adapter follows validated tracker output up to nominal 100 Hz; do not confuse planner releases, serial messages and physical PWM edges. |
| Lifetime | Control trajectory horizon/validity is not a serial command lease. W1 Output has no transport-expiry field; feedback is local-simulation time and exact-time checked. Embedded rejects creation more than 100 ms old/future creation and deadlines expired or over 100 ms ahead of receipt. No repository evidence establishes joint acceptance of this physical clock mapping/lease. Omar adapter must preserve original authority; never renew a cached command merely because serial is alive. |
| Adapter | Control examples in tests/integration/test_preintegration_local.py call propose then check_application; only application_checked VALID_NOMINAL commands reach the mock endpoint. None means inhibited, not a zero command. No ready-made ESP32 serial publisher is established by those examples. Adapt them after clock/lease agreement. |
| Direction | Control explicitly rejects initial unmeasured-direction operation. Quadrature A GPIO5/B GPIO6 is the project plan. Existing forward_only_control_confirmed remains false. Timing v2 magnitude-only mode remains an offline development facility, not approved driving. |
| Geometry | Replace old rear-axle proposal with Perception's reference: ground projection of CG, x forward/y left/z up. lf/lr and all sensor positions remain unmeasured. Actual vehicle CG is above that ground reference; record CG height separately. |
| Perception stream | Rev2 accepts 100 Hz, v1 pairing/CRC, acquisition times and continuous zero windows. v2 bit assignments still need acceptance. No second owner may open the serial port. car_nodes.py explicitly says ESP bridge is not written yet at reviewed revision. |
| Estimator schema | StateEstimate.msg still declares schema 1.1 without imu_age_ms/wheel_age_ms. v0.4/schema1.2 implementation remains Perception-owned. Contract intent does not prove live schema implementation. |
| Stuck wheel | Perception now rejects near-zero readings when previous wheel/IMU speed exceeds 0.5 m/s and recovers after 3 plausible nonzero samples. Embedded monitor uses applied positive effort and no pulses; threshold/time unset, fault latched until reboot. Different detection and recovery conditions must not be conflated. |
| Measured versus commanded | Steering feedback is commanded road-wheel angle. APPLIED status confirms successful driver calls, not motion or pulse-edge time. Control needs measured ranges, rate limits and delay distributions from bench/driving evidence. |

## Proposed adapter rules for Omar and Perception

These are reviewable requirements, not an implemented ROS adapter or peer approval.

1. One apex_esp_bridge owns /dev/apex_esp. Demultiplex type01/02/03/11; preserve
   source timestamps and boot identity. Current command/status are 64-byte type10/11.
2. Accept only a checked nominal command with its source authority and validity.
   Convert clocks conservatively. Proposed expiry is the earliest of source expiry,
   allowed command lease and MCU's 100 ms receive-relative limit, reduced for clock
   uncertainty. Reject missing/expired authority; do not fabricate timestamps.
3. Use bounded latest-command handling, no stale backlog, and no replay on reconnect.
   Agree exact ROS topic/type/QoS with Omar; earlier UInt8MultiArray topics are proposals.
4. On new boot or loss of clock confidence, invalidate old context and explicitly
   reinstall/rearm only through the agreed supervisor. Do not treat a held ID as
   current authority. Status effort is normalized, not achieved acceleration.
5. Preserve wheel latch times; Perception applies half-window correction once.
   Agree how a vehicle message with one header represents separate steering-write time.
6. Broader DEGRADED/LOST/recovery policy belongs in discussion with Perception.
   Retain MCU expiry/disarm/watchdog in initial tests. No active-braking claim exists.

## Pending decisions and owners

| Owner | Exact remaining answer |
|---|---|
| Omar with Control/Perception | ROS topics/types/QoS, transport-expiry source and clock mapping, bridge implementation location, reconnect/arm ownership. |
| Perception | v2 diagnostics acceptance; separate steering timestamp; schema1.2 delivery; estimator recovery versus MCU latched fault. |
| Embedded owner Aly | Exact quadrature sensor/target; calibration, electrical levels, geometry, actuator limits/delays and logs. |
| Control | Consume measured actuator data. No repeat request for documented units or a new test publisher. Resolve only a demonstrated adapter/authority mismatch through Omar. |

## Source links

- [Newest benchmark](https://github.com/Maxrwan/MPC_Controller_v1_IronManStyle/blob/2a6597410309a187ddbfd1ce393dcc09af71a7bb/exports/pi_compute_benchmark_v0/README.md)
- [W1 interface](https://github.com/Maxrwan/MPC_Controller_v1_IronManStyle/blob/19073ddb318d136f88b0fe45ca122da48fa0e535/docs/pre_sid/preintegration/LOCAL_INTERFACE.md)
- [Command examples](https://github.com/Maxrwan/MPC_Controller_v1_IronManStyle/blob/19073ddb318d136f88b0fe45ca122da48fa0e535/tests/integration/test_preintegration_local.py)
- [State definitions](https://github.com/Maxrwan/MPC_Controller_v1_IronManStyle/blob/8ee2b868e653d687383d6c4e2d38a3bece31319a/docs/STATE_DEFINITIONS.md)
- [Perception car nodes](https://github.com/FatmaAshraf23/grad-perception/blob/857559b4bf3e23f7e24fb173f7a7be921f122eaf/apex_bringup/apex_bringup/car_nodes.py)
- [State message](https://github.com/FatmaAshraf23/grad-perception/blob/857559b4bf3e23f7e24fb173f7a7be921f122eaf/apex_msgs/msg/StateEstimate.msg)
- [Mount reference](https://github.com/FatmaAshraf23/grad-perception/blob/857559b4bf3e23f7e24fb173f7a7be921f122eaf/apex_description/config/sensor_mounts_car.yaml)
- [Wheel detector](https://github.com/FatmaAshraf23/grad-perception/blob/857559b4bf3e23f7e24fb173f7a7be921f122eaf/state_estimation/state_estimation/wheel_stuck.py)
