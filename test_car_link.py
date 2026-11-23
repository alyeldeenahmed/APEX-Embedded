import struct
import unittest

from car_link import CarLink, MotorTelemetry
from control_link import crc


class FakeESP:
    """A byte-level acknowledgement peer for the Pi transport tests."""
    def __init__(self):
        self.buffer = bytearray()
        self.actions = []
        self.reply(0, 0, 0, 0, 4)

    def reply(self, code, sequence, session, ident, flags):
        p = bytearray(64)
        p[:6] = bytes([165, 90, 17, 1, code, flags])
        struct.pack_into('<6IQ', p, 8, 17, sequence, session, 1, ident, ident,
                         10000+sequence*500)
        struct.pack_into('<H', p, 62, crc(p[2:62]))
        self.buffer.extend(p)

    @property
    def in_waiting(self):
        return len(self.buffer)

    def read(self, n):
        data = bytes(self.buffer[:n])
        del self.buffer[:n]
        return data

    def write(self, data):
        action = data[4]
        assert crc(data[2:-2]) == int.from_bytes(data[-2:], 'little')
        boot, sequence, session, _, ident = struct.unpack_from('<5I', data, 8)
        assert boot == 17
        self.actions.append(action)
        self.reply({1:1, 2:3, 3:3, 5:6}[action], sequence, session, ident, 4)
        return len(data)


class IntegrationTests(unittest.TestCase):
    def test_unarmed_and_stale_commands_never_write(self):
        port = FakeESP()
        car = CarLink(port)
        with self.assertRaises(RuntimeError):
            car.set(0, 0)
        car.arm()
        actions = list(port.actions)
        car.last_received = 0
        with self.assertRaises(RuntimeError):
            car.set(0, 0)
        self.assertEqual(port.actions, actions)

    def test_restart_clears_rpm_history(self):
        motor = MotorTelemetry()
        motor.feed(dict(type='status', boot=1))
        motor.previous = (1, 0, 10000)
        motor.sample = dict(type='data', flags=0, seq=2, enc_ticks=100)
        motor.feed(dict(type='status', boot=2))
        event = dict(type='timing', seq=2, present=2, wheel_us=20000)
        motor.feed(event)
        self.assertNotIn('motor_rpm', event)

    def test_report_clock_reversal_discards_pair(self):
        motor = MotorTelemetry()
        motor.report = 50000
        motor.previous = (1, 0, 10000)
        motor.feed(dict(type='data', flags=0, seq=2, enc_ticks=100))
        event = dict(type='timing', seq=2, present=2, wheel_us=20000, report_us=30000)
        motor.feed(event)
        self.assertIsNone(motor.previous)
        self.assertNotIn('motor_rpm', event)

    def test_explicit_session(self):
        port = FakeESP()
        car = CarLink(port)
        car.arm()
        car.set(.05, .1)
        car.stop()
        self.assertEqual(port.actions, [1, 2, 3, 5])

    def test_rpm_wrap_and_discontinuity(self):
        motor = MotorTelemetry()
        def sample(seq, count, stamp, present=2):
            motor.feed(dict(type='data', flags=0, seq=seq, enc_ticks=count))
            timing = dict(type='timing', seq=seq, wheel_us=stamp, present=present)
            motor.feed(timing)
            return timing.get('motor_rpm')
        self.assertIsNone(sample(65535, 2**31-8192, 10000))
        self.assertEqual(sample(0, -2**31+8192, 20000), 6000.)
        self.assertIsNone(sample(1, 0, 30000, 0))
        self.assertIsNone(sample(2, 0, 40000))
        self.assertEqual(sample(3, -8192, 50000), -3000.)
