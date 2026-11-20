"""Build/run native tests; no port opens, no programmer access. CC0."""
import os
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]

class NativeTests(unittest.TestCase):
    def build(self, name):
        cc = shutil.which(os.environ.get("HOST_CC", "gcc"))
        if not cc:
            self.fail("Install a native C compiler or set HOST_CC")
        build = ROOT / "tests" / "build"
        build.mkdir(exist_ok=True)
        exe = build / (name + (".exe" if os.name == "nt" else ""))
        subprocess.run([cc, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-I" + str(ROOT / "tests" / "mock"),
                        str(ROOT / "tests" / (name + ".c")), "-o", str(exe)], check=True)
        return exe

    def test_production_v4_state_machines(self):
        exe = self.build("test_v4")
        subprocess.run([str(exe)], check=True)

    def test_loader(self):
        exe = self.build("test_loader")
        subprocess.run([str(exe)], check=True)
        def record(address, kind, data=b""):
            raw = bytes([len(data), address >> 8, address & 255, kind]) + data
            return ":" + (raw + bytes([-sum(raw) & 255])).hex() + "\n"
        eof = record(0, 1)
        good = record(0, 0, bytes.fromhex("12345600"))
        cases = {"good": (good + eof, True), "lowercase": (good.lower() + eof, True),
                 "missing-eof": (good, False), "bad-digit": (good.replace("56", "5g") + eof, False),
                 "misaligned-address": (record(2, 0, bytes(4)) + eof, False),
                 "misaligned-size": (record(0, 0, bytes(3)) + eof, False),
                 "buffer-overrun": (record(0, 0, bytes(24)) + eof, False),
                 "flash-overrun": (record(0, 4, b"\x00\x05") + record(0x57FC, 0, bytes(8)) + eof, False),
                 "high-address": (record(0, 4, b"\x80\x00") + good + eof, False),
                 "bad-checksum": (good[:-3] + "01\n" + eof, False),
                 "bad-eof": (good + record(0, 1, bytes(1)), False),
                 "after-eof": (good + eof + good, False),
                 "bad-ela-address": (record(4, 4, bytes(2)) + good + eof, False)}
        for name, (text, succeeds) in cases.items():
            with self.subTest(name=name):
                path = ROOT / "tests" / "build" / (name + ".hex")
                path.write_text(text)
                result = subprocess.run([str(exe), str(path)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0 if succeeds else 2, result.stdout + result.stderr)
        # XC16 bin2hex emits contiguous records across 64 KiB boundaries.
        path = ROOT / "tests/build/cross-bank.hex"
        path.write_text(record(0xFFFC, 0, bytes.fromhex("12345600") * 4) + eof)
        subprocess.run([str(exe), str(path), "--cross-bank"], check=True)

if __name__ == "__main__":
    unittest.main()
