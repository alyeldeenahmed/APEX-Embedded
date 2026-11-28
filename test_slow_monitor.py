import unittest
from slow_monitor import View
from wheel_speed import WheelSpeed

class SlowMonitorTests(unittest.TestCase):
    def pair(self,view,present=3):
        speed=WheelSpeed()
        for e in (dict(type='status',boot=7),dict(type='data',seq=1,flags=0,wheel_speed=1,gyro=[0,0,0],accel=[0,0,9.8]),
                  dict(type='timing',seq=1,present=present,report_us=10000,imu_us=9500,wheel_us=9500)):
            speed.feed(e)
            view.feed(e)
    def test_unconfirmed_speed_is_labelled(self):
        v=View();self.pair(v)
        text=v.text(v.received)
        self.assertIn('+1.000 m/s',text)
        self.assertIn('confirmation pending',text)
        self.assertIn('Z=+9.800',text)
    def test_gap_does_not_display_valid_zero(self):
        v=View();self.pair(v,1)
        text=v.text(v.received)
        self.assertIn('SPEED: UNAVAILABLE',text)
        self.assertEqual(v.gaps,1)
    def test_stale_link_hides_old_readings(self):
        v=View();self.pair(v)
        text=v.text(v.received+1)
        self.assertIn('LINK STALE',text)
        self.assertNotIn('+1.000',text)
    def test_mismatched_pair_is_not_displayed(self):
        v=View();v.feed(dict(type='data',seq=1,flags=0))
        v.feed(dict(type='timing',seq=2,present=3))
        self.assertIsNone(v.pair)
