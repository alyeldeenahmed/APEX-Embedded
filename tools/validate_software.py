"""Run software checks and record explicit pass/fail/blocked evidence. No hardware I/O."""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--firmware', action='store_true', help='Also compile all five target profiles')
    parser.add_argument('--report', type=Path, default=ROOT/'build/software-validation.json')
    args = parser.parse_args()
    results = []

    def run(name, command):
        result = subprocess.run(command, cwd=ROOT, text=True, encoding='utf-8', errors='replace',
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        results.append(dict(name=name, status='PASS' if result.returncode == 0 else 'FAIL',
                            command=command, returncode=result.returncode, output=result.stdout))
        print(name + ': ' + results[-1]['status'], flush=True)
        return result.returncode == 0

    run('Python unit tests', [sys.executable, '-m', 'unittest', '-v'])
    compiler = shutil.which(os.environ.get('CXX', 'g++'))
    if compiler:
        run('C++/Python packet compatibility', [sys.executable, 'tools/check_cpp_packets.py'])
        with tempfile.TemporaryDirectory() as tmp:
            for name in ('mpu6050', 'vehicle', 'endpoint'):
                exe = str(Path(tmp)/('test_'+name+('.exe' if os.name == 'nt' else '')))
                if run(name+' compile', [compiler, '-std=c++11', '-Wall', '-Wextra', '-Werror',
                                       '-I.', 'tests/test_'+name+'.cpp', '-o', exe]):
                    run(name+' execution', [exe])
    else:
        results.append(dict(name='C++ suites and packet comparison', status='BLOCKED',
                            reason='Install g++ or set CXX to its executable'))
    if args.firmware:
        if shutil.which('arduino-cli'):
            for mode in ([], ['--imu'], ['--bench'], ['--vehicle'], ['--vehicle','--telemetry-hz','200']):
                run('Firmware '+(' '.join(mode) or 'simulation'),
                    [sys.executable, 'tools/build_firmware.py', *mode])
        else:
            results.append(dict(name='Five firmware builds', status='BLOCKED',
                                reason='Arduino CLI and pinned ESP32 core required; see BUILD.md'))
    revision = subprocess.run(['git','rev-parse','HEAD'], cwd=ROOT, text=True, capture_output=True)
    dirty = subprocess.run(['git','status','--porcelain'], cwd=ROOT, text=True, capture_output=True)
    report = dict(created_utc=datetime.now(timezone.utc).isoformat(),
                  revision=revision.stdout.strip(), working_tree_changes=dirty.stdout.splitlines(),
                  firmware_requested=args.firmware, hardware_tested=False, results=results)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('Evidence: '+str(args.report))
    return int(any(r['status'] != 'PASS' for r in results))


if __name__ == '__main__':
    raise SystemExit(main())
