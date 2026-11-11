"""Exercise the three production pin helpers with independent PORT/LAT storage.

This checks register contracts, not electrical behavior or PIC24 instructions.
Target disassembly and the live DIO battery provide those separate checks.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

from test_core_system import function

ROOT = Path(__file__).resolve().parents[1]
TEMP = Path.home() / "AppData/Local/Temp/opencode"

PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define HIGH 1
#define LOW 0
#define INPUT 1
#define OUTPUT 0
#define ON 1
#define OFF 0
static uint16_t latch;
static unsigned regulator, pullups, delays;
#define IOLAT latch
void bp_set_voltage_regulator_state(unsigned value) { regulator=value; }
void bp_set_pullup_state(unsigned value) { pullups=value; }
void bp_delay_us(unsigned value) { assert(value==5); ++delays; }
'''
TEST = r'''
static uint16_t pack(unsigned value) {
 return ((value & 16)?AUX:0) | ((value & 8)?MOSI:0) |
        ((value & 4)?CLK:0) | ((value & 2)?MISO:0) | ((value & 1)?CS:0);
}
int main(void) {
 const uint16_t mask=AUX|MOSI|CLK|MISO|CS;
 const uint16_t seeds[]={0,0xffff,0xa55a};
 unsigned checks=0;
 for (unsigned s=0;s<3;++s)
 for (unsigned levels=0;levels<32;++levels)
 for (unsigned command=0;command<256;++command) {
  uint16_t old_lat=seeds[s], old_dir=(uint16_t)~seeds[s];
  uint16_t old_port=(uint16_t)((seeds[s] & ~mask) | pack(levels));
  latch=old_lat;direction.word=old_dir;port.word=old_port;delays=0;
  assert(bitbang_pin_state_get()==levels);
  assert(bitbang_pin_state_set(command)==levels);
  assert(latch==(uint16_t)((old_lat & ~mask) | pack(command)));
  assert(direction.word==old_dir && port.word==old_port);
  assert(regulator==!!(command & 64) && pullups==!!(command & 32));
  assert(delays==1);
  latch=old_lat;delays=0;
  assert(bitbang_pin_direction_set(command)==levels);
  assert(direction.word==(uint16_t)((old_dir & ~mask) | pack(command)));
  assert(latch==old_lat && port.word==old_port && delays==1);
  ++checks;
 }
 printf("PASS %u pin-helper states: independent PORT/LAT, mapping, siblings, directions, PSU/pull-ups and settling\n",checks);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    args = parser.parse_args()

    def source(path):
        if args.revision:
            return subprocess.check_output(
                ["git", "show", args.revision + ":Firmware/" + path],
                cwd=ROOT, text=True, encoding="utf-8")
        return (ROOT / path).read_text(encoding="utf-8")

    src = source("binary_io.c")
    helpers = "\n".join(function(src, name) for name in (
        "bitbang_pin_state_get", "bitbang_pin_direction_set", "bitbang_pin_state_set"))
    for board, port in (("V3", "B"), ("V4", "D")):
        hardware = source("hardware" + board.lower() + ".h")
        pins = "\n".join(re.findall(
            r"^#define (?:BP_(?:AUX0|MOSI|CLK|MISO|CS)(?:_DIR)?|AUX0?|MOSI|CLK|MISO|CS) .*",
            hardware, re.M))
        registers = ""
        for name, field in (("port", "R" + port), ("direction", "TRIS" + port)):
            bits = "".join("unsigned " + field + str(i) + ":1;" for i in range(16))
            registers += "static union { uint16_t word; struct {" + bits + "} bits; } " + name + ";\n"
        registers += "#define PORT" + port + "bits port.bits\n"
        registers += "#define TRIS" + port + "bits direction.bits\n"
        with tempfile.TemporaryDirectory(prefix="bp-dio-pins-", dir=TEMP) as tmp:
            c = Path(tmp) / "test.c"
            exe = c.with_suffix(".exe")
            c.write_text(PREFIX + registers + pins + "\n" + helpers + TEST, encoding="ascii")
            subprocess.run(["gcc", "-std=gnu99", "-O1", "-Wall", "-Wextra", "-Werror",
                            str(c), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
            print("PASS", board, flush=True)


if __name__ == "__main__":
    main()
