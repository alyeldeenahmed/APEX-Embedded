"""Compile only: never upload or open a serial port."""
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
config = json.loads((ROOT / "firmware/build-config.json").read_text())
subprocess.run(["arduino-cli", "compile", "--fqbn", config["fqbn"],
                "--warnings", "all", "--output-dir", str(ROOT / "build/esp32s3-n8r2"),
                str(ROOT / "firmware/telemetry_demo")], check=True)
