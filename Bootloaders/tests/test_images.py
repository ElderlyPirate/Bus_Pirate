"""Check the linked Intel HEX images, not just source constants. CC0."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

def load(path):
    memory, base = {}, 0
    for line in path.read_text().splitlines():
        raw = bytes.fromhex(line[1:])
        if sum(raw) & 255 or len(raw) != raw[0] + 5:
            raise AssertionError("bad Intel HEX record")
        size, address, kind = raw[0], int.from_bytes(raw[1:3], "big"), raw[3]
        if kind == 4: base = int.from_bytes(raw[4:6], "big") << 16
        elif kind == 0:
            for i, byte in enumerate(raw[4:4 + size]):
                target = base + address + i
                if target in memory and memory[target] != byte:
                    raise AssertionError("conflicting HEX data")
                memory[target] = byte
        elif kind == 1: return memory
        else: raise AssertionError("unexpected Intel HEX record")
    raise AssertionError("missing EOF")

def word(memory, address):
    return sum(memory.get(address * 2 + i, 255) << (i * 8) for i in range(3))

class LinkedImages(unittest.TestCase):
    def image(self, revision):
        path = ROOT / revision / "build" / ("bp" + revision + "-bootloader.hex")
        self.assertTrue(path.exists(), "Build both loaders before running the battery")
        return load(path)

    def test_v4_layout(self):
        memory = self.image("v4")
        self.assertEqual(word(memory, 0), 0x040400)
        self.assertEqual(word(memory, 2), 0)
        self.assertEqual(word(memory, 0x1FF6) & 0xFFFF, 0x040F)
        self.assertEqual(word(memory, 0x2ABFE) & 0xFFFF, 0x3E7F)
        self.assertEqual(word(memory, 0x2ABFC) & 0xFFFF, 0x239E)
        for byte_address in memory:
            pc = byte_address // 4 * 2
            self.assertTrue(pc < 0x200 or 0x400 <= pc < 0x2000 or pc >= 0x2ABFA,
                            hex(pc))
        self.assertFalse(any(0x2000 <= a // 4 * 2 < 0x2ABFA for a in memory))

    def test_v3_layout(self):
        memory = self.image("v3")
        self.assertEqual(word(memory, 0), 0x04A800)
        self.assertEqual(word(memory, 2), 0)
        self.assertEqual(word(memory, 0xABFA) & 0xFFFF, 0x0406)
        self.assertEqual(word(memory, 0xA7FC), 0xFFFFFF)
        self.assertEqual(word(memory, 0xA7FE), 0xFFFFFF)
        self.assertEqual(word(memory, 0xABFE) & 0xFFFF, 0x3F7F)
        self.assertEqual(word(memory, 0xABFC) & 0xFFFF, 0x799F)
        for byte_address in memory:
            pc = byte_address // 4 * 2
            self.assertTrue(pc < 0x200 or 0xA7FC <= pc < 0xAC00, hex(pc))
        map_text = (ROOT / "v3/build/bpv3-bootloader.map").read_text()
        import re
        end = int(re.search(r"0x([0-9a-f]+)\s+loader_end", map_text)[1], 16)
        self.assertLessEqual(end, 0xABF8)

if __name__ == "__main__": unittest.main()
