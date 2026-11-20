"""Opt-in independent ICSP readback using the existing XM64 host API. CC0.

Does not erase or write flash. ICSP reads reset the target, and the existing
linecheck operation pulses MCLR to exit programming mode when finished.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
import time

from bench_v4_protocol import PATTERN


def run(args):
    sys.dont_write_bytecode = True
    tempfile.tempdir = str(args.log.resolve().parent)
    spec = importlib.util.spec_from_file_location("picprog", args.picprog)
    tool = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tool)
    boot = tool.load_words(str(args.bootloader.resolve()))
    app = tool.load_words(str(args.application.resolve())) if args.application else None
    if app:
        app[0], app[1] = 0x040400, 0
    result = {"started": time.time(), "bootloader_sha256": hashlib.sha256(args.bootloader.read_bytes()).hexdigest()}
    programmer = tool.Programmer(args.port)
    try:
        result["device"] = programmer.id()
        assert result["device"][0] == 0x1019
        result["config"] = programmer.read_config()
        assert tuple(result["config"]) == (0x3E7F, 0x239E)
        rows = {}
        for address in range(0x400, 0x2000, 128):
            got = programmer.read_data(address)
            assert got == tool.chunk_payload(boot, address // 2), f"loader mismatch at {address:06x}"
            rows[f"{address:06x}"] = struct.pack("<96H", *got).hex()
        result["loader_rows"] = rows
        result["loader_fingerprint"] = hashlib.sha256(b"".join(bytes.fromhex(v) for v in rows.values())).hexdigest()
        got = programmer.read_data(0)
        assert got == tool.chunk_payload(app if app else boot, 0), "reset/IVT mismatch"
        result["reset_vector_row"] = struct.pack("<96H", *got).hex()
        if app:
            rows = {}
            for page in range(8, 0x2A800 // 1024):
                if not any(w != tool.BLANK for w in app[page * 512:(page + 1) * 512]):
                    continue
                for row in range(8):
                    address = page * 1024 + row * 128
                    got = programmer.read_data(address)
                    assert got == tool.chunk_payload(app, address // 2), f"application mismatch at {address:06x}"
                    rows[f"{address:06x}"] = struct.pack("<96H", *got).hex()
            result["application_rows"] = rows
            result["application_verified_rows"] = len(rows)
        else:
            assert programmer.read_data(0x2000) == [65535] * 96, "application entry not blank"
        if args.scratch_pattern:
            words = [PATTERN[i + 1] | PATTERN[i + 2] << 8 | PATTERN[i] << 16 for i in range(0, 192, 3)]
            assert programmer.read_data(0x2400) == tool.chunk_payload(words, 0), "scratch pattern mismatch"
            result["scratch_pattern_verified"] = True
        if args.blank_scratch:
            for address in range(0x2400, 0x2800, 128):
                assert programmer.read_data(address) == [65535] * 96, f"scratch page not erased at {address:06x}"
            result["scratch_erased_rows"] = 8
        result["passed"] = True
        print(f"PASS: 56 loader rows, reset/IVT, CONFIG1/2; application rows={result.get('application_verified_rows', 0)}; scratch checks complete")
    except Exception as error:
        result["passed"] = False
        result["error"] = repr(error)
        raise
    finally:
        try:
            result["release_linecheck"] = programmer.linecheck()
        finally:
            programmer.close()
            result["elapsed_seconds"] = time.time() - result["started"]
            args.log.write_text(json.dumps(result, indent=2), encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--picprog", type=Path, required=True)
    parser.add_argument("--port", required=True)
    parser.add_argument("--bootloader", type=Path, required=True)
    parser.add_argument("--application", type=Path)
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("--scratch-pattern", action="store_true")
    parser.add_argument("--blank-scratch", action="store_true")
    run(parser.parse_args())
