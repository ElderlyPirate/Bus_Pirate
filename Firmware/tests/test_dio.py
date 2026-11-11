"""Native production-source DIO contracts, not an electrical/PIC24 simulator."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TEMP = Path.home() / "AppData/Local/Temp/opencode"

BASE = r'''
#ifndef FIXTURE_BASE_H
#define FIXTURE_BASE_H
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define NO 0
typedef struct { unsigned write_with_read, numbits, int16; } mode_configuration_t;
extern mode_configuration_t mode_configuration;
extern uint16_t direction, latch;
extern unsigned pullups, vpu33, vpu50;
#define PARK_MASK (AUX | MOSI | CLK | MISO | CS)
#define IODIR direction
uint16_t *fixture_latch(void);
#define IOLAT (*fixture_latch())
void bp_disable_pullup(void);
void bp_disable_3v3_pullup(void);
#endif
'''
BINARY = r'''
#include <stdint.h>
uint8_t bitbang_pin_state_get(void);
uint8_t bitbang_pin_direction_set(uint8_t);
uint8_t bitbang_pin_state_set(uint8_t);
'''
ROUTING = r'''
#include "base.h"
#include "dio.h"
mode_configuration_t mode_configuration;
static unsigned state_calls, direction_calls;
static uint8_t argument;
uint8_t bitbang_pin_state_get(void) { return 0x15; }
uint8_t bitbang_pin_state_set(uint8_t value) {
 ++state_calls; argument=value; return 0x12;
}
uint8_t bitbang_pin_direction_set(uint8_t value) {
 ++direction_calls; argument=value; return 0x09;
}
int main(void) {
 assert(dio_read()==0x15);
 for (unsigned value=0;value<65536;++value) {
  state_calls=direction_calls=0;
  assert(dio_write(value)==((value & 0x80)?0x12:0x09));
  assert(argument==(uint8_t)value);
  assert(state_calls==!!(value & 0x80));
  assert(direction_calls==!(value & 0x80));
 }
 mode_configuration.write_with_read=1;
 dio_setup_prepare();
 assert(!mode_configuration.write_with_read);
 puts("PASS 65536 DIO command values, read delegation and entry flag reset");
}
'''
PARK_SUPPORT = r'''
uint16_t direction, latch;
unsigned pullups, vpu33, vpu50;
uint16_t *fixture_latch(void) {
 assert((direction & PARK_MASK)==PARK_MASK && !pullups);
#ifdef BUSPIRATEV4
 assert(!vpu33 && !vpu50);
#endif
 return &latch;
}
void bp_disable_pullup(void) {
 assert((direction & PARK_MASK)==PARK_MASK);
 pullups=0;
}
void bp_disable_3v3_pullup(void) {
 assert(!pullups);
 vpu33=vpu50=0;
}
'''
CLEANUP = r'''
#include "base.h"
#include "dio.h"
mode_configuration_t mode_configuration;
uint8_t bitbang_pin_state_get(void) { return 0; }
uint8_t bitbang_pin_state_set(uint8_t value) { return value; }
uint8_t bitbang_pin_direction_set(uint8_t value) { return value; }
int main(void) {
 for (unsigned seed=0;seed<65536;++seed) {
  direction=(uint16_t)seed;
  latch=(uint16_t)~seed;
  uint16_t old_dir=direction, old_lat=latch;
  pullups=vpu33=vpu50=1;
  mode_configuration.numbits=16;
  mode_configuration.int16=1;
  mode_configuration.write_with_read=1;
  dio_cleanup();
  assert(direction==(uint16_t)(old_dir | PARK_MASK));
  assert(latch==(uint16_t)(old_lat & ~PARK_MASK));
  assert(!pullups && mode_configuration.numbits==8 && !mode_configuration.int16);
  assert(mode_configuration.write_with_read==1);
#ifdef BUSPIRATEV4
  assert(!vpu33 && !vpu50);
#else
  assert(vpu33 && vpu50);
#endif
  dio_cleanup();
  assert(direction==(uint16_t)(old_dir | PARK_MASK));
  assert(latch==(uint16_t)(old_lat & ~PARK_MASK));
 }
 puts("PASS 65536 cleanup states: release/bias/latch order, siblings, width and idempotence");
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    parser.add_argument("--case", choices=("disabled", "routing", "cleanup"))
    parser.add_argument("--board", choices=("V3", "V4"))
    args = parser.parse_args()

    def source(path):
        if args.revision:
            return subprocess.check_output(
                ["git", "show", args.revision + ":Firmware/" + path],
                cwd=ROOT, text=True, encoding="utf-8")
        return (ROOT / path).read_text(encoding="utf-8")

    for board in ((args.board,) if args.board else ("V3", "V4")):
      hardware = source("hardware" + board.lower() + ".h")
      pins = "\n".join(re.findall(r"^#define (?:AUX0?|MOSI|CLK|MISO|CS) .*", hardware, re.M))
      for case in ((args.case,) if args.case else ("disabled", "routing", "cleanup")):
        with tempfile.TemporaryDirectory(prefix="bp-dio-", dir=TEMP) as tmp:
            out = Path(tmp)
            (out / "configuration.h").write_text(
                "#define BP_ENABLE_DIO_SUPPORT\n" if case != "disabled" else "",
                encoding="ascii")
            (out / "base.h").write_text(BASE + pins + "\n", encoding="ascii")
            (out / "binary_io.h").write_text(BINARY, encoding="ascii")
            for name in ("dio.c", "dio.h"):
                (out / name).write_text(source("protocol_dio/" + name), encoding="utf-8")
            flags = ["gcc", "-std=gnu99", "-O1", "-Wall", "-Wextra", "-Werror", "-DBUSPIRATE" + board]
            if case == "disabled":
                obj = out / "dio.o"
                subprocess.run([*flags, "-c", str(out / "dio.c"), "-o", str(obj)], check=True)
                symbols = subprocess.check_output(["nm", str(obj)], text=True)
                assert not re.search(r"\b_?dio_\w+\b", symbols)
                print("PASS DIO disabled: entire source compiles without DIO symbols", flush=True)
            else:
                test = CLEANUP if case == "cleanup" else ROUTING
                (out / "test.c").write_text(test + PARK_SUPPORT, encoding="ascii")
                exe = out / "test.exe"
                subprocess.run([*flags, str(out / "dio.c"), str(out / "test.c"),
                                "-o", str(exe)], check=True)
                subprocess.run([str(exe)], check=True)
            print("PASS", board, case, flush=True)


if __name__ == "__main__":
    main()
