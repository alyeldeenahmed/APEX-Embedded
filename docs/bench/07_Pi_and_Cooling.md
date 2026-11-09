# Raspberry Pi and cooling test

9 November 2026. Procedure only; no hardware pass claimed.

Use Raspberry Pi5 with a known-good suitable USB-C supply for the first bench tests. This bypasses the unfinished vehicle power carrier. Run Raspberry Pi OS64-bit for the camera-host path described by Perception.

| Pi connector | Connect to |
| --- | --- |
| USB-C power | Known-good Pi5 supply only; GPIO5V input disconnected |
| USB-A port1 | ESP COM USB-C through data cable |
| USB-A port2 | LiDAR original USB adapter when its individual test begins |
| FAN keyed connector | Matching official cooler plug; pin1 +5V, pin2 PWM, pin3 GND, pin4 tach |
| CAM/DISP0 | Correct Pi5 22-pin to CameraV2 15-pin cable, fitted with power off |
| 40-pin header | Unused for power during this bench sequence |

1. Mount cooler properly and plug the keyed FAN lead. Do not repin a generic PC fan by colour. Boot with only keyboard/network as needed; record OS, memory, power supply and cooler.

2. Record idle temperature and throttling before each test. The fan may remain stopped at low temperature; that alone is not a failure.

3. Connect ESP and identify its /dev/serial/by-id path. A fixed /dev/apex_esp name requires the agreed udev rule. Do not assume it is /dev/ttyUSB0 because LiDAR can take that name.

4. With the ROS bridge stopped, use the read-only capture tool and actual device path. Later only the bridge owns that port.

```text
cat /etc/os-release
ls -l /dev/serial/by-id/
vcgencmd measure_temp
vcgencmd get_throttled
```

## Run Control compute benchmark and combined load checks

Use Control branch codex/pi-compute-benchmark-v0 at2a659741. Follow its package README. Docker and buildx must already work on a64-bit OS. Copy the entire exports/pi_compute_benchmark_v0 folder, not only run.sh. Build on adequate power/cooling; it downloads dependencies and performs a real startup solve.

```text
cd exports/pi_compute_benchmark_v0
docker buildx build --platform linux/arm64 --load -t apex-pi-compute:v0 .
./run.sh
```

Run once initially, with no simultaneous benchmark. Default is three2-second synthetic windows, not two seconds of wall time. Preserve both successes and failures. Collect the new results/run-* folder including host.txt, timing_summary.csv and summary.json. Do not install different solver versions or tune the profile to make it pass.

## What to report

Control: startup/recurring compute p50/p95/p99/max, exceedances of100ms planning and10ms tracking, failures, CPU/RSS, temperatures, throttling bits, image ID and power/cooler conditions. These are measured Pi compute observations, not actuator latency or guaranteed deadlines. The previously published benchmark was tested in a VM, not on this Pi.

After individual peripheral passes, repeat a documented workload with camera, LiDAR and ESP active. Check for USB disconnects, supply sag and throttling. Capture diagnostics before/after; historical throttling bits must be distinguished from current bits. Do not silently disable power limits to hide an inadequate supply.

## Pass criteria

Stable boot and USB, no unexplained reset/undervoltage under tested load, fan responds to temperature policy, and complete retained benchmark results. A deadline exceedance is a result for Control, not an excuse to discard the run. Final regulator/battery operation is a separate Power guide test.

## References and configuration

[Raspberry Pi hardware and fan connector](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html)

[Pinned Control benchmark instructions](https://github.com/Maxrwan/MPC_Controller_v1_IronManStyle/blob/2a6597410309a187ddbfd1ce393dcc09af71a7bb/exports/pi_compute_benchmark_v0/README.md)
