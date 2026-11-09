# LiDAR connection and scan test

9 November 2026. Procedure only; no hardware pass claimed.

Project inventory identifies RPLIDAR A1M8. Check the actual unit/adapter label first. Use its original adapter and harness; internal connector revisions are not confirmed, so no guessed motor/UART pin order is supplied.

| Connection | Destination / rule |
| --- | --- |
| LiDAR factory communication/motor harness | Matching original A1M8 adapter sockets, with power off |
| Adapter USB | Pi USB-A data port or laptop for vendor test |
| ESP32 GPIO | No connection: Pi owns the LiDAR driver |
| Extra motor/power input | Only if required by the exact adapter manual; do not parallel an improvised external supply with USB |

1. Secure the sensor level with free clearance around its rotating head. Keep protective packaging off the optical window. Connect factory cables before power.

2. Identify the adapter by USB serial ID. Keep its device path separate from ESP. For the confirmed A1M8 profile, Perception uses115200 baud, requested10Hz scans and Sensitivity mode; actual delivered scan rate must be measured.

3. Use the vendor viewer first if ROS is not prepared. With ROS, use Perception's existing car LiDAR launch after reviewing its help and actual device/mount configuration. Do not copy simulator geometry into the car as measured data.

4. Place a flat target at measured distances such as0.5,1 and2m within the unit's valid range. Record mean/error/spread and missing returns; distinguish range offset from mounting offset.

```text
ls -l /dev/serial/by-id/
ros2 launch apex_bringup car.launch.py --show-args
```

## Timing geometry and delivery

After starting the LiDAR stage with the actual arguments shown by the launch file, inspect the raw and deskewed topics. Raw data can be tested before IMU/wheel integration; deskew quality requires valid motion inputs.

```text
ros2 topic hz /scan_a1_raw
ros2 topic echo /scan_a1_raw --once
ros2 bag record -o lidar-01 /scan_a1_raw /scan_a1 /tf_static
```

1. Record60 seconds stationary. Retain header timestamps, scan_time, time_increment, angular limits, invalid-return fraction and observed frequency. A requested10Hz is not a10Hz measurement.

2. Put the target straight ahead to verify yaw/angle convention. Measure spinning-head centre x/y/z from the CG ground reference; record yaw and physical mounting photo. Do not rotate scan data twice in both the driver and TF.

3. Check cable movement with motor off for intermittent USB disconnects. Reboot/reconnect and confirm the correct sensor gets its stable device name; serial order alone is not reliable.

4. Later record motion with IMU/wheel data and confirm Perception's reverse scan-time ordering/deskew assumptions against the actual device. Raw scan timing does not establish localization latency.

## Pass and troubleshooting

Initial pass: identified sensor, continuous scans without unexplained restarts, correct forward direction, reasonable known-target ranges and recorded actual scan rate. No spin/scan: check USB power and factory motor control path, port ownership, permissions and exact adapter mode. Do not connect a presumed5V UART output to ESP. If exact model differs, stop and use its own manual/configuration.

## Record and send

Perception: rosbag or vendor capture, actual rate and scan timing, distance-test rows, missing-return statistics, serial configuration, mounting pose/photo and restart results. Retain raw scans even if deskew cannot yet run.

## References and configuration

[SLAMTEC A1M8 connection manual](https://wiki.slamtec.com/download/attachments/83066883/LM108_SLAMTEC_rplidarkit_usermaunal_A1M8_v2.2_en.pdf?api=v2)

[Perception real-car driver configuration](https://github.com/FatmaAshraf23/grad-perception/blob/857559b4bf3e23f7e24fb173f7a7be921f122eaf/apex_bringup/apex_bringup/car_nodes.py)
