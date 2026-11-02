import binascii
import json
from pathlib import Path
import unittest
from protocol import Decoder, data_packet, sync_packet


class ReferencePacketTests(unittest.TestCase):
    def test_reference_packets(self):
        cases = json.loads((Path(__file__).parent / "tests/packet_vectors.json").read_text())["cases"]
        for case in cases:
            with self.subTest(name=case["name"]):
                expected = bytes.fromhex(case["hex"])
                actual = sync_packet(case["timestamp"]) if case["type"] == "sync" else data_packet(**case["values"])
                self.assertEqual(actual, expected)
                self.assertEqual(int.from_bytes(expected[-2:], "little"), binascii.crc_hqx(expected[2:-2], 0xffff))
                self.assertEqual(len(Decoder().feed(expected)), 1)
