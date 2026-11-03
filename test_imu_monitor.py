import unittest
from imu_monitor import FIELDS,parse_line,summarize

def row(seq=0,now=1000000,stamp=None,valid=1):
    r={k:0 for k in FIELDS}; r.update(seq=seq,uptime_us=now,sample_us=stamp if stamp is not None else now-500,state=4,present=1,valid=valid,az=9.80665)
    return r

class MonitorTests(unittest.TestCase):
    def test_parser(self):
        r=row(); self.assertEqual(parse_line(','.join(str(r[k]) for k in FIELDS)),r)
        self.assertIsNone(parse_line('boot text'))
        r['gx']=float('nan'); self.assertIsNone(parse_line(','.join(str(r[k]) for k in FIELDS)))
        r=row(); r['sample_us']=r['uptime_us']+1
        self.assertIsNone(parse_line(','.join(str(r[k]) for k in FIELDS)))
    def test_summary(self):
        s=summarize([row(0),row(1,1010000),row(3,1030000)])
        self.assertEqual(s['missing_records'],1)
        self.assertEqual(s['device_report_spacing_ms']['max'],20)
        self.assertAlmostEqual(s['acceleration_norm_mean'],9.80665)
    def test_reset_and_wrap(self):
        s=summarize([row(4294967295),row(0,1010000),row(0,1000)])
        self.assertEqual(s['observed_resets'],1); self.assertEqual(s['missing_records'],0)
    def test_invalid_and_held(self):
        a=row(); b=row(1,1010000,stamp=a['sample_us']); c=row(2,1020000,valid=0)
        s=summarize([a,b,c]); self.assertEqual(s['usable_unique_samples'],1)
        self.assertEqual(s['held_sample_rows'],1); self.assertEqual(s['invalid_rows'],1)
        self.assertIsNone(summarize([])['acceleration_norm_mean'])

if __name__=='__main__': unittest.main()
