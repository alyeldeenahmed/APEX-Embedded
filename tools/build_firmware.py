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
mode.add_argument('--vehicle',action='store_true',help='Integrated IMU, encoder, command endpoint; outputs disabled by default')
parser.add_argument('--address',choices=['0x68','0x69'],default='0x68')
parser.add_argument('--telemetry-hz',type=int,choices=[100,200],default=100)
parser.add_argument('--wheel-diagnostics-v2',action='store_true',help='Opt-in proposed timing v2; peer acceptance required')
args=parser.parse_args()
if args.wheel_diagnostics_v2 and not args.vehicle: parser.error('Diagnostics require --vehicle')
flags=f'-DAPEX_IMU_ADDRESS={args.address} -DAPEX_TELEMETRY_HZ={args.telemetry_hz}'
if args.imu or args.bench or args.vehicle: flags+=' -DAPEX_USE_MPU6050=1'
if args.vehicle: flags+=' -DAPEX_VEHICLE=1'
if args.bench: flags+=' -DAPEX_IMU_BENCH=1'
if args.wheel_diagnostics_v2: flags+=' -DAPEX_WHEEL_DIAGNOSTICS_V2=1'
extra=['--build-property','build.extra_flags='+flags]
output='vehicle' if args.vehicle else ('imu-bench' if args.bench else ('imu' if args.imu else 'simulation'))
if args.wheel_diagnostics_v2: output+='-wheel-v2'
if args.telemetry_hz!=100: output+=f'-{args.telemetry_hz}hz'
subprocess.run(["arduino-cli", "compile", "--fqbn", config["fqbn"], *extra,
                "--warnings", "all", "--output-dir", str(ROOT / 'build' / output),
                str(ROOT / "firmware/telemetry_demo")], check=True)
