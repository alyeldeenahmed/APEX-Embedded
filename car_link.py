"""Single-owner Pi USB interface for the TT-02. All command times use MCU status."""
import argparse
import json
import math
import time
from pathlib import Path

from control_link import encode
from protocol import Decoder, TelemetryReceipts


class MotorTelemetry:
    """Pair the existing count and its MCU acquisition timestamp for bench RPM."""
    def __init__(self):
        self.sample = None
        self.previous = None
        self.boot = None
        self.report = None

    def feed(self, event):
        if event['type'] == 'status':
            if self.boot is not None and event['boot'] != self.boot:
                self.sample = self.previous = self.report = None
            self.boot = event['boot']
        if event['type'] == 'timing' and 'report_us' in event:
            if self.report is not None and event['report_us'] <= self.report:
                self.sample = self.previous = None
            self.report = event['report_us']
        if event['type'] == 'data':
            self.sample = event
            if event['flags'] & 64:
                self.previous = None
        if event['type'] != 'timing':
            return
        s, self.sample = self.sample, None
        if not s or s['seq'] != event['seq'] or not event['present'] & 2:
            self.previous = None
            return
        seq, ticks, stamp = s['seq'], s['enc_ticks'], event['wheel_us']
        if self.previous:
            last_seq, last_ticks, last_stamp = self.previous
            if seq == (last_seq+1) % 65536 and 0 < stamp-last_stamp <= 25000:
                delta = (ticks-last_ticks+2**31) % 2**32-2**31
                event['motor_rpm'] = delta*60_000_000/(16384*(stamp-last_stamp))
        self.previous = seq, ticks, stamp


class CarLink:
    def __init__(self, port, on_event=None):
        self.port = port
        self.decoder = Decoder()
        self.receipts = TelemetryReceipts()
        self.receipts_sent = 0
        self.last_receipt_write = None
        self.motor = MotorTelemetry()
        self.on_event = on_event
        self.last = None
        self.boot = None
        self.session = None
        self.sequence = 0
        self.last_received = None

    def receipts_current(self):
        return (self.last_receipt_write is not None and
                self.receipts.last_report_us is not None and self.last is not None and
                self.receipts.boot == self.last['boot'] and
                time.monotonic() - self.last_receipt_write < .04)

    def _write(self, packet):
        try:
            if self.port.write(packet) != len(packet):
                raise OSError('Incomplete USB write')
        except OSError:
            self.session = None
            raise

    def poll(self):
        try:
            events = self.decoder.feed(self.port.read(min(4096, self.port.in_waiting or 1)))
        except OSError:
            self.session = None
            raise
        session_error = None
        for event in events:
            receipt = self.receipts.feed(event)
            if receipt is not None:
                self._write(receipt)
                self.receipts_sent += 1
                self.last_receipt_write = time.monotonic()
            self.motor.feed(event)
            if event['type'] == 'status':
                if self.session is not None:
                    if event['boot'] != self.boot:
                        session_error = 'ESP restarted during session'
                    elif event['link_lost']:
                        session_error = 'Pi receipts lost; start a new explicit session after recovery'
                    if session_error:
                        self.session = None
                self.last = event
                self.last_received = time.monotonic()
        if self.on_event:
            for event in events:
                self.on_event(event)
        if session_error:
            raise RuntimeError(session_error)
        return events

    def request(self, action, expected, steering=0., acceleration=0.):
        if self.session is None or self.last is None:
            raise RuntimeError('Install and arm an explicit session first')
        if self.last_received is None or time.monotonic()-self.last_received > .05:
            raise RuntimeError('Status is too old for a command; poll fresh status first')
        self.sequence = (self.sequence + 1) & 0xffffffff
        ident = self.sequence or 0xffffffff
        stamp = self.last['now_us']
        packet = encode(action=action, sequence=self.sequence, boot=self.boot,
                        session=self.session, epoch=1, command_id=ident,
                        created_us=stamp, deadline_us=stamp+80000,
                        steering=steering, acceleration=acceleration)
        self._write(packet)
        end = time.monotonic()+.08
        while time.monotonic() < end:
            for e in self.poll():
                if (e['type'] == 'status' and e['sequence'] == self.sequence and
                        e['command_id'] == ident and e['session'] == self.session and
                        e['epoch'] == 1):
                    if e['code'] != expected:
                        raise RuntimeError(f"{action} returned {e['code']}")
                    return
        self.session = None
        raise TimeoutError(f'{action} acknowledgement timed out; command outcome unknown')

    def arm(self):
        end = time.monotonic()+4
        while time.monotonic() < end:
            if self.last_received is not None and time.monotonic()-self.last_received < .05:
                if not self.last['receipt_watchdog']:
                    raise RuntimeError('Upload the paired ESP32 receipt-watchdog firmware first.')
                if not self.last['flags'] & 4:
                    raise RuntimeError('Physical PWM disabled. Check commissioning settings.')
                if self.last['flags'] & 2:
                    raise RuntimeError('Another session is armed')
                if not self.last['link_lost'] and self.receipts_current():
                    break
            self.poll()
        else:
            raise TimeoutError('No fresh vehicle status with a functioning telemetry receipt link')
        self.boot = self.last['boot']
        self.sequence = self.last['sequence']
        self.session = (self.last['session']+1) & 0xffffffff or 1
        self.request('install', 'installed')
        self.request('arm', 'applied')

    def set(self, steering_rad, accel_mps2):
        self.request('set', 'applied', steering_rad, accel_mps2)

    def stop(self):
        if self.session is not None:
            try:
                self.request('revoke', 'revoked')
            finally:
                self.session = None


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--port', required=True)
    commands = ap.add_subparsers(dest='mode', required=True)
    monitor = commands.add_parser('monitor', help='Acknowledge telemetry and record it; never arms')
    monitor.add_argument('--seconds', type=float, default=0, help='0 runs until Ctrl+C')
    monitor.add_argument('--output', type=Path, help='New JSONL capture file; never overwrites')
    monitor.add_argument('--quiet', action='store_true', help='Skip printing individual JSON records')
    bench = commands.add_parser('bench', help='Explicit short actuator test on stands')
    bench.add_argument('--steering-rad', type=float, default=0.)
    bench.add_argument('--accel-mps2', type=float, required=True)
    bench.add_argument('--seconds', type=float, required=True)
    args = ap.parse_args()
    if args.mode == 'monitor' and (not math.isfinite(args.seconds) or args.seconds < 0):
        ap.error('Monitor seconds must be finite and nonnegative')
    if args.mode == 'bench' and not (0 < args.seconds <= 2 and
            all(map(math.isfinite, [args.steering_rad, args.accel_mps2]))):
        ap.error('Use finite commands and a duration up to 2 seconds')
    import serial
    with serial.Serial(args.port, 921600, timeout=.003, write_timeout=.04, exclusive=True) as port:
        car = CarLink(port)
        if args.mode == 'monitor':
            capture = args.output.open('x', encoding='utf-8') if args.output else None
            end = time.monotonic()+args.seconds if args.seconds else math.inf
            try:
                while time.monotonic() < end:
                    for event in car.poll():
                        line = json.dumps(event) if capture or not args.quiet else None
                        if not args.quiet:
                            print(line)
                        if capture:
                            capture.write(line+'\n')
            except KeyboardInterrupt:
                pass
            finally:
                if capture:
                    capture.close()
            return
        try:
            car.arm()
            end = time.monotonic()+args.seconds
            next_set = time.monotonic()
            while time.monotonic()<end:
                if time.monotonic() >= next_set:
                    car.set(args.steering_rad, args.accel_mps2)
                    next_set = time.monotonic()+.02
                else:
                    car.poll()
        finally:
            car.stop()


if __name__ == '__main__':
    main()

