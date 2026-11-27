"""Exercise both ends of the receipt protocol over host pipes. No serial hardware."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from car_link import CarLink
from check_live_link import check


class TestPort:
    def __init__(self):
        self.incoming = bytearray()
        self.receipts = []

    @property
    def in_waiting(self):
        return len(self.incoming)

    def read(self, n):
        chunk = bytes(self.incoming[:n])
        del self.incoming[:n]
        return chunk

    def write(self, packet):
        self.receipts.append(bytes(packet))
        return len(packet)


def exercise(peer):
    port = TestPort()
    car = CarLink(port)

    def exchange(command):
        peer.stdin.write(command + '\n')
        peer.stdin.flush()
        reply = peer.stdout.readline()
        if not reply:
            raise RuntimeError(f'Firmware peer closed during {command}')
        return reply.strip()

    def deliver(data):
        # USB can split frames anywhere.
        for offset in range(0, len(data), 7):
            port.incoming.extend(data[offset:offset+7])
            car.poll()

    def pair():
        deliver(bytes.fromhex(exchange('pair')))
        assert len(port.receipts) == 1, 'Expected one Pi receipt for the complete pair'
        return port.receipts.pop()

    def send(receipt):
        assert exchange('rx ' + receipt.hex()) == 'ok'

    deliver(bytes.fromhex(exchange('status')))
    first = pair()
    assert exchange('state') == '1 0'
    send(first)
    assert exchange('state') == '0 0', 'ESP rejected the Pi receipt'

    # Lose the 65535 reply, then deliver the cumulative reply at sequence 0.
    pair()
    send(pair())
    assert exchange('state') == '0 0', 'Receipt window failed across sequence wrap'

    # A corrupt pair must receive no ACK. Replaying a once-valid ACK cannot help.
    corrupt = bytearray.fromhex(exchange('pair'))
    corrupt[-1] ^= 1
    deliver(corrupt)
    assert not port.receipts
    send(first)
    assert exchange('state') == '1 0'
    pair()
    pair()
    assert exchange('state') == '3 0'
    newest = pair()
    assert exchange('state') == '4 1', 'Fourth unacknowledged pair did not trip'
    deliver(bytes.fromhex(exchange('status')))
    assert car.last['link_lost'] and car.last['code']=='link_lost' and car.last['unacked_pairs']==4
    send(newest)
    # Relinking proves a current receipt was accepted; actuation needs explicit ARM.
    assert exchange('state') == '0 0'
    deliver(bytes.fromhex(exchange('status')))
    assert not car.last['link_lost'] and not car.last['flags'] & 11

    # Verify the same commissioning check shipped for COM3 and the Pi.
    class PeerPort(TestPort):
        def __init__(self):
            super().__init__()
            self.frames = 0
            self.incoming.extend(bytes.fromhex(exchange('status')))

        @property
        def in_waiting(self):
            if not self.incoming:
                self.frames += 1
                self.incoming.extend(bytes.fromhex(exchange('pair')))
                if self.frames % 10 == 0 or exchange('state') == '4 1':
                    self.incoming.extend(bytes.fromhex(exchange('status')))
            return len(self.incoming)

        def write(self, packet):
            send(packet)
            return len(packet)

    assert check(PeerPort()).startswith('PASS:')


def main():
    with tempfile.TemporaryDirectory() as temp:
        exe = Path(temp) / ('pi_receipt_peer.exe' if os.name == 'nt' else 'pi_receipt_peer')
        subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++11', '-Wall', '-Wextra',
                        '-Werror', '-I.', 'tests/pi_receipt_peer.cpp', '-o', str(exe)],
                       cwd=ROOT, check=True)
        with subprocess.Popen([str(exe)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              text=True) as peer:
            try:
                exercise(peer)
            finally:
                peer.stdin.close()
            if peer.wait(timeout=5):
                raise RuntimeError('Firmware peer failed')
    print('PASS: ESP32 C++ packets -> Pi CarLink -> ESP32 C++ receipt parser/watchdog; '
          'fragmentation, wrap, lost reply, corrupt pair, stale ACK, fourth-miss trip and recovery.')


if __name__ == '__main__':
    main()
