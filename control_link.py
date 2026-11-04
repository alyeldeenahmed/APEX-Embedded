"""Proposed Control protocol codec and explicit, one-shot serial bench client.

No automatic arming, no periodic command generation and no ROS dependency.
Perception/Control's Pi integration must have a single owner of the serial port.
"""
import argparse
import math
import struct
import time
import json
from dataclasses import dataclass


def crc(data):
    value = 0xffff
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ (0x1021 if value & 0x8000 else 0)) & 0xffff
    return value


ACTIONS = dict(install=1, arm=2, set=3, cancel=4, revoke=5)
CODES = ('idle installed accepted applied simulated cancelled revoked bad_boot replay '
         'bad_context bad_time bad_value not_ready not_armed unknown_target fault expired').split()


def encode(*, action, boot, sequence, session, epoch, command_id, created_us,
           deadline_us, target=0, steering=0.0, acceleration=0.0):
    if action not in ACTIONS:
        raise ValueError('Unknown action')
    for value in (boot, sequence, session, epoch, command_id, target):
        if type(value) is not int or not 0 <= value <= 0xffffffff:
            raise ValueError('Identifiers must be uint32')
    if not command_id or not all(math.isfinite(v) for v in (steering, acceleration)):
        raise ValueError('Nonzero command ID and finite values required')
    if action != 'set' and (steering or acceleration):
        raise ValueError('Only SET may carry nonzero actuator values')
    if not (type(created_us) is int and type(deadline_us) is int and
            0 <= created_us < deadline_us <= 0xffffffffffffffff):
        raise ValueError('Invalid MCU timestamps')
    p = bytearray(64)
    struct.pack_into('<BBBBB', p, 0, 0xa5, 0x5a, 0x10, 1, ACTIONS[action])
    struct.pack_into('<6I2Q2f', p, 8, boot, sequence, session, epoch, command_id,
                     target, created_us, deadline_us, steering, acceleration)
    struct.pack_into('<H', p, 62, crc(p[2:62]))
    return bytes(p)


def status(p):
    if len(p) != 64 or p[:4] != b'\xa5\x5a\x11\x01' or crc(p[2:62]) != struct.unpack_from('<H', p, 62)[0]:
        raise ValueError('Invalid status frame')
    if p[4] >= len(CODES) or p[5] & 0xf0 or p[6] or p[7]:
        raise ValueError('Unsupported status')
    values = struct.unpack_from('<6I2Q2fHI', p, 8)
    keys = ('boot', 'sequence', 'session', 'epoch', 'command_id', 'held_id',
            'now_us', 'applied_us', 'steering', 'effort', 'parser_errors', 'tx_drops')
    result = dict(zip(keys, values))
    result.update(code=CODES[p[4]], flags=p[5])
    if not all(math.isfinite(result[k]) for k in ('steering', 'effort')):
        raise ValueError('Nonfinite output')
    return result


class Stream:
    """Demultiplex telemetry, sync and control status without stealing payload bytes."""
    lengths = {1: 54, 2: 14, 0x11: 64}

    def __init__(self):
        self.buffer = bytearray()
        self.errors = 0

    def feed(self, data):
        self.buffer.extend(data)
        frames = []
        while len(self.buffer) >= 4:
            n = self.lengths.get(self.buffer[2])
            if self.buffer[:2] != b'\xa5\x5a' or not n or self.buffer[3] != 1:
                del self.buffer[0]
                self.errors += 1
                continue
            if len(self.buffer) < n:
                break
            p = bytes(self.buffer[:n])
            if crc(p[2:-2]) != struct.unpack_from('<H', p, n-2)[0]:
                del self.buffer[0]
                self.errors += 1
                continue
            frames.append(p)
            del self.buffer[:n]
        return frames


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--port', required=True)
    ap.add_argument('--seconds', type=float, default=5)
    ap.add_argument('--action', choices=ACTIONS)  # omitted = read-only monitor
    ap.add_argument('--session', type=int, default=1)
    ap.add_argument('--epoch', type=int, default=1)
    ap.add_argument('--id', type=int, default=1)
    ap.add_argument('--target', type=int, default=0)
    ap.add_argument('--steering', type=float, default=0)
    ap.add_argument('--acceleration', type=float, default=0)
    args = ap.parse_args()
    if not math.isfinite(args.seconds) or args.seconds <= 0:
        ap.error('--seconds must be positive and finite')
    import serial
    parser = Stream()
    sent = False
    sent_boot = None
    end = time.monotonic() + args.seconds
    # Opening the port may reset this board. Wait for its own boot status.
    with serial.Serial(args.port, 921600, timeout=.01, write_timeout=.1) as port:
        while time.monotonic() < end:
            for frame in parser.feed(port.read(512)):
                if frame[2] != 0x11:
                    continue
                s = status(frame)
                print(json.dumps(s), flush=True)
                if sent and s['boot'] != sent_boot:
                    raise SystemExit('MCU restarted; command outcome unknown. No retry.')
                if args.action and not sent:
                    # Use observed MCU time conservatively, without adding estimated
                    # transit delay. Queuing may cause rejection, never extend authority.
                    packet = encode(action=args.action, boot=s['boot'],
                                    sequence=(s['sequence']+1)&0xffffffff,
                                    session=args.session, epoch=args.epoch, command_id=args.id,
                                    target=args.target, steering=args.steering,
                                    acceleration=args.acceleration, created_us=s['now_us'],
                                    deadline_us=s['now_us']+80000)
                    port.write(packet)
                    sent = True
                    sent_boot = s['boot']
                elif sent and s['command_id'] == args.id:
                    return
    if args.action:
        raise SystemExit('No matching acknowledgement; outcome unknown. No retry.')


if __name__ == '__main__':
    main()
