"""Check the real XC16 feature gate: supported on BPv4, absent on BPv3.

Accepts an optional compiler executable path, as does the CDC gate test.
"""
from pathlib import Path
import os
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build/avrisp-board-gate"
OUT.mkdir(parents=True, exist_ok=True)
compiler = (sys.argv[1] if len(sys.argv) > 1 else str(Path(os.environ.get(
    "XC16_DIR", "C:/Program Files (x86)/Microchip/MPLABXC16/v2.10")) /
    "bin/xc16-gcc.exe"))
nm = str(Path(compiler).with_name("xc16-nm" + Path(compiler).suffix))
probe = '''#include "configuration.h"
#ifdef BUSPIRATEV4
#ifndef BP_ENABLE_AVRISP_MKII_SUPPORT
#error "BPv4 programmer support unexpectedly disabled"
#endif
#else
#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT
#error "Unsupported board exposes the programmer"
#endif
#endif
'''
for cpu in ("24FJ256GB106", "24FJ64GA002"):
    subprocess.run([compiler, "-mcpu="+cpu, "-I", str(ROOT), "-E", "-P", "-x", "c", "-"],
                   input=probe, text=True, capture_output=True, check=True)
forced = subprocess.run([compiler, "-mcpu=24FJ64GA002", "-I", str(ROOT),
                         "-DBP_ENABLE_AVRISP_MKII_SUPPORT", "-E", "-P", "-x", "c", "-"],
                        input=probe, text=True, capture_output=True)
assert forced.returncode != 0 and "requires Bus Pirate v4" in forced.stderr
for source in ROOT.glob("avrisp_*.c"):
    obj = OUT / (source.stem+".o")
    subprocess.run([compiler, "-mcpu=24FJ64GA002", "-O1", "-I", str(ROOT),
                    "-c", str(source), "-o", str(obj)], check=True, capture_output=True)
    symbols = subprocess.run([nm, "-g", str(obj)], check=True, capture_output=True, text=True).stdout
    assert not re.search(r"\b_avrisp_", symbols), source.name
dispatch = subprocess.run([compiler, "-mcpu=24FJ64GA002", "-I", str(ROOT),
                           "-E", "-P", str(ROOT/"binary_io.c")],
                          check=True, capture_output=True, text=True).stdout
assert "avrisp_mkii_enter(" not in dispatch
print("XC16 board gate: BPv4 enabled; BPv3 excluded, forced enable rejected, no programmer symbols/dispatch")
