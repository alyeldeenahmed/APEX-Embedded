import unittest
from protocol import Decoder,data_packet,timing_packet,frame
from sensor_events import SensorEvents

class WheelDiagnosticsTests(unittest.TestCase):
    def events(self,diag,speed=0):
        adapter=SensorEvents()
        adapter.feed(dict(type='status',boot=7),0)
        wire=data_packet(1,0,flags=2,speed=speed)+timing_packet(1,10000,0,9990,0,2,diag,2)
        result=[]
        for record in Decoder().feed(wire): result+=adapter.feed(record,11000)
        return result
    def test_unsigned_valid_zero(self):
        e=self.events(4)[0]
        self.assertEqual(e['speed'],0)
        self.assertFalse(e['direction_measured'])
        self.assertTrue(e['motion_monitor_enabled'])
        self.assertEqual(e['sample_us'],9990)
    def test_fault_cannot_publish_even_if_valid_bit_inconsistent(self):
        self.assertEqual(self.events(6),[])
    def test_unsigned_negative_rejected_signed_reverse_allowed(self):
        self.assertEqual(self.events(0,-1),[])
        self.assertEqual(self.events(1,-1)[0]['speed'],-1)
    def test_v1_reserved_and_v2_unknown_bits_rejected(self):
        for version,diag in [(1,1),(2,8),(3,0)]:
            with self.assertRaises(ValueError): timing_packet(1,10,0,9,0,2,diag,version)
        original=timing_packet(1,10,0,9,0,2,0,2)
        body=bytearray(original[2:-2]);body[5]=8
        d=Decoder();self.assertEqual(d.feed(frame(body)),[]);self.assertEqual(d.header_errors,1)
    def test_backwards_report_requires_boot_status(self):
        a=SensorEvents();a.feed(dict(type='status',boot=7),0)
        def pair(t):
            out=[]
            for r in Decoder().feed(data_packet(1,0,flags=2)+timing_packet(1,t,0,t,0,2,4,2)):
                out+=a.feed(r,None)
            return out
        self.assertTrue(pair(100))
        self.assertEqual(pair(50),[])
        self.assertEqual(pair(60),[])
        a.feed(dict(type='status',boot=8),0)
        self.assertTrue(pair(70))
    def test_orphan_overwrite_counted(self):
        a=SensorEvents();a.feed(dict(type='status',boot=7),0)
        a.feed(dict(type='data'),None);a.feed(dict(type='data'),None)
        self.assertEqual(a.rejected,1)
    def test_generated_handover(self):
        from tools.generate_wheel_handover import generate
        _,_,summary=generate()
        self.assertEqual(summary['events'],{'wheel':80,'imu':100})
    def test_control_demultiplexer_preserves_v2(self):
        from control_link import Stream
        raw=timing_packet(1,10,0,9,0,2,4,2)
        stream=Stream();out=[]
        for byte in raw: out+=stream.feed(bytes([byte]))
        self.assertEqual(out,[raw]);self.assertEqual(stream.errors,0)
    def test_fragmented_mixed_versions(self):
        raw=timing_packet(1,10,0,9,0,2)+timing_packet(2,20,0,19,0,2,4,2)
        d=Decoder();out=[]
        for byte in raw: out+=d.feed(bytes([byte]))
        self.assertEqual([r['version'] for r in out],[1,2])
        self.assertEqual([r['diagnostics'] for r in out],[0,4])
