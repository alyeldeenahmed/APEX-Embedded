"""Compile only: never upload or open a serial port."""
import json
from pathlib import Path
import subprocess
import argparse

ROOT = Path(__file__).resolve().parents[1]
config = json.loads((ROOT / "firmware/build-config.json").read_text())
parser=argparse.ArgumentParser(description=__doc__)
mode=parser.add_mutually_exclusive_group()
mode.add_argument('--imu',action='store_true',help='Binary telemetry with MPU-6050')
mode.add_argument('--bench',action='store_true',help='Standalone CSV IMU test, no binary packets')
parser.add_argument('--address',choices=['0x68','0x69'],default='0x68')
args=parser.parse_args()
flags=f'-DAPEX_IMU_ADDRESS={args.address}'
if args.imu or args.bench: flags+=' -DAPEX_USE_MPU6050=1'
if args.bench: flags+=' -DAPEX_IMU_BENCH=1'
extra=['--build-property','build.extra_flags='+flags]
output='imu-bench' if args.bench else ('imu' if args.imu else 'simulation')
subprocess.run(["arduino-cli", "compile", "--fqbn", config["fqbn"], *extra,
                "--warnings", "all", "--output-dir", str(ROOT / 'build' / output),
                str(ROOT / "firmware/telemetry_demo")], check=True)
