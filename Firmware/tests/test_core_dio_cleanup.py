"""Core DIO dispatch/park tests, not DIO-family or electrical tests.

MinGW PE weak static initializers do not model XC16 ELF weak linkage. Compile
the fallback as strong here (or rename it for the replacement dispatch test);
verify actual weak/strong resolution separately in XC16 target builds.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

from test_core_system import function

ROOT = Path(__file__).resolve().parents[1]
TEMP = Path.home() / "AppData/Local/Temp/opencode"

BASE = r'''
#ifndef FIXTURE_BASE_H
#define FIXTURE_BASE_H
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "core.h"
#define NO 0
#define OFF 0
#define LOW 0
#define INPUT 1
#define OUTPUT 0
typedef struct { unsigned numbits,int16,command_error; } mode_configuration_t;
extern mode_configuration_t mode_configuration;
extern bus_pirate_configuration_t bus_pirate_configuration;
extern uint16_t direction,latch;
extern unsigned pullups,vpu33,vpu50,regulator,override_calls;
#define PARK_MASK (AUX | MOSI | CLK | MISO | CS)
#define IODIR direction
uint16_t *fixture_latch(void);
#define IOLAT (*fixture_latch())
void bp_disable_pullup(void);
void bp_disable_3v3_pullup(void);
void bp_disable_voltage_regulator(void);
#define MSG_COMMAND_HAS_NO_EFFECT ((void)0)
#define MSG_SPI_PINS_STATE ((void)0)
#define BPMSG1088 ((void)0)
#define MSG_CLUTCH_DISENGAGED ((void)0)
#define MSG_POWER_SUPPLIES_OFF ((void)0)
#define bpBR ((void)0)
#endif
'''

TEST = r'''
#include <stdio.h>
#include <string.h>
#include "base.h"
mode_configuration_t mode_configuration;
bus_pirate_configuration_t bus_pirate_configuration;
uint16_t direction,latch;
unsigned pullups,vpu33,vpu50,regulator,override_calls;
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
void bp_disable_voltage_regulator(void) {
#ifdef BP_ENABLE_DIO_SUPPORT
 assert(bus_pirate_configuration.bus_mode==BP_DIO);
#ifdef STRONG_HOOK
 assert(override_calls);
#else
 assert((direction & PARK_MASK)==PARK_MASK && !(latch & PARK_MASK));
 assert(!pullups && mode_configuration.numbits==8 && !mode_configuration.int16);
#endif
#endif
 regulator=0;
}
#ifdef BP_ENABLE_DIO_SUPPORT
uint16_t dio_read(void) { return 0; }
uint16_t dio_write(uint16_t value) { return value; }
void dio_setup_prepare(void) {}
#endif
@@POWER_OFF@@
int main(void) {
#ifdef BP_ENABLE_DIO_SUPPORT
 for (unsigned seed=0;seed<256;++seed) {
  direction=(uint16_t)(seed*257u);
  latch=(uint16_t)((255u-seed)*257u);
  uint16_t old_dir=direction,old_lat=latch;
  pullups=vpu33=vpu50=regulator=1;override_calls=0;
  mode_configuration.numbits=16;mode_configuration.int16=1;
  bus_pirate_configuration.bus_mode=BP_DIO;
  assert(!strcmp(bp_current_protocol()->name,"DIO"));
  assert(bp_current_protocol()->cleanup==dio_cleanup);
  bp_current_protocol()->cleanup();
  assert(regulator && bus_pirate_configuration.bus_mode==BP_DIO);
#ifdef STRONG_HOOK
  assert(override_calls==1 && direction==old_dir && latch==old_lat && pullups);
#else
  assert(direction==(uint16_t)(old_dir | PARK_MASK));
  assert(latch==(uint16_t)(old_lat & ~PARK_MASK));
  assert(!pullups && mode_configuration.numbits==8 && !mode_configuration.int16);
#ifdef BUSPIRATEV4
  assert(!vpu33 && !vpu50);
#else
  assert(vpu33 && vpu50);
#endif
#endif
  switch_psu_off();
  assert(!regulator && bus_pirate_configuration.bus_mode==BP_DIO);
 }
 puts("PASS 256 states: DIO cleanup dispatch, parked siblings/order or strong override, w retains DIO then disables regulator");
#else
 assert(ENABLED_PROTOCOLS_COUNT==1 && !strcmp(bp_current_protocol()->name,"HiZ"));
 regulator=1;switch_psu_off();assert(regulator);
 puts("PASS DIO disabled: HiZ-only descriptor table links without any DIO symbol");
#endif
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--revision")
    args = p.parse_args()

    def source(path):
        if args.revision:
            return subprocess.check_output(["git", "show", args.revision + ":Firmware/" + path],
                                           cwd=ROOT, text=True, encoding="utf-8")
        return (ROOT / path).read_text(encoding="utf-8")

    menu = source("core_system/proc_menu.c")
    assert re.search(r"case 'w':\s*switch_psu_off\(\);", menu)
    power_off = function(menu, "switch_psu_off")
    for board in ("V3", "V4"):
        hardware = source("hardware" + board.lower() + ".h")
        pins = "\n".join(re.findall(r"^#define (?:AUX0?|MOSI|CLK|MISO|CS) .*", hardware, re.M))
        for enabled, strong in ((False, False), (True, False), (True, True)):
            with tempfile.TemporaryDirectory(prefix="bp-core-dio-", dir=TEMP) as tmp:
                out = Path(tmp)
                (out / "configuration.h").write_text("#define BP_ENABLE_DIO_SUPPORT\n" if enabled else "", encoding="ascii")
                (out / "base.h").write_text(BASE + pins + "\n", encoding="ascii")
                (out / "bitbang.h").write_text("", encoding="ascii")
                (out / "dio.h").write_text("#include <stdint.h>\nuint16_t dio_read(void);\nuint16_t dio_write(uint16_t);\nvoid dio_setup_prepare(void);\n", encoding="ascii")
                (out / "protocol_dio").mkdir()
                (out / "protocol_dio/dio.h").write_bytes((out / "dio.h").read_bytes())
                for name in ("core.c", "core.h"):
                    text = source("core_system/" + name)
                    if name == "core.c":
                        declaration = "void __attribute__((weak)) dio_cleanup(void)"
                        assert text.count(declaration) == 1
                        text = text.replace(declaration, "void " + ("fixture_fallback" if strong else "dio_cleanup") + "(void)")
                    (out / name).write_text(text, encoding="utf-8")
                (out / "test.c").write_text(TEST.replace("@@POWER_OFF@@", power_off), encoding="ascii")
                inputs = [str(out / "core.c"), str(out / "test.c")]
                if strong:
                    hook = out / "strong.c"
                    hook.write_text('#include "core.h"\nextern unsigned override_calls;\nvoid dio_cleanup(void) { ++override_calls; }\n', encoding="ascii")
                    inputs.append(str(hook))
                exe = out / "test.exe"
                subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
                                "-DBUSPIRATE" + board, *(["-DSTRONG_HOOK"] if strong else []),
                                *inputs, "-o", str(exe)], check=True)
                subprocess.run([str(exe)], check=True)
                if not enabled:
                    symbols = subprocess.check_output(["nm", str(exe)], text=True)
                    assert not re.search(r"\b_?dio_\w+\b", symbols)
            print("PASS", board, "DIO", "strong override" if strong else "enabled fallback" if enabled else "disabled", flush=True)


if __name__ == "__main__":
    main()
