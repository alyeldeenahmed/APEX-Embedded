"""ESP32 commissioning viewer. Receipts run at the wire rate, display is slow."""
import argparse
import json
import math
import time
from pathlib import Path
from car_link import CarLink

class View:
    def __init__(self):
        self.pending=None
        self.pair=None
        self.received=0
        self.boot=None
        self.gaps=0
        self.pairs=0
        self.link_status=None
    def feed(self, event):
        if event['type']=='status':
            if self.boot is not None and self.boot!=event['boot']:
                self.pending=self.pair=None
            self.boot=event['boot']
            self.link_status=event
        elif event['type']=='data':
            if event['flags'] & 64:
                self.pair=None
            self.pending=event
        elif event['type']=='timing':
            d,self.pending=self.pending,None
            if d is None or d['seq']!=event['seq']:return
            self.pair=(d,event)
            self.received=time.monotonic()
            self.pairs+=1
            if not event['present'] & 2:self.gaps+=1
    def text(self, now=None):
        now=time.monotonic() if now is None else now
        if self.pair is None:return 'Waiting for a matched data/timing pair...'
        if now-self.received>.2:return 'LINK STALE: no recent pair. Previous sensor values hidden.'
        d,t=self.pair
        report=t['report_us']
        def fresh(bit,stamp,limit):
            return bool(t['present'] & bit) and 0<stamp<=report and report-stamp<=limit
        wheel=fresh(2,t['wheel_us'],25000)
        imu=fresh(1,t['imu_us'],50000)
        lines=[f"Sequence {d['seq']} | pairs {self.pairs} | encoder gap records {self.gaps}"]
        if self.link_status and not self.link_status.get('receipt_watchdog'):
            lines.append('Old ESP firmware: upload the receipt-watchdog integration sketch.')
        elif self.link_status is not None and 'unacked_pairs' in self.link_status:
            status=self.link_status
            state='LOST' if status['link_lost'] else 'clear'
            lines.append(f"Last ESP status: receipt fault {state} | outstanding {status['unacked_pairs']}/4 | armed {'YES' if status['flags'] & 2 else 'NO'}")
        if wheel:
            v=d['wheel_speed']
            lines.append(f"SPEED: {v:+.3f} m/s  ({v*3.6:+.2f} km/h) | wheel valid={'YES' if d['flags']&2 else 'NO: confirmation pending'}")
        else:lines.append('SPEED: UNAVAILABLE - encoder interval missing/stale (do not interpret as stopped)')
        if imu:
            lines.append('ACCEL m/s2: X={:+.3f} Y={:+.3f} Z={:+.3f}'.format(*d['accel']))
            lines.append('GYRO rad/s: X={:+.3f} Y={:+.3f} Z={:+.3f}'.format(*d['gyro']))
            lines.append('IMU valid: '+('YES' if d['flags']&1 else 'NO: confirmation/freshness gate; inspect raw readings'))
        else:lines.append('IMU: UNAVAILABLE - no recent source sample')
        return '\n'.join(lines)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--port',default='COM3')
    ap.add_argument('--display-hz',type=float,default=1)
    ap.add_argument('--record',type=Path,help='Optional new full-rate decoded JSONL file')
    a=ap.parse_args()
    if not math.isfinite(a.display_hz) or not .2<=a.display_hz<=5:ap.error('Display rate must be 0.2 to 5 Hz')
    import serial
    capture=None
    try:
        if a.record:capture=a.record.open('x',encoding='utf-8')
        view=View()
        def event(e):
            view.feed(e)
            if capture:capture.write(json.dumps(e)+'\n')
        with serial.Serial(a.port,921600,timeout=.003,write_timeout=.04,exclusive=True) as port:
            car=CarLink(port,on_event=event)
            print('APEX slow viewer - acknowledges telemetry. Sends no actuator commands. Ctrl+C to stop.')
            print('Close Arduino Serial Monitor and other recorders before using this viewer.')
            due=time.monotonic()
            while True:
                car.poll()
                now=time.monotonic()
                if now>=due:
                    print(f'\nPi/host receipts sent: {car.receipts_sent}\n'+view.text(now),flush=True)
                    due=now+1/a.display_hz
    except KeyboardInterrupt:print('\nViewer stopped.')
    except (OSError,serial.SerialException) as e:
        print('Cannot run viewer:',e)
        return 1
    finally:
        if capture:capture.close()
    return 0

if __name__=='__main__':raise SystemExit(main())
