"""Offline raw-event handover; never opens a port or sends commands.

Binary captures lack receipt timestamps: emit null, never invented host timing.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from protocol import Decoder
from sensor_events import SensorEvents


def replay(source, sink, chunk_size=4096):
    if chunk_size <= 0:
        raise ValueError("chunk_size must be positive")
    decoder, adapter = Decoder(), SensorEvents()
    records, events = Counter(), Counter()
    while True:
        chunk = source.read(chunk_size)
        if not chunk:
            break
        for record in decoder.feed(chunk):
            records[record['type']] += 1
            for event in adapter.feed(record, None):
                events[event['sensor']] += 1
                sink.write(json.dumps(event, allow_nan=False) + "\n")
    return dict(records=dict(records), events=dict(events),
                crc_errors=decoder.crc_errors, header_errors=decoder.header_errors,
                trailing_bytes=len(decoder.buffer), rejected_pairs=adapter.rejected,
                pending_pair=adapter.pending is not None,
                boot_observed=adapter.boot is not None,
                receipt_timing_available=False,
                scope="offline raw events; no clock mapping, trust or hardware validation")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('events', type=Path)
    args = parser.parse_args()
    if args.capture.resolve() == args.events.resolve():
        parser.error('Input and output must differ')
    # Exclusive creation protects existing evidence from accidental overwrite.
    with args.capture.open('rb') as source, args.events.open('x', encoding='utf-8') as sink:
        summary = replay(source, sink)
    print(json.dumps(summary, indent=2))
    return int(not summary['boot_observed'] or not summary['events'] or
               any(summary[k] for k in ('crc_errors', 'header_errors',
                   'trailing_bytes', 'rejected_pairs', 'pending_pair')))


if __name__ == '__main__':
    raise SystemExit(main())
