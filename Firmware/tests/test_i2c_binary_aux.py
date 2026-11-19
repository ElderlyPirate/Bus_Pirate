"""RA-11: extracted I2C 0x09 replies and unchanged terminal AUX helpers."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build/i2c-binary-aux-tests"
OUT.mkdir(parents=True, exist_ok=True)


def function(source, name):
    match = re.search(r"(?m)^[\w *]+\b" + name + r"\([^;]*?\)\s*\{", source)
    start = source.index("{", match.start())
    depth = 1
    end = start + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


i2c = (ROOT / "protocol_i2c/i2c.c").read_text()
aux = (ROOT / "aux_pin.c").read_text()
case = i2c[i2c.index("      case 9: // extended AUX command"):]
case = case[:case.index("      case 0b1111:")]
helpers = "\n".join(function(aux, name) for name in (
    "bp_aux_pin_set_low", "bp_aux_pin_set_high",
    "bp_aux_pin_set_high_impedance", "bp_aux_pin_read"))
source = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define LOW 0
#define HIGH 1
#define OUTPUT 0
#define INPUT 1
#define Nop() ((void)0)
#define MASKBOTTOM8(v,n) ((v)&((1u<<(n))-1))
static int BP_AUX0, BP_CS, BP_AUX0_DIR, BP_CS_DIR;
#ifndef BUSPIRATEV3
static int BP_AUX1, BP_AUX2, BP_AUX1_DIR, BP_AUX2_DIR;
#endif
static struct { unsigned alternate_aux; } mode_configuration;
static unsigned char reply[100], command;
static unsigned length, reads;
static void user_serial_transmit_character(unsigned char c) {
    assert(length < sizeof(reply)); reply[length++] = c;
}
static unsigned char user_serial_read_byte(void) { ++reads; return command; }
#define REPORT_IO_SUCCESS() user_serial_transmit_character(1)
static void text(const char *s) {
    while (*s) user_serial_transmit_character((unsigned char)*s++);
}
#define BPMSG1039 text("AUX INPUT/HI-Z")
#define BPMSG1040 text("AUX HIGH")
#define BPMSG1041 text("AUX LOW")
''' + helpers + r'''
static void dispatch(void) {
    unsigned char inByte, fr;
    switch (9) {
''' + case + r'''
    }
}
int main(void) {
    unsigned checks=0;
    for (unsigned selector=0; selector<2; ++selector)
    for (unsigned sub=0; sub<256; ++sub)
    for (unsigned level=0; level<2; ++level)
    for (unsigned dir=0; dir<2; ++dir) {
        mode_configuration.alternate_aux=selector;
        BP_AUX0=BP_CS=level; BP_AUX0_DIR=BP_CS_DIR=dir;
        command=sub; length=reads=0; dispatch();
        unsigned result=(sub<=2 || sub==0x10 || sub==0x20) ? 1 :
                        (sub==3 ? level : 0);
        assert(length==2 && reply[0]==1 && reply[1]==result && reads==1);
        assert(mode_configuration.alternate_aux==
               (sub==0x10 ? 0 : sub==0x20 ? 1 : selector));
        int value=selector ? BP_CS : BP_AUX0;
        int direction=selector ? BP_CS_DIR : BP_AUX0_DIR;
        assert(value==(sub<=1 ? (int)sub : (int)level));
        assert(direction==(sub<=1 ? OUTPUT : sub<=3 ? INPUT : (int)dir));
        assert((selector ? BP_AUX0 : BP_CS)==(int)level);
        assert((selector ? BP_AUX0_DIR : BP_CS_DIR)==(int)dir);
        ++checks;
    }
    for (unsigned selector=0; selector<2; ++selector) {
        mode_configuration.alternate_aux=selector;
        length=0; bp_aux_pin_set_low();
        assert(length==7 && !memcmp(reply,"AUX LOW",7));
        length=0; bp_aux_pin_set_high();
        assert(length==8 && !memcmp(reply,"AUX HIGH",8));
        length=0; bp_aux_pin_set_high_impedance();
        assert(length==14 && !memcmp(reply,"AUX INPUT/HI-Z",14));
    }
    printf("PASS: %u binary reply/pad cases; terminal AUX messages retained\n", checks);
}
'''
(OUT / "test.c").write_text(source, encoding="ascii")
for board in ("BUSPIRATEV4", "BUSPIRATEV3"):
    exe = OUT / (board + ".exe")
    subprocess.run(["gcc", "-std=c99", "-O1", "-Wall", "-Wextra", "-Werror",
                    "-D" + board, str(OUT / "test.c"), "-o", str(exe)], check=True)
    print(board, flush=True)
    subprocess.run([str(exe)], check=True)
