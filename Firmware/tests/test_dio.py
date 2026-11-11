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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    parser.add_argument("--case", choices=("disabled", "routing"))
    args = parser.parse_args()

    def source(path):
        if args.revision:
            return subprocess.check_output(
                ["git", "show", args.revision + ":Firmware/" + path],
                cwd=ROOT, text=True, encoding="utf-8")
        return (ROOT / path).read_text(encoding="utf-8")

    for case in ((args.case,) if args.case else ("disabled", "routing")):
        with tempfile.TemporaryDirectory(prefix="bp-dio-", dir=TEMP) as tmp:
            out = Path(tmp)
            (out / "configuration.h").write_text(
                "#define BP_ENABLE_DIO_SUPPORT\n" if case != "disabled" else "",
                encoding="ascii")
            (out / "base.h").write_text(BASE, encoding="ascii")
            (out / "binary_io.h").write_text(BINARY, encoding="ascii")
            for name in ("dio.c", "dio.h"):
                (out / name).write_text(source("protocol_dio/" + name), encoding="utf-8")
            flags = ["gcc", "-std=gnu99", "-O1", "-Wall", "-Wextra", "-Werror"]
            if case == "disabled":
                obj = out / "dio.o"
                subprocess.run([*flags, "-c", str(out / "dio.c"), "-o", str(obj)], check=True)
                symbols = subprocess.check_output(["nm", str(obj)], text=True)
                assert not re.search(r"\b_?dio_\w+\b", symbols)
                print("PASS DIO disabled: entire source compiles without DIO symbols", flush=True)
            else:
                (out / "test.c").write_text(ROUTING, encoding="ascii")
                exe = out / "test.exe"
                subprocess.run([*flags, str(out / "dio.c"), str(out / "test.c"),
                                "-o", str(exe)], check=True)
                subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
