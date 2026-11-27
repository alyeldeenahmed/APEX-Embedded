import struct
import unittest
import binascii

from car_link import CarLink, MotorTelemetry
from control_link import crc
from protocol import data_packet, timing_packet


class FakeESP:
    """A byte-level acknowledgement peer for the Pi transport tests."""
    def __init__(self):
        self.buffer = bytearray()
        self.actions = []
        self.receipts = []
        self.reply(0, 0, 0, 0, 4)
        self.pair(1, 10200)

    def pair(self, sequence, stamp):
        self.buffer.extend(data_packet(sequence, stamp))
        self.buffer.extend(timing_packet(sequence, stamp, 0, 0, 0))

    def reply(self, code, sequence, session, ident, flags, pending=0, boot=17, receipt_capable=True):
        p = bytearray(64)
        p[:6] = bytes([165, 90, 17, 1, code, flags | (32 if receipt_capable else 0)])
        p[6] = pending
        struct.pack_into('<6IQ', p, 8, boot, sequence, session, 1, ident, ident,
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
        assert crc(data[2:-2]) == int.from_bytes(data[-2:], 'little')
        if data[2] == 0x12:
            self.receipts.append(data)
            return len(data)
        action = data[4]
        boot, sequence, session, _, ident = struct.unpack_from('<5I', data, 8)
        assert boot == 17
        self.actions.append(action)
        self.reply({1:1, 2:3, 3:3, 5:6}[action], sequence, session, ident, 4)
        return len(data)


class IntegrationTests(unittest.TestCase):
    def test_old_firmware_cannot_arm_on_one_way_telemetry(self):
        port = FakeESP()
        port.buffer.clear()
        port.reply(0, 0, 0, 0, 4, receipt_capable=False)
        port.pair(1, 10500)
        car = CarLink(port)
        with self.assertRaisesRegex(RuntimeError, 'paired ESP32'):
            car.arm()
        self.assertEqual(port.receipts, [])
        self.assertEqual(port.actions, [])

    def test_pi_receipt_wire_and_validity_independence(self):
        port = FakeESP()
        car = CarLink(port)
        events = car.poll()
        self.assertEqual(events[0]['unacked_pairs'], 0)
        self.assertFalse(events[0]['link_lost'])
        body = struct.pack('<BBIHQ', 0x12, 1, 17, 1, 10200)
        self.assertEqual(port.receipts, [b'\xa5\x5a'+body+struct.pack('<H', binascii.crc_hqx(body, 0xffff))])
        self.assertEqual(port.actions, [])
        self.assertEqual(car.receipts_sent, 1)

    def test_confirmed_pair_only_across_corruption_fragmentation_and_wrap(self):
        port = FakeESP()
        port.buffer.clear()
        car = CarLink(port)
        port.pair(3, 900)  # No status yet.
        car.poll()
        port.reply(0, 0, 0, 0, 0)
        broken = bytearray(data_packet(2, 901))
        broken[20] ^= 1
        port.buffer.extend(broken + timing_packet(2, 901, 0, 0, 0))
        port.buffer.extend(data_packet(7, 902) + timing_packet(8, 902, 0, 0, 0))
        port.buffer.extend(data_packet(9, 903))
        car.poll()
        self.assertEqual(port.receipts, [])
        port.buffer.extend(timing_packet(9, 903, 0, 0, 0)[:11])
        car.poll()
        self.assertEqual(port.receipts, [])
        port.buffer.extend(timing_packet(9, 903, 0, 0, 0)[11:])
        port.pair(9, 903)  # Duplicate pair.
        port.pair(65535, 200000)
        port.pair(0, 210000)
        car.poll()
        self.assertEqual([struct.unpack_from('<HQ', p, 8) for p in port.receipts],
                         [(9, 903), (65535, 200000), (0, 210000)])

    def test_receipts_stop_until_boot_confirmed_after_clock_reversal(self):
        port = FakeESP()
        car = CarLink(port)
        car.poll()
        port.pair(6, 50)
        port.pair(7, 60)
        car.poll()
        self.assertEqual(car.receipts_sent, 1)
        port.reply(0, 0, 0, 0, 4, boot=18)
        port.pair(1, 70)
        car.poll()
        self.assertEqual(struct.unpack_from('<IHQ', port.receipts[-1], 4), (18, 1, 70))

    def test_link_loss_invalidates_session_even_when_receipts_resume(self):
        port = FakeESP()
        car = CarLink(port)
        car.arm()
        port.reply(17, 2, 1, 1, 0x14, pending=4)
        with self.assertRaisesRegex(RuntimeError, 'receipts lost'):
            car.poll()
        self.assertTrue(car.last['link_lost'])
        self.assertEqual(car.last['unacked_pairs'], 4)
        port.pair(2, 30000)
        port.reply(17, 2, 1, 1, 4)
        car.poll()
        self.assertFalse(car.last['link_lost'])
        with self.assertRaises(RuntimeError):
            car.set(0, 0)
        self.assertEqual(port.actions, [1, 2])

    def test_restart_during_armed_session_discards_authority(self):
        port = FakeESP()
        car = CarLink(port)
        car.arm()
        port.reply(0, 0, 0, 0, 4, boot=18)
        with self.assertRaisesRegex(RuntimeError, 'restarted'):
            car.poll()
        with self.assertRaises(RuntimeError):
            car.set(0, 0)

    def test_partial_receipt_write_clears_session(self):
        port = FakeESP()
        car = CarLink(port)
        car.arm()
        port.pair(7, 30000)
        port.write = lambda packet: len(packet)-1
        with self.assertRaisesRegex(OSError, 'Incomplete'):
            car.poll()
        self.assertIsNone(car.session)
        self.assertEqual(car.receipts_sent, 1)

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
