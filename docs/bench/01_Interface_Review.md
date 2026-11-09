# Embedded interface review

9 November 2026. Procedure only; no hardware pass claimed.

Embedded-owned comparison completed on 9 November 2026. Joint interface closure remains pending the specific answers on page 3. No hardware validation is implied.

## Branches checked

All 23 published Control branch tips were checked. Newest: pi-compute-benchmark-v0 at 2a659741; newer workbench: nmpc-lab-v1 at 8ee2b868; relevant local interface: pre-int-w1 at 19073ddb. Main at 48adbdf6 is older. These branches contain separate work, not one cumulative release. Perception reviewed at 857559b4; Embedded runtime baseline 814dfce2.

| Item | Disposition |
| --- | --- |
| Commands | [delta, a_cmd], rad and m/s2. Left steering positive. Dynamic a_cmd = Fx_total/m = dvx/dt - r*vy. Our feedforward force-to-effort meaning agrees; it is not a speed command. |
| Direction | Control rejected unmeasured direction. Quadrature on GPIO5 and GPIO6 is the plan. Keep forward_only_control_confirmed false. Rejection does not approve reverse driving. |
| Rates | Control uses nominal 100 Hz TVLQR; planning timing threshold is 100 ms. Embedded telemetry is 100 Hz; physical PWM 50 Hz. A 100 Hz command stream does not make 100 Hz servo pulses. |
| Limits and delays | Embedded must measure them. No proposed simulation limits are accepted as physical car limits. |
| Geometry | Use ground projection of CG, x forward/y left/z up, consistent with Perception. Replace rear-axle-origin proposal; positions remain unmeasured. |

## Changes made in our working copy

Recorded Control rejection and owners; aligned the proposed geometry reference; corrected the old 50 Hz SET suggestion to a 100 Hz tracker-aligned adapter proposal; clarified that transport authority is still unresolved. Runtime safety gates and calibration remain disabled/unset. No firmware change is justified by inventing missing peer agreement.

## Clock and command authority are not yet an agreed wire contract

The Control local records explicitly are not wire messages. Its examples call propose then check_application. Only an application-checked valid nominal command reaches the mock endpoint. An inhibited output has command=None, not an implicit zero/brake command.

| Boundary | Current implementation / remaining work |
| --- | --- |
| Control lifetime | Trajectory validity ends with its source packet. Local feedback requires exact simulation time. Output carries no physical serial-expiry field. |
| ESP32 lifetime | Rejects future creation, creation older than 100 ms, expired deadline or deadline over 100 ms ahead of arrival. These are implemented local bounds, not peer approval. |
| Adapter proposal | Preserve source authority, map clocks conservatively, and cap expiry by source expiry and allowed MCU lease minus uncertainty. Do not keep extending a stale command. Omar must agree how authority is supplied. |
| Physical output | APPLIED means successful driver calls. It does not measure pulse-edge time, steering position or achieved acceleration. |
| Recovery | Keep MCU expiry/disarm/watchdog for initial testing. Broader state recovery is discussed with Perception. Active braking is not physically validated. |

## Perception comparison

Rev2 accepts 100 Hz, paired timing v1, CRC rules, source timestamps and continuous zero-speed windows. The checked car launch source explicitly says the ESP bridge is not written yet. StateEstimate.msg still uses schema 1.1 without the planned imu_age_ms/wheel_age_ms fields. Timing-v2 diagnostics remain proposed.

Their wheel-stuck detector rejects near-zero after motion above 0.5 m/s and recovers after three plausible nonzero samples. Our applied-drive/no-pulse monitor has unset threshold/time and a reboot-cleared latch. Its agreement and calibration remain pending.

## Exact remaining requests

## Message for Omar

Please confirm the integration branch and exact command/status ROS topics, message types and QoS for apex_esp_bridge as the sole /dev/apex_esp owner. Our implemented frames are type10/type11, 64 bytes each. We need the source-command expiry/authority field, clock mapping into ESP boot time, latest-command queue policy, and reconnect/install/arm ownership. Control examples provide local checked commands, not an ESP serial publisher. We propose tracker-aligned commands at nominal 100 Hz, preserving original authority and the MCU 100 ms maximum deadline bound. Please identify any mismatch before we implement the transport adapter.

## Message for Perception

Control rejected unmeasured wheel direction; we will prepare quadrature. Please close timing-v2 diagnostics acceptance, how separate wheel/steering timestamps are represented, schema1.2 implementation, and coordination between your three-sample recovery and our proposed reboot-latched drive/no-pulse fault. We will use your CG-projected-to-floor geometry reference. Please identify the bridge implementation revision and coordinate ROS command/status transport with Omar.

## Closure criteria

Embedded review is complete. Full joint closure requires those answers and a recorded integration test against the agreed adapter. Physical limits, timing and failure behavior remain hardware tasks. No messages were sent on Aly's behalf and no peer approval is inferred.

## References and configuration

[Control local interface and limitations](https://github.com/Maxrwan/MPC_Controller_v1_IronManStyle/blob/19073ddb318d136f88b0fe45ca122da48fa0e535/docs/pre_sid/preintegration/LOCAL_INTERFACE.md)

[Control application-checked command examples](https://github.com/Maxrwan/MPC_Controller_v1_IronManStyle/blob/19073ddb318d136f88b0fe45ca122da48fa0e535/tests/integration/test_preintegration_local.py)

[Perception car launch and bridge status](https://github.com/FatmaAshraf23/grad-perception/blob/857559b4bf3e23f7e24fb173f7a7be921f122eaf/apex_bringup/apex_bringup/car_nodes.py)

[Perception state schema](https://github.com/FatmaAshraf23/grad-perception/blob/857559b4bf3e23f7e24fb173f7a7be921f122eaf/apex_msgs/msg/StateEstimate.msg)
