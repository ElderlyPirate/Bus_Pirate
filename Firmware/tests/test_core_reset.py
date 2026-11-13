"""Hardware-free checks of the production reset and SMPS stop bodies.

Register fixtures verify state/order, not PIC24 timing or electrical HiZ.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

from test_core_system import function
from test_core_system import source as grouped_source

ROOT = Path(__file__).resolve().parents[2]
TEMP = Path.home() / "AppData/Local/Temp/opencode"


def source(path, revision):
    if revision:
        return subprocess.check_output(
            ["git", "show", revision + ":" + path], cwd=ROOT,
            text=True, encoding="utf-8")
    return (ROOT / path).read_text(encoding="utf-8")


PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define INPUT 1
#define LOW 0
#define OFF 0
#define NO 0
#define BP_HIZ 0
#define NULL_IO 0
typedef union {
 uint16_t word;
 struct { unsigned short D0:1,D1:1,D2:1,D3:1,D4:1,D5:1,D6:1,D7:1,
                         D8:1,D9:1,D10:1,D11:1,D12:1,D13:1,D14:1,D15:1; } bits;
} port_t;
static port_t direction, latch;
#define BP_MOSI_DIR direction.bits.D1
#define BP_CLK_DIR direction.bits.D2
#define BP_MISO_DIR direction.bits.D3
#define BP_CS_DIR direction.bits.D4
#define BP_AUX0_DIR direction.bits.D5
static unsigned aux1_route, aux2_route, aux0_route;
static unsigned stop_calls, adc_setups, eeprom_setups, aux_resets, expected_ssrc;
static unsigned pullups, supplies, mode_led, usb_led;
static struct { unsigned numbits,int16,write_with_read,command_error,
 high_impedance,speed,periodicService,alternate_aux,little_endian; } mode_configuration;
static struct { unsigned bus_mode; } bus_pirate_configuration;
static struct { unsigned ADON,ASAM,SSRC; } AD1CON1bits;
static struct { unsigned AD1IE; } IEC0bits;
#if defined(BUSPIRATEV4) && defined(BP_ENABLE_SMPS_SUPPORT)
static struct { unsigned AD1IF; } IFS0bits;
#endif
static uint16_t AD1CSSL,AD1CON2,AD1CON3,OC5R,OC5RS,OC5CON1,OC5CON2;
static void bp_disable_mode_led(void) { mode_led=0; }
static void bp_enable_usb_led(void) { usb_led=1; }
static void bp_disable_pullup(void) { pullups=0; }
static void bp_disable_voltage_regulator(void) { supplies=0; }
static void bp_aux_pin_reset_state(void) { aux0_route=0; ++aux_resets; }
#ifdef BUSPIRATEV4
#define BP_AUX1_DIR direction.bits.D8
#define BP_AUX2_DIR direction.bits.D0
#define BP_AUX1_LAT latch.bits.D8
#define BP_AUX2_LAT latch.bits.D0
#define BP_AUX1_RPOUT aux1_route
#define BP_AUX2_RPOUT aux2_route
#define BPV4_HWI2CPINS_SETUP() ((void)0)
#define BP_BUTTON_SETUP() ((void)0)
static void bp_disable_3v3_pullup(void) {}
static void eeprom_initialize(void) { ++eeprom_setups; }
#endif
static void bp_adc_pin_setup(void) {
 ++adc_setups;
#if defined(BUSPIRATEV4) && defined(BP_ENABLE_SMPS_SUPPORT)
 assert(stop_calls==1 && !IEC0bits.AD1IE && !AD1CON1bits.ADON && !AD1CON1bits.ASAM);
 assert(!OC5R && !OC5RS && !OC5CON1 && !OC5CON2);
#endif
}
'''

SMPS_WRAPPER = r'''
static void smps_stop(void) {
 assert(BP_AUX1_DIR==INPUT && BP_AUX2_DIR==INPUT);
 assert(AD1CON1bits.SSRC==expected_ssrc && !adc_setups);
 ++stop_calls;
 fixture_smps_stop();
}
'''

TEST = r'''
int main(void) {
 for (unsigned seed=0; seed<256; ++seed) {
  direction.word=(uint16_t)(seed*257u);
  latch.word=(uint16_t)((255u-seed)*257u);
  uint16_t old_dir=direction.word, old_lat=latch.word;
  aux0_route=aux1_route=aux2_route=22;
  stop_calls=adc_setups=eeprom_setups=aux_resets=0;
  AD1CON1bits.ADON=AD1CON1bits.ASAM=IEC0bits.AD1IE=1;
  AD1CON1bits.SSRC=expected_ssrc=3;
  OC5R=64; OC5RS=127; OC5CON1=0x1c06; OC5CON2=0x1f;
  mode_led=pullups=supplies=1; usb_led=0;
  memset(&mode_configuration,0xff,sizeof(mode_configuration));
  bus_pirate_configuration.bus_mode=7;
  bp_reset_board_state();
  unsigned released=0x3eu;
#ifdef BUSPIRATEV4
  released|=0x101u;
  assert(!aux1_route && !aux2_route);
  assert(!(latch.word&0x101u) && (latch.word&~0x101u)==(old_lat&~0x101u));
  assert(eeprom_setups==1);
#else
  assert(latch.word==old_lat && !eeprom_setups);
  assert(aux1_route==22 && aux2_route==22);
#endif
  assert(direction.word==(uint16_t)(old_dir|released));
  assert(!aux0_route && aux_resets==1 && adc_setups==1);
  assert(!pullups && !supplies && !mode_led && usb_led);
  assert(bus_pirate_configuration.bus_mode==BP_HIZ && mode_configuration.numbits==8);
  assert(!mode_configuration.int16 && !mode_configuration.write_with_read &&
         !mode_configuration.alternate_aux && !mode_configuration.little_endian);
  assert(AD1CON1bits.SSRC==7 && !AD1CSSL && AD1CON3==0x1f02 && !AD1CON2);
#if defined(BUSPIRATEV4) && defined(BP_ENABLE_SMPS_SUPPORT)
  assert(stop_calls==1);
#else
  assert(!stop_calls);
#endif
  stop_calls=adc_setups=0; expected_ssrc=7;
  bp_reset_board_state();
  assert(adc_setups==1 && aux_resets==2);
  assert(direction.word==(uint16_t)(old_dir|released));
#ifdef BUSPIRATEV4
  assert(eeprom_setups==2 && !aux1_route && !aux2_route && !(latch.word&0x101u));
#endif
#if defined(BUSPIRATEV4) && defined(BP_ENABLE_SMPS_SUPPORT)
  assert(stop_calls==1 && !IEC0bits.AD1IE && !AD1CON1bits.ADON && !AD1CON1bits.ASAM);
#endif
 }
 puts("PASS 256 states plus repeated reset: GPIO/latch/PPS release, sibling preservation, defaults and feature-gated SMPS/ADC ordering");
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    args = parser.parse_args()
    base = source("Firmware/core_system/base.c", args.revision)
    smps = grouped_source("smps.c", args.revision)
    body = function(base, "clear_mode_configuration") + "\n" + function(base, "bp_reset_board_state")
    for label, defines in (("v3", "#define BUSPIRATEV3\n"),
                           ("v4", "#define BUSPIRATEV4\n"),
                           ("v4-smps", "#define BUSPIRATEV4\n#define BP_ENABLE_SMPS_SUPPORT\n")):
        production_stop = ""
        if label == "v4-smps":
            production_stop = function(smps, "smps_stop").replace("void smps_stop(", "static void fixture_smps_stop(") + SMPS_WRAPPER
        with tempfile.TemporaryDirectory(prefix="bp-core-reset-", dir=TEMP) as tmp:
            c = Path(tmp) / "reset.c"
            exe = c.with_suffix(".exe")
            c.write_text(defines + PREFIX + production_stop + body + TEST, encoding="ascii")
            subprocess.run(["gcc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                            "-Wno-unused-function", str(c), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
        print("PASS", label, "production reset fixture", flush=True)

    menu = source("Firmware/core_system/proc_menu.c", args.revision)
    assert "bp_reset_board_state();" in function(menu, "reset_terminal_state")
    assert function(menu, "changemode").count("bp_reset_board_state();") == 2
    assert "bp_reset_board_state();" in function(menu, "bp_change_mode")
    assert re.search(r"case '#':.*?reset_terminal_state\(\);", menu, re.S)
    assert re.search(r"enter_binary_bitbang_mode\(\);\s*#ifdef BUSPIRATEV4\s*reset_terminal_state\(\);", menu)
    print("PASS source reachability: m1/mode changes, scripted MODE, # and binary return share board reset")


if __name__ == "__main__":
    main()
