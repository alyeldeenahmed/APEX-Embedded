import io
import json
import struct
import unittest
from protocol import data_packet, timing_packet, frame
from tools.replay_capture import replay


def boot_packet(boot):
    body = bytearray(60)
    body[:2] = bytes([0x11, 1])
    struct.pack_into('<I', body, 6, boot)
    return frame(body)


def pair(seq, stamp, flags=3, imu=None):
    imu = stamp if imu is None else imu
    return (data_packet(seq, imu, flags=flags) +
            timing_packet(seq, stamp, imu, stamp, 0, 3))


class ReplayTests(unittest.TestCase):
    def run_capture(self, data, chunk=7):
        sink = io.StringIO()
        summary = replay(io.BytesIO(data), sink, chunk)
        return summary, [json.loads(line) for line in sink.getvalue().splitlines()]

    def test_stopped_car_and_imu_dropout(self):
        data = boot_packet(42) + pair(0, 10000)
        data += b''.join(pair(i, (i+1)*10000, flags=2, imu=10000)
                         for i in range(1, 101))
        summary, events = self.run_capture(data, 1)
        self.assertEqual(summary['events'], {'imu': 1, 'wheel': 101})
        self.assertTrue(all(e['received_monotonic_us'] is None for e in events))
        self.assertTrue(all(e['speed'] == 0 for e in events if e['sensor'] == 'wheel'))
        self.assertEqual(summary['crc_errors'], 0)

    def test_crc_recovery_and_truncated_tail(self):
        bad = bytearray(pair(0, 10000)); bad[20] ^= 1
        summary, events = self.run_capture(boot_packet(42)+bad+pair(1, 20000)+b'\xa5')
        self.assertEqual(summary['crc_errors'], 1)
        self.assertEqual(summary['trailing_bytes'], 1)
        self.assertEqual(summary['rejected_pairs'], 1)
        self.assertEqual(len(events), 2)

    def test_boot_required_and_reset_restarts_sample_clock(self):
        summary, events = self.run_capture(pair(0, 10000))
        self.assertFalse(summary['boot_observed'])
        self.assertEqual(events, [])
        summary, events = self.run_capture(boot_packet(42)+pair(0, 90000)+
                                          boot_packet(43)+pair(0, 10000))
        self.assertEqual([e['boot'] for e in events], [42,42,43,43])

    def test_chunking_does_not_change_observations(self):
        data = boot_packet(42)+pair(0,10000)+pair(1,20000)
        self.assertEqual(self.run_capture(data,1), self.run_capture(data,4096))
        with self.assertRaises(ValueError):
            replay(io.BytesIO(data), io.StringIO(), 0)


if __name__ == '__main__':
    unittest.main()
