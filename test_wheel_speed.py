import unittest

from protocol import Decoder, frame, WHEEL_DETAIL
from wheel_speed import WheelSpeed


class WheelSpeedTests(unittest.TestCase):
    def setUp(self):
        self.w=WheelSpeed()
        self.w.feed(dict(type='status',boot=7))

    def pair(self,t,present=2,source=None,speed=1.5,flags=2,seq=1):
        self.w.feed(dict(type='data',flags=flags,seq=seq,wheel_speed=speed))
        e=dict(type='timing',seq=seq,report_us=t,present=present,wheel_us=t if source is None else source)
        self.w.feed(e)
        return e['wheel_speed_output']

    def detail(self,report=10000,seq=1,boot=7,flags=1,start=7500,end=9700,speed=-.25):
        return frame(WHEEL_DETAIL.pack(4,1,boot,seq,report,flags,0,start,end,speed,2,4,1))

    def test_held_expires_at_contract_limit(self):
        self.assertEqual(self.pair(10000)['state'],'measured')
        held=self.pair(59999,present=0)
        self.assertEqual((held['state'],held['age_us'],held['source_us'],held['wheel_valid']),('held',49999,10000,False))
        stale=self.pair(60000,present=0)
        self.assertEqual((stale['speed_mps'],stale['last_known_mps'],stale['state']),(None,1.5,'unavailable'))

    def test_duplicate_zero_preserves_original_stamp(self):
        self.assertEqual(self.pair(10000,speed=0)['speed_mps'],0)
        self.assertEqual(self.pair(20000,source=10000,speed=0)['state'],'held')
        self.assertFalse(self.pair(27000,source=10000,speed=0)['wheel_valid'])

    def test_detail_pairing_and_reset(self):
        for raw in (self.detail(seq=3),self.detail(boot=8),self.detail(report=10100)):
            self.w.feed(Decoder().feed(raw)[0])
            self.assertIsNone(self.pair(10000,present=0)['speed_mps'])
        self.w.feed(Decoder().feed(self.detail())[0])
        q=self.pair(10000,present=0)
        self.assertEqual((q['state'],q['speed_mps'],q['source_us'],q['wheel_valid']),('short_window',-.25,9700,False))
        self.assertIsNone(self.pair(5000,present=0)['last_known_mps'])
        self.w.feed(dict(type='status',boot=8))
        self.assertIsNone(self.pair(20000,present=0)['speed_mps'])

    def test_hard_sensor_fault_clears_history(self):
        self.pair(5000)
        self.w.feed(Decoder().feed(self.detail(flags=2,start=0,end=0,speed=0))[0])
        self.assertIsNone(self.pair(10000)['last_known_mps'])

    def test_bad_crc_timing_and_flags_rejected(self):
        bad=bytearray(self.detail()); bad[-1]^=1
        for b in (bad,self.detail(flags=3),self.detail(end=12000),self.detail(start=9600),self.detail(speed=float('nan'))):
            self.assertEqual(Decoder().feed(b),[])
