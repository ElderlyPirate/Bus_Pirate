"""Hardware-free checks for retired core declarations and stable USB encodings."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
TEMP = Path.home() / "AppData/Local/Temp/opencode"


def source(path, revision):
    if revision:
        return subprocess.check_output(
            ["git", "show", revision + ":" + path], cwd=ROOT,
            text=True, encoding="utf-8")
    return (ROOT / path).read_text(encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    args = parser.parse_args()
    paths = subprocess.check_output(
        ["git", "ls-files", "Firmware"], cwd=ROOT, text=True).splitlines()
    retired = re.compile(r"\b(?:last_command|command_t|POWERED_STATE|ADR_PENDING_STATE|USB_DEVICE_STATE)\b")
    for path in paths:
        if Path(path).suffix not in (".c", ".h", ".s") or path.startswith("Firmware/tests/"):
            continue
        text = source(path, args.revision)
        text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
        assert not retired.search(text), path
    print("PASS no production definition, extern, dead type or state reference remains", flush=True)

    header = source("Firmware/core_usb/usb_stack.h", args.revision)
    states = re.search(r"enum\s*\{\s*DETACHED_STATE\b.*?\};", header, re.S)
    assert states
    assert re.search(r"extern\s+volatile\s+BYTE\s+usb_device_state\s*;", header)
    program = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
@@STATES@@
int main(void) {
 assert(DETACHED_STATE==0x00 && ATTACHED_STATE==0x01 && DEFAULT_STATE==0x04);
 assert(ADDRESS_STATE==0x10 && CONFIGURED_STATE==0x20);
 volatile uint8_t state=ADDRESS_STATE;
 assert(state<CONFIGURED_STATE);
 state=CONFIGURED_STATE;
 assert(state>=CONFIGURED_STATE && sizeof(state)==1);
 puts("PASS active USB values/comparisons and the byte-sized state contract are unchanged");
}
'''.replace("@@STATES@@", states[0])
    with tempfile.TemporaryDirectory(prefix="bp-core-cleanup-", dir=TEMP) as tmp:
        c = Path(tmp) / "states.c"
        exe = c.with_suffix(".exe")
        c.write_text(program, encoding="ascii")
        subprocess.run(["gcc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                        str(c), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
