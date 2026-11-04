import unittest
import struct
import control_link as c


class ControlLinkTests(unittest.TestCase):
    def test_wire_layout(self):
        p = c.encode(action='set', boot=42, sequence=7, session=2, epoch=3,
                     command_id=8, created_us=100, deadline_us=1000,
                     steering=.25, acceleration=-2)
        self.assertEqual(len(p), 64)
        self.assertEqual(p[:5], bytes([0xa5, 0x5a, 0x10, 1, 3]))
        self.assertEqual(struct.unpack_from('<6I2Q2f', p, 8),
                         (42, 7, 2, 3, 8, 0, 100, 1000, .25, -2))
        self.assertEqual(c.crc(b'123456789'), 0x29b1)
        self.assertEqual(struct.unpack_from('<H', p, 62)[0], c.crc(p[2:62]))

    def test_validation(self):
        fields = dict(action='arm', boot=1, sequence=1, session=1, epoch=1,
                      command_id=1, created_us=0, deadline_us=100)
        for change in (dict(steering=1), dict(acceleration=float('nan')),
                       dict(sequence=-1), dict(command_id=0), dict(deadline_us=0)):
            with self.assertRaises(ValueError):
                c.encode(**(fields | change))

    def test_mixed_stream(self):
        p = bytearray(64)
        p[:4] = b'\xa5\x5a\x11\x01'
        struct.pack_into('<I', p, 8, 42)
        struct.pack_into('<H', p, 62, c.crc(p[2:62]))
        parser = c.Stream()
        frames = []
        for byte in b'noise'+p:
            frames += parser.feed(bytes([byte]))
        self.assertEqual(c.status(frames[0])['boot'], 42)
        bad = bytearray(p);bad[10] ^= 1
        self.assertEqual(parser.feed(bad+p), [bytes(p)])
        self.assertGreater(parser.errors, 0)


if __name__ == '__main__':
    unittest.main()
