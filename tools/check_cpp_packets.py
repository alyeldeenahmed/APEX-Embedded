"""Compile the firmware serializer on the host and verify byte interoperability."""
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from protocol import Decoder, data_packet, sync_packet, timing_packet


def command(case):
    if case['type']=='timing':
        v=case['values']
        return 'T '+' '.join(str(v[k]) for k in ('seq','present','report_us','imu_us','wheel_us','steering_us'))
    if case["type"] == "sync":
        return f"S {case['timestamp']}"
    v = case["values"]
    return "D " + " ".join(map(str, [v["seq"], v["timestamp"], v.get("flags", 0),
        *v.get("gyro", [0., 0., 0.]), *v.get("accel", [0., 0., 9.80665]),
        v.get("ticks", 0), v.get("speed", 0.), v.get("wheel_age", 0),
        v.get("steering", 0.), v.get("steer_age", 0)]))


def main():
    compiler = shutil.which(os.environ.get("CXX", "g++"))
    if not compiler:
        raise SystemExit("C++ compiler missing: install g++ or set CXX; this check does not silently skip.")
    cases = json.loads((ROOT / "tests/packet_vectors.json").read_text())["cases"]
    golden_count = len(cases)
    rng = random.Random(20261102)
    for i in range(200):
        cases.append(dict(name=f"random-{i}", type="data", values=dict(
            seq=rng.randrange(131072), timestamp=rng.randrange(2**33), flags=rng.randrange(128),
            gyro=[rng.randint(-1000, 1000) / 8 for _ in range(3)],
            accel=[rng.randint(-1000, 1000) / 8 for _ in range(3)],
            ticks=rng.randint(-2**31, 2**31-1), speed=rng.randint(-100, 100) / 8,
            wheel_age=rng.randrange(65536), steering=rng.randint(-8, 8) / 16,
            steer_age=rng.randrange(65536))))
    for i in range(100):
        cases.append(dict(name=f'timing-{i}',type='timing',values=dict(seq=rng.randrange(65536),
            present=rng.randrange(8),report_us=rng.randrange(2**64),imu_us=rng.randrange(2**64),
            wheel_us=rng.randrange(2**64),steering_us=rng.randrange(2**64))))
    with tempfile.TemporaryDirectory() as tmp:
        exe = Path(tmp) / ("packet_probe.exe" if os.name == "nt" else "packet_probe")
        subprocess.run([compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT),
                        str(ROOT / "tests/packet_probe.cpp"), "-o", str(exe)], check=True)
        result = subprocess.run([str(exe)], input="\n".join(map(command, cases)) + "\n",
                                text=True, capture_output=True, check=True)
    lines = result.stdout.splitlines()
    if len(lines) != len(cases):
        raise AssertionError("Host encoder returned wrong frame count")
    for case, line in zip(cases, lines):
        cpp = bytes.fromhex(line)
        py = sync_packet(case["timestamp"]) if case["type"] == "sync" else (timing_packet(**case['values']) if case['type']=='timing' else data_packet(**case["values"]))
        if cpp != py:
            raise AssertionError(f"C++/Python mismatch: {case['name']}")
        if "hex" in case and cpp.hex() != case["hex"]:
            raise AssertionError(f"Reference-byte mismatch: {case['name']}")
        d = Decoder()
        decoded = []
        for offset in range(0, len(cpp), 7):
            decoded += d.feed(cpp[offset:offset+7])
        if len(decoded) != 1 or d.crc_errors or d.buffer:
            raise AssertionError(f"Decoder rejected firmware frame: {case['name']}")
    print(f"PASS: {golden_count} fixed vectors + 200 data + 100 timing cases; C++/Python bytes match; decoder accepts all frames.")


if __name__ == "__main__":
    main()
