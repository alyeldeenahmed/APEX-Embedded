"""Compile only: never upload or open a serial port."""
import json
from pathlib import Path
import subprocess
import argparse

ROOT = Path(__file__).resolve().parents[1]
config = json.loads((ROOT / "firmware/build-config.json").read_text())
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--imu',action='store_true',help='Compile MPU-6050 mode, pins unassigned')
args=parser.parse_args()
extra=['--build-property','build.extra_flags=-DAPEX_USE_MPU6050=1'] if args.imu else []
subprocess.run(["arduino-cli", "compile", "--fqbn", config["fqbn"], *extra,
                "--warnings", "all", "--output-dir", str(ROOT / ('build/imu' if args.imu else 'build/esp32s3-n8r2')),
                str(ROOT / "firmware/telemetry_demo")], check=True)
