"""Flash-config address/read contracts; fixture values are not installed fuses.

Compile the production table-read helper and actual status-report prefix.
Cross-check board definitions against the selected device linker scripts.
Keep generated sources and executables under Firmware/build/core-config-test
for AV diagnostics; retry a transient Windows access-denied launch only once.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import time

from test_core_system import function

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "Firmware" / "build" / "core-config-test"

PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint16_t TBLPAG;
static unsigned reads,emitted;
static uint32_t accessed[4];
static uint16_t output[4];
static const uint32_t expected[]={@@ADDRESSES@@};
static const uint16_t fixture_values[]={0xa135,0xb246,0xc357};
static uint16_t read_low(uint16_t offset) {
 uint32_t absolute=((uint32_t)TBLPAG<<16)|offset;
 assert(reads<4);accessed[reads++]=absolute;
 for(unsigned i=0;i<sizeof(expected)/sizeof(expected[0]);++i)
  if(absolute==expected[i]) return fixture_values[i];
 /* The old 128KB bank can be erased even when the real config is programmed. */
 return 0xffff;
}
#define __builtin_tblrdl(offset) read_low(offset)
#define __builtin_tblrdh(offset) (assert(!"must not read/merge the upper byte"),0)
static void bp_write_hex_word(uint16_t value) { assert(emitted<4);output[emitted++]=value; }
#define MSG_CFG0_FIELD ((void)0)
#define BPMSG1136 ((void)0)
#define BPMSG1137 ((void)0)
#define bpSP ((void)0)
#define bpBR ((void)0)
'''

TEST = r'''
int main(void) {
 unsigned count=sizeof(expected)/sizeof(expected[0]);
 for(unsigned saved=0;saved<256;++saved) {
  TBLPAG=saved;reads=emitted=0;
  print_status_info();
  assert(TBLPAG==saved && reads==count && emitted==count);
  for(unsigned i=0;i<count;++i) {
   assert(accessed[i]==expected[i]);
   assert(output[i]==fixture_values[i]);
  }
 }
 printf("PASS @@BOARD@@: %u config reads across 256 saved TBLPAG values, linker addresses and low-word display order\n",count*256);
}
'''


def run_fixture(exe):
    for attempt in range(2):
        try:
            subprocess.run([str(exe)], check=True)
            return
        except PermissionError as exc:
            if getattr(exc, "winerror", None) != 5:
                raise
            if attempt == 0:
                print("RETRY WinError 5 after 0.5s:", exe.resolve(), flush=True)
                time.sleep(0.5)
                continue
            print("BLOCKED executable retained:", exe.resolve(), flush=True)
            try:
                print("SHA256:", hashlib.sha256(exe.read_bytes()).hexdigest(), flush=True)
            except OSError as hash_error:
                print("SHA256 unavailable:", hash_error, flush=True)
            raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    args = parser.parse_args()

    def source(path):
        if args.revision:
            return subprocess.check_output(["git", "show", args.revision + ":Firmware/" + path],
                                           cwd=ROOT, text=True, encoding="utf-8")
        return (ROOT / "Firmware" / path).read_text(encoding="utf-8")

    base = function(source("core_system/base.c"), "bp_read_from_flash")
    # Keep the actual CFG0 guard, labels, calls and output order; omit unrelated
    # pin/ADC/protocol status after the first separator.
    status = function(source("core_system/proc_menu.c"), "print_status_info").split("  BPMSG1119;", 1)[0] + "}\n"
    expected_by_board = {"V4": (0x2ABFA, 0x2ABFC, 0x2ABFE), "V3": (0xABFC, 0xABFE)}
    for board, script, numbers in (("V4", "p24FJ256GB106.gld", (3, 2, 1)),
                                    ("V3", "p24FJ64GA002.gld", (2, 1))):
        linker = source(script)
        addresses = tuple(int(re.search(r"^__CONFIG" + str(number) + r"\s*=\s*(0x[0-9a-fA-F]+);", linker, re.M)[1], 16)
                          for number in numbers)
        assert addresses == expected_by_board[board]
        hardware = source("hardware" + board.lower() + ".h")
        definitions = "\n".join(re.findall(r"^#define CFG_ADDR_\w+\s+0x[0-9a-fA-F]+", hardware, re.M))
        prefix = PREFIX.replace("@@ADDRESSES@@", ",".join(hex(address) for address in addresses))
        test = TEST.replace("@@BOARD@@", board)
        # Separate board directories retain both fixtures, including a blocked EXE.
        build = BUILD / board
        build.mkdir(parents=True, exist_ok=True)
        c = build / "config.c"
        exe = c.with_suffix(".exe")
        c.write_text("#define BUSPIRATE" + board + "\n" + definitions + "\n" + prefix + base + status + test, encoding="ascii")
        subprocess.run(["gcc", "-std=c99", "-Wall", "-Wextra", "-Werror", str(c), "-o", str(exe)], check=True)
        run_fixture(exe)
        print("PASS", board, "status mapping", ", ".join(hex(address) for address in addresses), flush=True)

    # Device selection and the shared DEVID path are not flash-size-dependent.
    config = source("configuration.h")
    assert "#if defined(__PIC24FJ256GB106__)" in config
    hardware = source("hardware.h")
    for name, value in (("DEV_ADDR_UPPER", 0xFF), ("DEV_ADDR_TYPE", 0), ("DEV_ADDR_REV", 2)):
        assert int(re.search(r"^#define " + name + r"\s+(0x[0-9a-fA-F]+)", hardware, re.M)[1], 16) == value
    v3 = source("hardwarev3.h")
    assert re.search(r"^#define BL_ADDR_VER 0xABFA\b", v3, re.M)
    assert "CFG_ADDR_0" not in v3
    print("PASS V4 device selection, separate DEVID space, and unchanged V3 two-word/bootloader contracts")


if __name__ == "__main__":
    main()
