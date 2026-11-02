"""Perception v0.1 packet prototype. Standard library only."""
import argparse
import json
import math
import struct
from pathlib import Path

SYNC = b"\xa5\x5a"
BODY = struct.Struct("<BBBBHI6fifHfH")


def crc16(data):
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ (0x1021 if crc & 0x8000 else 0)) & 0xFFFF
    return crc


def frame(body):
    return SYNC + body + struct.pack("<H", crc16(body))


def data_packet(seq, timestamp, gyro=(0., 0., 0.), accel=(0., 0., 9.80665),
                ticks=0, speed=0., wheel_age=0, steering=0., steer_age=0, flags=0):
    if flags & ~0x7F:
        raise ValueError("Reserved flag must be zero")
    if not all(math.isfinite(x) for x in (*gyro, *accel, speed, steering)):
        raise ValueError("Non-finite sensor value")
    body = BODY.pack(1, 1, flags, 0, seq & 0xFFFF, timestamp & 0xFFFFFFFF,
                     *gyro, *accel, ticks, speed, wheel_age, steering, steer_age)
    return frame(body)


def sync_packet(timestamp):
    # Provisional coverage: bytes 2 through 11; requires Perception agreement.
    return frame(struct.pack("<BBQ", 2, 1, timestamp))


class Decoder:
    def __init__(self):
        self.buffer = bytearray()
        self.crc_errors = 0
        self.header_errors = 0

    def feed(self, chunk):
        self.buffer.extend(chunk)
        result = []
        while True:
            start = self.buffer.find(SYNC)
            if start < 0:
                self.buffer[:] = self.buffer[-1:] if self.buffer[-1:] == b"\xa5" else b""
                break
            del self.buffer[:start]
            if len(self.buffer) < 4:
                break
            kind, version = self.buffer[2:4]
            if kind not in (1, 2) or version != 1:
                self.header_errors += 1
                del self.buffer[0]
                continue
            size = 54 if kind == 1 else 14
            if len(self.buffer) < size:
                break
            packet = bytes(self.buffer[:size])
            if crc16(packet[2:-2]) != struct.unpack("<H", packet[-2:])[0]:
                self.crc_errors += 1
                del self.buffer[0]
                continue
            del self.buffer[:size]
            if kind == 2:
                result.append({"type": "sync", "t_esp_us": struct.unpack_from("<Q", packet, 4)[0]})
                continue
            v = BODY.unpack(packet[2:-2])
            if v[3] != 0 or v[2] & 0x80:
                self.header_errors += 1
                continue
            result.append(dict(type="data", flags=v[2], seq=v[4], t_imu_us=v[5],
                               gyro=v[6:9], accel=v[9:12], enc_ticks=v[12],
                               wheel_speed=v[13], wheel_age_us=v[14],
                               steering=v[15], steer_age_us=v[16]))
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("generate", "decode"))
    parser.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.action == "generate":
        packets = [sync_packet(0)]
        packets += [data_packet(i, i * 10000, flags=64 if i == 0 else 0) for i in range(100)]
        args.path.write_bytes(b"".join(packets))
        print("Wrote 100 SIMULATED data packets and one provisional sync frame.")
    else:
        decoder = Decoder()
        with args.path.open("rb") as capture:
            while chunk := capture.read(4096):
                for record in decoder.feed(chunk):
                    print(json.dumps(record))
        print(json.dumps(dict(crc_errors=decoder.crc_errors,
                              header_errors=decoder.header_errors,
                              trailing_bytes=len(decoder.buffer))))


if __name__ == "__main__":
    main()
