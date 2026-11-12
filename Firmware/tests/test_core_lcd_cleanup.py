"""Core LCD cleanup dispatch/park checks, not LCD-family or electrical tests.

As with the DIO fixture, normalize weak linkage for MinGW PE. Actual weak vs
strong replacement and descriptor pointers require the separate XC16 proof.
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
typedef struct { unsigned numbits,int16,command_error; } mode_configuration_t;
extern mode_configuration_t mode_configuration;
extern bus_pirate_configuration_t bus_pirate_configuration;
extern uint16_t direction,latch;
extern unsigned spi_enabled,route_mosi,route_clk,spi_calls,override_calls,regulator;
#define PARK_MASK (MOSI | CLK | MISO | CS)
#define IODIR direction
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
#include "hd44780.h"
mode_configuration_t mode_configuration;
bus_pirate_configuration_t bus_pirate_configuration;
uint16_t direction,latch;
unsigned spi_enabled,route_mosi,route_clk,spi_calls,override_calls,regulator;
#ifdef BP_ENABLE_HD44780_SUPPORT
static unsigned pullups,vpu;
#endif
void spi_disable_interface(void) {
 assert((direction & PARK_MASK)==PARK_MASK && regulator);
 spi_enabled=route_mosi=route_clk=0;++spi_calls;
}
void bp_disable_voltage_regulator(void) {
#ifdef BP_ENABLE_HD44780_SUPPORT
 assert(bus_pirate_configuration.bus_mode==BP_HD44780);
#ifdef STRONG_HOOK
 assert(override_calls);
#else
 assert((direction & PARK_MASK)==PARK_MASK && spi_calls);
 assert(!spi_enabled && !route_mosi && !route_clk);
 assert(mode_configuration.numbits==8 && !mode_configuration.int16);
#endif
#endif
 regulator=0;
}
#ifdef BP_ENABLE_HD44780_SUPPORT
void LCDstart(void) {}
void LCDstop(void) {}
uint16_t LCDwrite(uint16_t value) { return value; }
void LCDmacro(uint16_t value) { (void)value; }
void LCDsetup(void) {}
void LCDsetup_exc(void) {}
void LCDpins(void) {}
#endif
@@POWER_OFF@@
int main(void) {
#ifdef BP_ENABLE_HD44780_SUPPORT
 for(unsigned seed=0;seed<256;++seed) {
  direction=(uint16_t)(seed*257u);latch=(uint16_t)((255u-seed)*257u);
  uint16_t old_dir=direction,old_lat=latch;
  spi_enabled=route_mosi=route_clk=regulator=pullups=vpu=1;
  spi_calls=override_calls=0;
  mode_configuration.numbits=16;mode_configuration.int16=1;
  bus_pirate_configuration.bus_mode=BP_HD44780;
  assert(!strcmp(bp_current_protocol()->name,"LCD"));
  assert(bp_current_protocol()->cleanup==hd44780_cleanup);
  bp_current_protocol()->cleanup();
  assert(regulator && pullups && vpu && latch==old_lat);
  assert(bus_pirate_configuration.bus_mode==BP_HD44780);
#ifdef STRONG_HOOK
  assert(override_calls==1 && !spi_calls && direction==old_dir && spi_enabled);
#else
  assert(direction==(uint16_t)(old_dir | PARK_MASK));
  assert(spi_calls==1 && !spi_enabled && !route_mosi && !route_clk);
  assert(mode_configuration.numbits==8 && !mode_configuration.int16);
#endif
  switch_psu_off();
  assert(!regulator && bus_pirate_configuration.bus_mode==BP_HD44780);
 }
 puts("PASS 256 LCD histories: callback dispatch, release-before-SPI-disable, sibling/latch/policy preservation, width and w ordering");
#else
 assert(ENABLED_PROTOCOLS_COUNT==1 && !strcmp(bp_current_protocol()->name,"HiZ"));
 puts("PASS LCD disabled: core table links without any LCD symbol");
#endif
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    args = parser.parse_args()

    def source(path):
        if args.revision:
            return subprocess.check_output(["git", "show", args.revision + ":Firmware/" + path],
                                           cwd=ROOT, text=True, encoding="utf-8")
        return (ROOT / path).read_text(encoding="utf-8")

    power_off = function(source("core_system/proc_menu.c"), "switch_psu_off")
    for board in ("V3", "V4"):
        pins = "\n".join(re.findall(r"^#define (?:MOSI|CLK|MISO|CS) .*", source("hardware" + board.lower() + ".h"), re.M))
        for enabled, strong in ((False, False), (True, False), (True, True)):
            with tempfile.TemporaryDirectory(prefix="bp-core-lcd-", dir=TEMP) as tmp:
                out = Path(tmp)
                (out / "configuration.h").write_text("#define BP_ENABLE_HD44780_SUPPORT\n" if enabled else "", encoding="ascii")
                (out / "base.h").write_text(BASE + pins + "\n", encoding="ascii")
                (out / "bitbang.h").write_text("", encoding="ascii")
                (out / "hd44780.h").write_text("#include <stdint.h>\nvoid LCDstart(void);\nvoid LCDstop(void);\nuint16_t LCDwrite(uint16_t);\nvoid LCDmacro(uint16_t);\nvoid LCDsetup(void);\nvoid LCDsetup_exc(void);\nvoid LCDpins(void);\nvoid spi_disable_interface(void);\n", encoding="ascii")
                for name in ("core.c", "core.h"):
                    text = source("core_system/" + name)
                    if name == "core.c":
                        declaration = "void __attribute__((weak)) hd44780_cleanup(void)"
                        assert text.count(declaration)==1
                        text = text.replace(declaration, "void " + ("fixture_fallback" if strong else "hd44780_cleanup") + "(void)")
                    (out / name).write_text(text, encoding="utf-8")
                (out / "test.c").write_text(TEST.replace("@@POWER_OFF@@", power_off), encoding="ascii")
                inputs = [str(out / "core.c"), str(out / "test.c")]
                if strong:
                    hook = out / "strong.c"
                    hook.write_text('#include "core.h"\nextern unsigned override_calls;\nvoid hd44780_cleanup(void) { ++override_calls; }\n', encoding="ascii")
                    inputs.append(str(hook))
                exe = out / "test.exe"
                subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
                                "-DBUSPIRATE" + board, *(["-DSTRONG_HOOK"] if strong else []), *inputs, "-o", str(exe)], check=True)
                subprocess.run([str(exe)], check=True)
                if not enabled:
                    symbols = subprocess.check_output(["nm", str(exe)], text=True)
                    assert not re.search(r"\b_?(?:hd44780_cleanup|LCD\w*)\b", symbols)
            print("PASS", board, "LCD", "strong override" if strong else "enabled fallback" if enabled else "disabled", flush=True)


if __name__ == "__main__":
    main()
