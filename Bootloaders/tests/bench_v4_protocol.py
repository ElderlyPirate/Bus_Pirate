"""Opt-in v4 serial regression, not an installer. CC0.

Requires a resident loader with an ERASED application entry. The optional
--scratch-pattern step erases/writes application page 0x2400; never run it
on an application that must be preserved. Use pirate-loader for app installs
and the existing XM64 picprog.py for independent readback.
"""
import argparse
import datetime
import json
from pathlib import Path
import time

import serial

SCRATCH = 0x2400
PATTERN = bytes((i * 37 + 11) & 255 for i in range(192))


def packet(address, command, size=1, payload=None, bad_crc=False):
    result = bytearray(address.to_bytes(3, "big") + bytes([command, size]))
    if size:
        if payload is None:
            payload = bytes((i * 13) & 255 for i in range(size - 1))
        assert len(payload) == size - 1
        result += payload
        result.append((-sum(result) & 255) ^ bool(bad_crc))
    return bytes(result)


def run(port, output, scratch=False, clear=False):
    records = []

    def record(name, actual, expected):
        item = {"test": name, "actual": actual.hex(), "expected": expected.hex(),
                "passed": actual == expected,
                "utc": datetime.datetime.now(datetime.timezone.utc).isoformat()}
        records.append(item)
        output.write_text(json.dumps(records, indent=2), encoding="utf-8")
        if actual != expected:
            raise AssertionError(item)

    with serial.Serial(port, 115200, timeout=3, write_timeout=5) as connection:
        connection.reset_input_buffer()

        def hello(name="hello"):
            connection.write(b"\xc1")
            connection.flush()
            record(name, connection.read(4), b"\xf1\x05\x00K")

        def command(name, frame, expected, fragments=False):
            if fragments:
                for i in range(0, len(frame), 7):
                    connection.write(frame[i:i + 7])
                    time.sleep(0.002)
            else:
                connection.write(frame)
            connection.flush()
            record(name, connection.read(len(expected)), expected)

        hello()
        if not clear:
            for size in range(256):
                command(f"write-length-{size}", packet(0x400, 2, size),
                        b"P" if size == 193 else b"U")
            for offset in range(1, 128):
                command(f"unaligned-row-{offset}", packet(0x2000 + offset, 2, 193), b"P")
            for address in range(0x400, 0x2000, 128):
                command(f"protected-loader-{address:06x}", packet(address, 2, 193), b"P")
            for address in (0x2A800, 0x2AB80, 0x2AC00, 0x800000, 0xFFFF80):
                command(f"protected-config-or-range-{address:06x}", packet(address, 2, 193), b"P")
            for address in (0x80, 0x2080, 0x400, 0x1C00, 0x2A800):
                command(f"invalid-or-protected-erase-{address:06x}", packet(address, 1), b"P")
            command("bad-write-checksum", packet(0x2000, 2, 193, bad_crc=True), b"N")
            command("bad-erase-checksum", packet(0x2000, 1, bad_crc=True), b"N")
            command("unknown-command", packet(0x2000, 0x82, 193), b"U")
            command("fragmented-oversize", packet(0x2000, 2, 255), b"U", fragments=True)
            command("recovery-after-oversize", packet(0x400, 2, 193), b"P")
            command("arm-erase-page", packet(0x2000, 1), b"K")
            command("erase-page-mismatch", packet(SCRATCH, 2, 193), b"P")
            for count in (64, 256):
                command(f"pipelined-{count}", packet(0, 0x82) * count, b"U" * count)
            connection.write(b"\x00\x20")
            connection.flush()
            time.sleep(1.2)
            hello("partial-header-timeout-resync")
            connection.write(packet(0x2000, 2, 193)[:37])
            connection.flush()
            time.sleep(1.2)
            hello("partial-row-timeout-resync")
        if scratch or clear:
            command("scratch-arm-erase", packet(SCRATCH, 1), b"K")
            data = bytes([255]) * 192 if clear else PATTERN
            command("scratch-erase-row-write-verify", packet(SCRATCH, 2, 193, data), b"K")
        command("restart-ack", packet(0x010203, 0xFF), b"K")
    print(f"{len(records)} hardware protocol checks passed; log: {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--log", required=True, type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--scratch-pattern", action="store_true")
    mode.add_argument("--clear-scratch", action="store_true")
    args = parser.parse_args()
    run(args.port, args.log, args.scratch_pattern, args.clear_scratch)
