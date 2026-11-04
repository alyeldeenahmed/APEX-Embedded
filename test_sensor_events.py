import unittest
from protocol import Decoder, data_packet, timing_packet
from sensor_events import SensorEvents


class SensorEventTests(unittest.TestCase):
    def setUp(self):
        self.events = SensorEvents()
        self.events.feed(dict(type='status', boot=42), 0)

    def pair(self, seq, report, imu, wheel, flags=3, speed=0, present=3):
        records=Decoder().feed(data_packet(seq, imu, flags=flags, speed=speed)+
            timing_packet(seq, report, imu, wheel, 0, present))
        out=[]
        for r in records: out += self.events.feed(r, report+500)
        return out

    def test_dropout_keeps_wheels_and_zero_speed(self):
        self.assertEqual(len(self.pair(0, 10000, 9900, 9800)), 2)
        for i in range(1, 101):
            out=self.pair(i, 10000+i*10000, 9900, 9800+i*10000, flags=2)
            self.assertEqual([e['sensor'] for e in out], ['wheel'])
            self.assertEqual(out[0]['speed'], 0)
        self.assertEqual(self.events.last['imu'], 9900)

    def test_duplicate_does_not_count_as_recovery(self):
        self.pair(0, 10000, 9900, 9800)
        out=self.pair(1, 15000, 14900, 9800)
        self.assertEqual([e['sensor'] for e in out], ['imu'])
        out=self.pair(2, 20000, 14900, 19800)
        self.assertEqual([e['sensor'] for e in out], ['wheel'])

    def test_rollover_and_restart(self):
        t=2**32
        out=self.pair(65535, t+100, t+90, t+80)
        self.assertEqual(out[0]['sample_us'], t+80)
        self.assertTrue(self.pair(0, t+200, t+190, t+180))
        self.events.feed(dict(type='status',boot=43),t+300)
        self.assertTrue(self.pair(0,100,90,80))

    def test_missing_pair_and_backlog_provenance(self):
        self.assertEqual(self.events.feed(dict(type='timing',seq=3), 0), [])
        out=self.pair(1,1000,900,800)
        self.assertEqual(out[0]['received_monotonic_us'],1500)
        self.assertEqual(out[0]['sample_us'],800)
        self.assertEqual(self.pair(2,900,890,880), [])

    def test_invalid_future_and_missing_sensor(self):
        self.assertEqual(self.pair(0,1000,1001,900), [])
        self.assertEqual(self.pair(1,2000,1900,1800,present=2), [])
        self.assertEqual(self.pair(2,3000,2900,2800,flags=0), [])

    def test_no_boot_no_observations(self):
        self.events=SensorEvents()
        self.assertEqual(self.pair(0,100,90,80), [])


if __name__ == '__main__': unittest.main()
