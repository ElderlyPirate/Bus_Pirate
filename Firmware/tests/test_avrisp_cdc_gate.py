"""Check the real XC16 CDC translation unit, without injected feature defines.

Usage: python tests/test_avrisp_cdc_gate.py [path/to/xc16-gcc.exe]
Native PHY mocks predefine the feature flags and cannot catch include-order
bugs in this integration boundary.
"""
from pathlib import Path
import os
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
compiler = (sys.argv[1] if len(sys.argv) > 1 else str(Path(os.environ.get(
    "XC16_DIR", "C:/Program Files (x86)/Microchip/MPLABXC16/v2.10")) /
    "bin" / "xc16-gcc.exe"))
result = subprocess.run(
    [compiler, "-mcpu=24FJ256GB106", "-std=gnu99", "-I", str(root),
     "-E", "-P", str(root / "dp_usb" / "cdc.c")],
    check=True, capture_output=True, text=True,
)
text = result.stdout
start = text.index("void user_configured_init(void) {")
end = text.index("void cdc_setup(void) {", start)
body = text[start:end]
assert "avrisp_usb_personality_active" in body, "mkII dispatch was compiled out"
assert "avrisp_usb_configure_endpoints();" in body, "mkII endpoint hook is absent"
assert "CDCFlushOnTimeout" in body, "normal CDC configuration was lost"
print("XC16 CDC integration: mkII dispatch and CDC fallback are both compiled")
