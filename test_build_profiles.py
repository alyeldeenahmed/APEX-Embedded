import contextlib
import io
from pathlib import Path
import runpy
import unittest
from unittest.mock import patch

SCRIPT = Path(__file__).parent/'tools/build_firmware.py'

class BuildProfileTests(unittest.TestCase):
    def command(self, *args):
        with patch('sys.argv', [str(SCRIPT), *args]), patch('subprocess.run') as run:
            runpy.run_path(str(SCRIPT), run_name='__main__')
            return run.call_args.args[0]

    def test_vehicle_outputs_are_explicit_and_separate(self):
        command = self.command('--vehicle', '--enable-outputs')
        flags = command[command.index('--build-property')+1]
        self.assertIn('-DAPEX_ENABLE_OUTPUTS=1', flags)
        self.assertIn('-DAPEX_VEHICLE=1', flags)
        self.assertEqual(Path(command[command.index('--output-dir')+1]).name, 'vehicle-outputs')

    def test_sensor_profile_keeps_outputs_disabled(self):
        command = self.command('--vehicle')
        self.assertIn('-DAPEX_ENABLE_OUTPUTS=0', command[command.index('--build-property')+1])
        self.assertEqual(Path(command[command.index('--output-dir')+1]).name, 'vehicle')

    def test_bench_cannot_enable_outputs(self):
        for args in [[], ['--imu'], ['--bench'], ['--encoder-bench']]:
            with self.subTest(args=args), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as error:
                    self.command(*args, '--enable-outputs')
                self.assertEqual(error.exception.code, 2)
