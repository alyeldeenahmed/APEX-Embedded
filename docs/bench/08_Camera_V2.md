# Camera Module V2 connection and test

9 November 2026. Procedure only; no hardware pass claimed.

CameraV2 IMX219 connects to the Pi, not to ESP GPIO. First verify images on Raspberry Pi OS, then use Perception's host camera/ROS-container pipeline. Camera detection-model and calibration files come from Perception.

| Component connection | Destination / instruction |
| --- | --- |
| Camera15-pin CSI socket | 15-pin end of proper Pi5 camera adapter ribbon |
| Pi CAM/DISP0 22-pin socket | 22-pin end of same camera cable |
| Power / data / ground | All supplied through CSI ribbon; no separate ESP or5V jumper |
| Cable contacts | Match connector contact orientation using official instructions; fully seat and latch with all power removed |

1. Shut down Pi and unplug power. Fit the correct22-to-15-pin cable without forcing either latch. Keep the cable clear of fan blades and sharp edges.

2. Boot Raspberry Pi OS. List cameras and save a still. If no camera is listed, power down before reseating; check cable type/contacts before changing software.

3. Record image resolution, focus/blur, lens obstruction and exposure. Use a stable test target and check the full field of view.

4. Run Perception's host camera timing path without a detector first. Only one process may use the camera at a time.

```text
rpicam-hello --list-cameras
rpicam-still --nopreview -o camera-test.jpg
```

If rpicam commands are missing, install the Raspberry Pi OS camera applications through its normal package setup; do not switch to a generic USB-camera driver. The ribbon is a differential-camera harness; no hand-made straight pin-to-pin adapter is specified here.

## Perception pipeline and evidence

At the reviewed revision, Pi host picamera2 sends detection packets by UDP5600 to detection_bridge in the ROS container. The host captures full sensor framing1640x1232 and outputs820x616 at a requested30Hz. Detector throughput may be lower; frames are skipped rather than queued. Hardware operation remains to be measured.

```text
# From Perception repo on the Pi host, with its environment ready
~/camenv/bin/python pi_camera/cam_detect.py --source picamera2:820x616:30 --detector none --exposure-us 4000 --gain 4
```

For detection, use the exact model and calibration from Perception's pi_camera/README.md. Their proposed ROS command is below; replace the file path with the actual calibrated file and use the configured container environment.

```text
ros2 run camera_publisher detection_bridge --ros-args -p camera_info_file:=/actual/path/calibration_1640x1232_pi.yaml
ros2 topic hz /camera/detections
```

1. Record at least60 seconds of rate and capture-to-publish statistics with detector off, then with the intended model. Retain dropped/lost/future-stamp counters and Pi CPU/temperature.

2. Run camera calibration at the actual sensor mode/crop; do not reuse calibration from a different image framing without correct scaling. Measure lens position and rotation relative to the CG ground reference.

3. For an independent latency check, record a timestamped visual change and image response using a shared reference. ROS topic arrival rate alone is not exposure-to-estimator latency. Use Perception's latency_probe procedure for their pipeline.

## Record and send

Perception: still image, model/calibration IDs, camera mode/crop/exposure/gain, capture and publish rates, latency distribution with clock method, dropped counts and mount pose. Preserve camera host timestamps; container and host share kernel realtime clock in their design. If the ROS bridge/model is unavailable, deliver the basic camera evidence and mark pipeline measurements pending.

## References and configuration

[Official camera connection instructions](https://www.raspberrypi.com/documentation/accessories/camera.html)

[Perception host camera workflow](https://github.com/FatmaAshraf23/grad-perception/blob/857559b4bf3e23f7e24fb173f7a7be921f122eaf/pi_camera/README.md)
