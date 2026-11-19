"""Native source contracts for the #227 JTAG/XSVF repairs (no board I/O).

The two behaviours are hardware-path conditions (a stalled UART stream and a
v3 assembly transfer), so there is no PIC24 emulator here; the fixture pins
the production source in the bounded form the repair introduced:

  RA-6  xsvfDoXCOMMENT() leaves its comment scan on xsvf_stream_stalled
        instead of looping on the 0xFF the stalled readByte() serves.
  RA-9  the v3 CMD_TAP_SHIFT branch skips binOpenOCDTapShiftFast() for a
        zero count (the asm computes bits-1 -> 0xFFFF); the v4 branch uses a
        while (>0) loop that skips a zero count.

It also checks that the other stalled-stream results are unchanged: the
stalled byte is still 0xFF, so an illegal command (0x04), an undefined TAP
state (0x05) and the XSDRSIZE bound (0x06) keep their existing behaviour.

python Firmware/tests/test_jtag_xsvf_contracts.py
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]

micro = (ROOT / "Firmware/tool_xsvf/micro.c").read_text()
ports_c = (ROOT / "Firmware/tool_xsvf/ports.c").read_text()
ports_h = (ROOT / "Firmware/tool_xsvf/ports.h").read_text()
openocd = (ROOT / "Firmware/tool_openocd/openocd.c").read_text()
asm = (ROOT / "Firmware/tool_openocd/openocd_asm.s").read_text()

OK = []
BAD = []


def check(name, cond, detail=""):
    (OK if cond else BAD).append(name)
    print("[%s] %s %s" % ("PASS" if cond else "FAIL", name, detail))


# --- RA-6: XCOMMENT must stop on a stalled host stream --------------------
match = re.search(r"int xsvfDoXCOMMENT\s*\([^)]*\)\s*\{(.*?)\n\}", micro, re.S)
check("RA-6 xsvfDoXCOMMENT body found", match is not None)
body = match.group(1) if match else ""
check("RA-6 XCOMMENT loop exits on xsvf_stream_stalled",
      re.search(r"\}\s*while\s*\(\s*ucText\s*&&\s*!\s*xsvf_stream_stalled\s*\)\s*;", body)
      is not None,
      "while (ucText && !xsvf_stream_stalled)")
check("RA-6 stalled flag is exported to the player",
      re.search(r"extern\s+bool\s+xsvf_stream_stalled\s*;", ports_h) is not None)
check("RA-6 stalled wait still serves 0xFF (0x04/0x05/0x06 unchanged)",
      re.search(r"if\s*\(\s*xsvf_stream_stalled\s*\)\s*\{\s*\*out\s*=\s*0xff", ports_c)
      is not None)
check("RA-6 stalled readByte still yields an illegal-command byte",
      re.search(r"if\s*\(\s*xsvf_stream_stalled\s*&&\s*bufBytes\s*==\s*0\s*\)"
                r"\s*\{[^}]*buf\[0\]\s*=\s*0xff", ports_c, re.S) is not None)
check("RA-6 player unwinds at the command boundary on a stall",
      re.search(r"if\s*\(\s*xsvf_stream_stalled\s*\)\s*"
                r"\{[^}]*XSVF_ERROR_UNKNOWN", micro, re.S) is not None)

# --- RA-9: v3 zero-count guard (v4 already guarded) -----------------------
v3_guard = re.search(
    r"if\s*\(\s*j\s*\)\s*\{.*?binOpenOCDTapShiftFast\s*\(\s*UART1RXBuf", openocd, re.S)
check("RA-9 v3 fast shift is guarded by if (j)", v3_guard is not None)
check("RA-9 v3 non-zero path still prepares 2*i RX bytes",
      re.search(r"if\s*\(\s*j\s*\)\s*\{.*?UART1RXToRecv\s*=\s*2\s*\*\s*i\s*;"
                r".*?binOpenOCDTapShiftFast", openocd, re.S) is not None)
check("RA-9 v3 asm still computes bits-1 (the reason for the guard)",
      re.search(r"dec\.w\s+w2\s*,\s*w6", asm) is not None)
check("RA-9 v4 zero-bit shift uses a while (>0) loop",
      re.search(r"while\s*\(\s*bit_sequences\s*>\s*0\s*\)", openocd) is not None)
check("RA-9 v4 zero-bit shift is not a do/while again",
      re.search(r"\}\s*while\s*\(\s*bit_sequences\s*>\s*0\s*\)\s*;", openocd) is None)

print("%d passed, %d failed" % (len(OK), len(BAD)))
raise SystemExit(1 if BAD else 0)
