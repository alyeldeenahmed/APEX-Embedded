import binascii
import struct
import unittest
from protocol import Decoder, crc16, data_packet, sync_packet


class ProtocolTests(unittest.TestCase):
    def test_crc_known_vector(self):
        self.assertEqual(crc16(b"123456789"), 0x29B1)

    def test_wire_offsets_and_independent_crc(self):
        p = data_packet(0x1234, 0x12345678, gyro=(1, -2, 3),
                        ticks=-123, speed=-1.5, wheel_age=500, steering=0.25,
                        steer_age=1000, flags=7)
        self.assertEqual(len(p), 54)
        self.assertEqual(p[:12], bytes.fromhex("a55a01010700341278563412"))
        self.assertEqual(struct.unpack_from("<i", p, 36)[0], -123)
        self.assertEqual(struct.unpack_from("<f", p, 40)[0], -1.5)
        self.assertEqual(struct.unpack_from("<H", p, 44)[0], 500)
        self.assertEqual(struct.unpack_from("<f", p, 46)[0], 0.25)
        self.assertEqual(struct.unpack_from("<H", p, 50)[0], 1000)
        self.assertEqual(struct.unpack_from("<H", p, 52)[0], binascii.crc_hqx(p[2:52], 0xffff))

    def test_fragmented_stream_and_garbage(self):
        d = Decoder()
        raw = b"noise\xa5" + data_packet(1, 10) + sync_packet(2**32 + 10)
        records = []
        for byte in raw:
            records += d.feed(bytes([byte]))
        self.assertEqual(len(records), 2)
        self.assertEqual(records[0]["seq"], 1)
        self.assertEqual(records[1]["t_esp_us"], 2**32 + 10)
        self.assertEqual(len(d.buffer), 0)

    def test_corruption_recovers(self):
        bad = bytearray(data_packet(1, 0))
        bad[24] ^= 1
        d = Decoder()
        r = d.feed(bad + data_packet(2, 10000))
        self.assertEqual([x["seq"] for x in r], [2])
        self.assertEqual(d.crc_errors, 1)

    def test_wrap(self):
        r = Decoder().feed(data_packet(65536, 2**32))
        self.assertEqual((r[0]["seq"], r[0]["t_imu_us"]), (0, 0))

    def test_bad_header_and_truncation(self):
        d = Decoder()
        packet = data_packet(5, 20)
        self.assertEqual(d.feed(b"\xa5\x5a\x03\x01" + packet[:20]), [])
        self.assertEqual(d.feed(packet[20:])[0]["seq"], 5)
        self.assertEqual(d.header_errors, 1)

    def test_sync_layout(self):
        p = sync_packet(0x0102030405060708)
        self.assertEqual(len(p), 14)
        self.assertEqual(p[:12], bytes.fromhex("a55a02010807060504030201"))
        self.assertEqual(struct.unpack_from("<H", p, 12)[0], binascii.crc_hqx(p[2:12], 0xffff))

    def test_invalid_inputs(self):
        with self.assertRaises(ValueError): data_packet(0, 0, flags=128)
        with self.assertRaises(ValueError): data_packet(0, 0, speed=float("nan"))


if __name__ == "__main__":
    unittest.main()
