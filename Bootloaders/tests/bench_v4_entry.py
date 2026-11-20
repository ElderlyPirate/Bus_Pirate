"""Opt-in software entry/restart regression; no ICSP or physical actions. CC0."""
import argparse
import json
from pathlib import Path
import time

import serial
import serial.tools.list_ports


def wait_port(pid):
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        found = [p.device for p in serial.tools.list_ports.comports()
                 if p.vid == 0x04D8 and p.pid == pid]
        if len(found) == 1:
            time.sleep(0.3)
            return found[0]
        time.sleep(0.2)
    raise AssertionError(f"no unique BP port for PID {pid:04x}")


def run(rounds, output, expected_firmware):
    records = []
    for number in range(1, rounds + 1):
        entry = {"round": number}
        records.append(entry)
        try:
            port = wait_port(0xFB00)
            with serial.Serial(port, 115200, timeout=0.25, write_timeout=3) as connection:
                connection.reset_input_buffer()
                connection.write(b"\r")
                connection.flush()
                time.sleep(0.3)
                prompt = connection.read(4096)
                assert b"HiZ>" in prompt, repr(prompt)
                connection.write(b"$\r")
                connection.flush()
                time.sleep(0.7)
                confirmation = connection.read(4096)
                entry["confirmation"] = confirmation.decode("latin1")
                assert b"Are you sure" in confirmation, repr(confirmation)
                connection.write(b"y")
                connection.flush()
                # Match the existing enter_bootloader.py helper: keep reading
                # until detach. Closing here can strand the application's
                # transmission-done wait before it executes RESET.
                deadline = time.monotonic() + 5
                after_y = bytearray()
                while time.monotonic() < deadline:
                    try:
                        after_y += connection.read(256)
                    except serial.SerialException:
                        break
                    if any(p.vid == 0x04D8 and p.pid == 0xFAFF
                           for p in serial.tools.list_ports.comports()):
                        break
                entry["after_y"] = after_y.decode("latin1")
            port = wait_port(0xFAFF)
            with serial.Serial(port, 115200, timeout=3, write_timeout=3) as connection:
                connection.reset_input_buffer()
                connection.write(b"\xc1")
                connection.flush()
                hello = connection.read(4)
                entry["bootloader_hello"] = hello.hex()
                assert hello == b"\xf1\x04\x0fK", repr(hello)
                packet = bytearray([1, 2, 3, 255, 1])
                packet.append(-sum(packet) & 255)
                connection.write(packet)
                connection.flush()
                ack = connection.read(1)
                entry["restart_ack"] = ack.hex()
                assert ack == b"K", repr(ack)
            port = wait_port(0xFB00)
            with serial.Serial(port, 115200, timeout=0.25, write_timeout=3) as connection:
                connection.reset_input_buffer()
                connection.write(b"\ri\r")
                connection.flush()
                time.sleep(0.6)
                banner = connection.read(16384)
                entry["application_banner"] = banner.decode("latin1")
                assert b"HiZ>" in banner and expected_firmware.encode() in banner, repr(banner)
            entry["passed"] = True
            print(f"PASS round {number}: firmware request -> loader 4.15 -> restart -> application, no replug")
        except Exception as error:
            entry["error"] = repr(error)
            entry["passed"] = False
            raise
        finally:
            output.write_text(json.dumps(records, indent=2), encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("--expected-firmware", required=True)
    args = parser.parse_args()
    if not 1 <= args.rounds <= 100:
        parser.error("rounds must be 1..100")
    run(args.rounds, args.log, args.expected_firmware)
