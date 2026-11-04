"""Compile actual core functions with deterministic I/O mocks; no PIC/SIE model.

python Firmware/tests/test_core_system.py [--case name] [--revision commit]
"""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def source(name, revision=None):
    if revision:
        for directory in ("core_system/", ""):
            r = subprocess.run(["git", "show", revision + ":Firmware/" + directory + name], cwd=ROOT,
                               stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
            if r.returncode == 0:
                return r.stdout
        raise RuntimeError(name + " absent at " + revision)
    path = ROOT / "Firmware/core_system" / name
    return (path if path.exists() else ROOT / "Firmware" / name).read_text()


def function(text, name):
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    m = re.search(r"^(?:static |inline )*(?:void|int|long|bool|uint\d+_t)\s+" + name + r"\([^;]*?\)\s*\{", text, re.M)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[m.start():end]


PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#define YES 1
#define NO 0
#define ON 1
#define OFF 0
#define BUSPIRATEV4
#define HEX 0
#define DEC 1
#define BIN 2
#define RAW 3
#define BP_HIZ 0
#define BP_COMMAND_BUFFER_SIZE 256
#define CMDLENMSK 255
#define BELL 7
#define MASKBOTTOM16(v,b) ((uint16_t)(v) & ((1u << (b))-1u))
static struct { unsigned numbits, int16, high_impedance, speed, periodicService, alternate_aux,
                         little_endian, write_with_read, command_error; } mode_configuration;
static struct { unsigned display_mode, terminal_speed, bus_mode; } bus_pirate_configuration;
static struct { char data[256]; unsigned char guard[8]; } ring;
#define cmdbuf ring.data
static unsigned int cmdstart, cmdend;
static struct { unsigned cursor_position, history_entry_counter, binary_mode_counter, escape_state; } menu_state;
static unsigned values[32], used, serial_reads;
static const char *input = "";
static unsigned readpos;
static char output[4096];
static unsigned outlen;
static void user_serial_transmit_character(char c) { output[outlen++] = c; values[used++ % 32] = (uint8_t)c; }
static uint8_t user_serial_read_byte(void) { assert(input[readpos]); ++serial_reads; return input[readpos++]; }
static bool user_serial_ready_to_read(void) { return input[readpos] != 0; }
static void user_serial_wait_transmission_done(void) {}
static void user_serial_initialise(void) {}
static void user_serial_set_baud_rate(uint16_t v) { (void)v; }
static void emit(unsigned v) { values[used++ % 32] = v; }
static void bp_write_hex_word(uint16_t v) { emit(v); }
static void bp_write_hex_byte(uint8_t v) { emit(v); }
static void bp_write_dec_word(uint16_t v) { emit(v); }
static void bp_write_dec_byte(uint8_t v) { emit(v); }
static void bp_write_bin_byte(uint8_t v) { emit(v); }
static void bp_write_string(const char *s) { while (*s) { assert(outlen < sizeof(output)-1); output[outlen++]=*s++; } }
static void bp_write_line(const char *s) { bp_write_string(s); bp_write_string("\n"); }
#define bpSP ((void)0)
#define bpBR ((void)0)
static void load(const char *s, unsigned start) {
    memset(&ring, 0, sizeof(ring)); memset(ring.guard, 0xa5, sizeof(ring.guard));
    cmdstart = start; cmdend = (start + strlen(s)) & CMDLENMSK;
    for (unsigned i=0; s[i]; ++i) cmdbuf[(start+i)&CMDLENMSK]=s[i];
    mode_configuration.command_error=0; used=serial_reads=readpos=outlen=0;
}
static int getnumber(int def, int min, int max, int x);
static int getint(void);
static void consumewhitechars(void) { while (cmdbuf[cmdstart]==' ') cmdstart=(cmdstart+1)&CMDLENMSK; }
'''

CASES = {
    "format": ("base.c", ["bp_write_formatted_integer"], r'''
int main(void) {
    for (unsigned bits=1; bits<=16; ++bits) for (unsigned fmt=0; fmt<4; ++fmt) {
        mode_configuration.numbits=bits; mode_configuration.int16=bits>8;
        bus_pirate_configuration.display_mode=fmt; used=0;
        bp_write_formatted_integer(0xffff);
        unsigned mask=(1u<<bits)-1u;
        if (fmt<2 || bits<=8) assert(values[0]==mask);
        else assert(values[0]==(mask>>8) && values[1]==(mask&255));
    }
    puts("PASS 64 width/format combinations use the masked value");
}'''),
    "state": ("base.c", ["clear_mode_configuration"], r'''
int main(void) {
    memset(&mode_configuration, 0xff, sizeof(mode_configuration));
    clear_mode_configuration();
    assert(mode_configuration.numbits==8 && !mode_configuration.int16 && !mode_configuration.write_with_read);
    assert(!mode_configuration.command_error && !mode_configuration.little_endian && !mode_configuration.periodicService);
    puts("PASS complete mode-state reset precedes protocol setup");
}'''),
    "ring": ("proc_menu.c", ["remove_current_character_from_command_line", "handle_delete", "handle_character"], r'''
int main(void) {
    for (unsigned start=0; start<256; ++start) {
        load("ab",start); menu_state.cursor_position=start;
        handle_character('x');
        assert(cmdbuf[start]=='x' && cmdbuf[(start+1)&255]=='a' && cmdbuf[(start+2)&255]=='b');
        for (unsigned i=0;i<8;++i) assert(ring.guard[i]==0xa5);
        menu_state.cursor_position=start; handle_delete();
        assert(cmdbuf[start]=='a' && cmdbuf[(start+1)&255]=='b' && !cmdbuf[(start+2)&255]);
        menu_state.cursor_position=cmdend; unsigned end=cmdend; handle_delete(); assert(cmdend==end);
    }
    puts("PASS insertion/deletion at every ring seam; first/end delete bounds and guards");
}'''),
    "parse": ("proc_menu.c", ["getint", "getrepeat", "getnumbits"], r'''
int main(void) {
    const char *tokens[]={"65535","0xFFFF","0XAB","0b10101","0123"};
    const unsigned expected[]={65535,65535,171,21,123};
    for (unsigned i=0;i<5;++i) for(unsigned start=0;start<256;++start) {
        load(tokens[i],start); char saved[256]; memcpy(saved,cmdbuf,256);
        assert((uint16_t)getint()==expected[i] && !mode_configuration.command_error);
        assert(!memcmp(saved,cmdbuf,256));
    }
    const char *bad[]={"65536","0x10000","0b11111111111111111","0x","0b"};
    for(unsigned i=0;i<5;++i) { load(bad[i],253); getint(); assert(mode_configuration.command_error); }
    load("r:0",0); assert(getrepeat()==0 && mode_configuration.command_error);
    load("r:65535",0); assert(getrepeat()==0 && mode_configuration.command_error);
    load("r;17",0); assert(getnumbits()==0 && mode_configuration.command_error);
    load("r;0",0); assert(getnumbits()==0 && mode_configuration.command_error);
    puts("PASS nonmutating bounded decimal/hex/binary parsing, leading zeroes, count/width validation");
}'''),
    "dialog": ("proc_menu.c", ["getnumber"], r'''
int main(void) {
    input="-2\r2\r"; assert(getnumber(1,1,10,1)==2);
    readpos=0; input="32767\r"; assert(getnumber(1,0,32767,1)==32767);
    puts("PASS signed dialog range checked after sign; full int16 positive range accepted");
}'''),
    "baud": ("proc_menu.c", ["getint", "set_baud_rate"], r'''
static int getnumber(int def,int min,int max,int x) {(void)min;(void)max;(void)x;return def;}
int main(void) {
    load("b9",0); set_baud_rate(); assert(!serial_reads);
    puts("PASS virtual CDC baud command never waits for a space");
}'''),
    "flash": ("base.c", ["bp_read_from_flash"], r'''
int main(void) { TBLPAG=5; assert(bp_read_from_flash(2,0x100)==0x1234 && TBLPAG==5);
    puts("PASS flash low-word read excludes upper program byte and restores TBLPAG"); }
'''),
    "escape": ("proc_menu.c", ["handle_escape_key"], r'''
int main(void) {
    handle_escape_key(); assert(!serial_reads);
#ifdef TEST_ESCAPE_FSM
    assert(!handle_escape_sequence('i') && !menu_state.escape_state);
    handle_escape_key(); assert(handle_escape_sequence('['));
    assert(handle_escape_sequence('D') && keys[0]==1 && !menu_state.escape_state);
    handle_escape_key(); assert(handle_escape_sequence('[')); assert(handle_escape_sequence('1'));
    assert(!handle_escape_sequence('i') && !menu_state.escape_state && !serial_reads);
#endif
    puts("PASS lone/truncated ESC never blocks; complete sequences and normal input survive");
}'''),
}


def run_case(name, revision):
    filename, names, tests = CASES[name]
    src = source(filename, revision)
    if name == "escape" and "static bool handle_escape_sequence(uint8_t character) {" in src:
        names = names + ["handle_escape_sequence"]
    body = "\n".join(function(src, n) for n in names)
    prefix = PREFIX
    if name == "escape":
        prefix += "static unsigned keys[7];\n"
        for i, key in enumerate(("left_arrow", "right_arrow", "up_arrow", "down_arrow", "home_key", "end_key", "delete")):
            prefix += "static void handle_" + key + "(void) { keys[" + str(i) + "]++; }\n"
        if len(names) > 1: prefix += "#define TEST_ESCAPE_FSM\n"
    if name == "flash":
        prefix += "static uint16_t TBLPAG;\n#define __builtin_tblrdh(a) ((void)(a),0xab)\n#define __builtin_tblrdl(a) ((void)(a),0x1234)\n"
    # Host mocks deliberately include only the selected production routines.
    defines = "\n".join("#define " + n + " ((void)0)" for n in sorted(set(re.findall(r"\b(?:BPMSG\d+|MSG_[A-Z_]+)\b", body))))
    if "static int getint(void);" in prefix and "getint" in names:
        body = re.sub(r"^int getint", "static int getint", body, flags=re.M)
    if "getnumber" in names:
        body = re.sub(r"^int getnumber", "static int getnumber", body, flags=re.M)
    tmpbase = Path(tempfile.gettempdir()) / "opencode"
    tmpbase.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="core-test-", dir=tmpbase) as tmp:
        c=Path(tmp)/"test.c"; exe=c.with_suffix(".exe")
        c.write_text(prefix+defines+"\n"+body+tests, encoding="ascii")
        subprocess.run(["gcc","-std=gnu99","-Wall","-Wextra","-Werror","-Wno-unused-function",
                        "-Wno-unused-variable","-Wno-unused-but-set-variable","-Wno-implicit-fallthrough",str(c),"-o",str(exe)],check=True)
        subprocess.run([str(exe)],check=True)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--revision")
    parser.add_argument("--case",choices=["all"]+list(CASES),default="all")
    args=parser.parse_args()
    for name in CASES if args.case=="all" else [args.case]:
        run_case(name,args.revision)


if __name__=="__main__":
    main()
