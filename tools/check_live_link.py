"""Test the receipt watchdog on a sensor-only ESP32 with the USB port kept open."""
import argparse
from pathlib import Path
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from protocol import Decoder, TelemetryReceipts


def check(port, timeout=8):
    decoder, receipts = Decoder(), TelemetryReceipts()
    phase = 'establish'
    boot = None
    acknowledged = withheld = resumed = 0
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        for event in decoder.feed(port.read(min(2048, port.in_waiting or 1))):
            if event['type'] == 'status':
                if not event['receipt_watchdog']:
                    raise RuntimeError('Upload the paired receipt-watchdog firmware first.')
                if event['flags'] & 15:
                    raise RuntimeError('Use the default sensor-only sketch with no installed session or PWM.')
                if boot is not None and event['boot'] != boot:
                    raise RuntimeError('ESP restarted during the check. Repeat on one uninterrupted boot.')
                boot = event['boot']
                if phase == 'establish' and acknowledged >= 5 and not event['link_lost']:
                    phase = 'withhold'
                elif phase == 'withhold' and event['link_lost']:
                    if event['unacked_pairs'] != 4 or withheld < 4:
                        raise RuntimeError('Link lost without four observed withheld pairs. Inspect USB timing.')
                    phase = 'restore'
                elif phase == 'restore' and resumed and not event['link_lost']:
                    return (f'PASS: boot {boot}, ESP reported 4 outstanding pairs, '
                            f'receipts restored, outputs disarmed. Withheld {withheld} Pi replies.')
            reply = receipts.feed(event)
            if reply is None:
                continue
            if phase == 'withhold':
                withheld += 1
            else:
                if port.write(reply) != len(reply):
                    raise OSError('Incomplete USB receipt write')
                acknowledged += 1
                if phase == 'restore':
                    resumed += 1
    raise TimeoutError(f'No complete result during {phase}. Check port, firmware and single-owner access.')


def main():
    import serial
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True)
    args = parser.parse_args()
    try:
        with serial.Serial(args.port, 921600, timeout=.003, write_timeout=.04, exclusive=True) as port:
            print(check(port))
    except (OSError, RuntimeError) as error:
        parser.exit(1, f'Check incomplete: {error}\n')


if __name__ == '__main__':
    main()
