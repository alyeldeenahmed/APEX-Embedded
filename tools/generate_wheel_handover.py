"""Generate SIMULATED v2 decoder examples, never hardware evidence."""
import io,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from protocol import data_packet,timing_packet,sync_packet,frame
from tools.replay_capture import replay

def generate():
    capture=bytearray()
    for i in range(100):
        t=2**32+10000*(i+1)
        if i%10==0:
            body=bytearray(60);body[:2]=bytes([0x11,1])
            struct.pack_into('<I',body,6,42);struct.pack_into('<Q',body,30,t)
            capture+=frame(body)
        if i==0: capture+=sync_packet(t)
        suspect=i>=80
        speed=0 if i<40 or suspect else .5
        seq=(65520+i)&0xffff
        capture+=data_packet(seq,t-100,flags=(1 if suspect else 3)|(64 if i==0 else 0),speed=speed, wheel_age=200,steer_age=65535)
        capture+=timing_packet(seq,t,t-100,t-200,0,3,6 if suspect else 4,2)
    sink=io.StringIO();summary=replay(io.BytesIO(capture),sink,7)
    assert summary['events']=={'wheel':80,'imu':100},summary
    assert not summary['crc_errors'] and not summary['header_errors']
    return bytes(capture),sink.getvalue(),summary

if __name__=='__main__':
    out=ROOT/'handover/wheel-v2-simulated';out.mkdir(parents=True,exist_ok=True)
    capture,events,summary=generate()
    (out/'SIMULATED_capture.bin').write_bytes(capture)
    (out/'expected_events.jsonl').write_text(events)
    (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    (out/'README.md').write_text('# SIMULATED wheel-v2 decoder handover\n\nNot a real ESP32 capture. No physical timing, bias, noise, sensor health or axes\nare demonstrated. Peer approval of timing version 2 is still required.\n\n100 data/timing pairs at synthetic 100 Hz: 40 valid unsigned zeros, 40 valid\nunsigned moving values, 20 suspect/invalid wheels with continuing valid IMU.\nSequence wraps 65535->0; 64-bit time exceeds 2^32. Status boot=42 every 100 ms.\nExpected 80 wheel and 100 IMU events, no CRC/header errors. Gyro/accel values are\nsynthetic. Host receipt time is null; do not use this file to measure serial jitter.\nRun tools/generate_wheel_handover.py to reproduce. Exact byte/bit proposal:\ndocs/PERCEPTION_DECISIONS_2026_11_09.md.\n')
    print(json.dumps(summary,indent=2))
