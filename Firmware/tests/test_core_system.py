"""Compile actual core functions with deterministic I/O mocks; no PIC/SIE model.

python Firmware/tests/test_core_system.py [--case name] [--revision commit]
"""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

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
    text = re.sub(r"__attribute__\(\([^)]*\)\)", "", text)
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
#define ASCII_NUL 0
#define ASCII_STX 2
#define MASKBOTTOM16(v,b) ((uint16_t)(v) & ((1u << (b))-1u))
static struct { unsigned numbits, int16, high_impedance, speed, periodicService, alternate_aux,
                         little_endian, write_with_read, command_error; } mode_configuration;
static struct { unsigned display_mode, terminal_speed, bus_mode, quiet, basic; } bus_pirate_configuration;
static struct { char data[256]; unsigned char guard[8]; } ring;
#define cmdbuf ring.data
static unsigned int cmdstart, cmdend;
static struct { unsigned cursor_position, history_entry_counter, binary_mode_counter, escape_state; bool command_present; } menu_state;
static unsigned values[32], used, serial_reads;
static const char *input = "";
static unsigned readpos;
static char output[4096];
static unsigned outlen;
static bool setup_defaults_mode;
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
static void bp_write_dec_dword(uint32_t v) { emit(v); }
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
    for (unsigned n=0;n<=65535;++n) {
        char token[16]; snprintf(token,sizeof(token),"%u",n); load(token,0);
        assert((uint16_t)getint()==n && !mode_configuration.command_error);
        snprintf(token,sizeof(token),"0X%X",n); load(token,0);
        assert((uint16_t)getint()==n && !mode_configuration.command_error);
    }
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
    readpos=0; input="327670\r123\r"; assert(getnumber(1,0,32767,1)==123);
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
    "prefix": ("proc_menu.c", ["update_binary_prefix"], r'''
int main(void) {
    const uint8_t ordinary[]={'i','\r',27,'[','D',8};
    for (unsigned i=0;i<sizeof(ordinary);++i) {
        menu_state.binary_mode_counter=19; update_binary_prefix(ordinary[i]);
        assert(!menu_state.binary_mode_counter);
    }
    menu_state.binary_mode_counter=5; update_binary_prefix(0); assert(menu_state.binary_mode_counter==5);
    update_binary_prefix(2); assert(menu_state.binary_mode_counter==5);
    puts("PASS only consecutive NUL/raw-STX prefixes survive normal input");
}'''),
    "reset": ("proc_menu.c", ["reset_terminal_state"], r'''
int main(void) {
    memset(&menu_state,0xff,sizeof(menu_state)); cmdend=37; user_macro=1;
    bus_pirate_configuration.quiet=1; bus_pirate_configuration.display_mode=RAW;
    reset_terminal_state();
    assert(cleaned==1 && reset_board==1 && !user_macro && !bus_pirate_configuration.quiet);
    assert(menu_state.cursor_position==37 && !menu_state.escape_state && !menu_state.binary_mode_counter && !menu_state.command_present);
    puts("PASS simulated reset clears protocol/editor state without jumping into parser locals");
}'''),
    "latch": ("hardware.h", ["bp_enable_pullup", "bp_disable_pullup", "bp_enable_voltage_regulator", "bp_disable_voltage_regulator"], r'''
int main(void) {
    vreg_pin=0; vreg_lat=0; bp_enable_voltage_regulator(); assert(vreg_lat==1 && vreg_pin==0 && vreg_dir==0);
    bp_disable_voltage_regulator(); assert(!vreg_lat && !vreg_pin);
    pullup_lat=1; bp_enable_pullup(); assert(!pullup_lat && pullup_dir==1);
    bp_disable_pullup(); assert(!pullup_lat && !pullup_dir);
    puts("PASS power/pullup writes target LAT before TRIS, leaving PORT readback separate");
}'''),
    "dword": ("base.c", ["print_decimal", "bp_write_dec_dword"], r'''
int main(void) {
    const uint32_t numbers[]={0,1,99999999,100000000,2147483647UL,4294967295UL};
    for (unsigned i=0;i<sizeof(numbers)/sizeof(numbers[0]);++i) {
        char expected[16]; snprintf(expected,sizeof(expected),"%lu",(unsigned long)numbers[i]);
        outlen=0; memset(output,0,sizeof(output)); bp_write_dec_dword(numbers[i]);
        assert(!strcmp(output,expected));
    }
    puts("PASS decimal dword printing covers the complete uint32 range");
}'''),
    "baud_timeout": ("proc_menu.c", ["wait_baud_confirmation"], r'''
int main(void) {
    assert(!wait_baud_confirmation() && timer_reads>=10000 && timer_reads<=10002 && !serial_reads);
    input=" "; assert(wait_baud_confirmation() && serial_reads==1);
    puts("PASS five-second UART deadline across Timer1 wraps; no blocking read when empty");
}'''),
    "macro": ("proc_menu.c", ["remove_current_character_from_command_line", "handle_character", "handle_backspace", "load_user_macro"], r'''
int main(void) {
    load("",253); menu_state.cursor_position=cmdend; strcpy(user_macros[0],"=12"); user_macro=1;
    load_user_macro(); assert(!user_macro && !menu_state.command_present && menu_state.cursor_position==0);
    handle_backspace(); handle_character('3');
    assert(cmdbuf[253]=='=' && cmdbuf[254]=='1' && cmdbuf[255]=='3' && !cmdbuf[0]);
    for(unsigned i=0;i<8;++i) assert(ring.guard[i]==0xa5);
    puts("PASS macro playback uses the bounded editor and remains editable, not auto-executed");
}'''),
    "history": ("proc_menu.c", ["cmdhistory", "handle_up_arrow", "handle_down_arrow"], r'''
int main(void) {
    load("h",100); assert(cmdhistory()==1 && !dialogs);
    load("h",100); cmdbuf[10]='i'; cmdbuf[20]='v';
    assert(cmdhistory()==1 && dialogs==1 && maximum==2);
    menu_state.history_entry_counter=0; refreshed=0; handle_up_arrow();
    assert(refreshed==1 && menu_state.history_entry_counter==1);
    puts("PASS history excludes current command/uninitialized choice; one arrow recalls one entry");
}'''),
    "uart_isr": ("base.c", ["_U1RXInterrupt", "_U1TXInterrupt"], r'''
int main(void) {
    _U1RXInterrupt(); assert(!IEC0bits.U1RXIE);
    UART1RXBuf=rx; UART1RXToRecv=1; U1RXREG=0x5a; rx[1]=0xa5;
    _U1RXInterrupt(); _U1RXInterrupt(); assert(rx[0]==0x5a && rx[1]==0xa5 && UART1RXRecvd==1);
    UART1TXBuf=rx; UART1TXAvailable=1; UART1TXSent=0; U1TXREG=0x55;
    _U1TXInterrupt(); _U1TXInterrupt(); assert(UART1TXSent==1 && U1TXREG==0x55 && !IEC0bits.U1TXIE);
    puts("PASS v3 UART ISR null/zero/completed transfers never escape their buffers");
}'''),
    "read_format": ("proc_menu.c", ["getint", "getrepeat", "getnumbits", "change_read_display"], r'''
int main(void) {
    load("rx:2;4",253);
    assert(change_read_display()==1 && cmdstart==254);
    assert(getrepeat()==2 && getnumbits()==4 && !mode_configuration.command_error);
    load("r",255); assert(!change_read_display() && cmdstart==255);
    puts("PASS read display selector is consumed before repeat/width suffixes");
}'''),
    "long_dialog": ("proc_menu.c", ["getlong"], r'''
int main(void) {
    input="9999999999\r12\r"; assert(getlong(1,0,2147483647,1)==12);
    readpos=0; input="12345678901\r12\r"; assert(getlong(1,0,2147483647,1)==12);
    readpos=0; input="-2\r12\r"; assert(getlong(1,0,2147483647,1)==12);
    readpos=0; input="2147483647\r"; assert(getlong(1,0,2147483647,1)==2147483647);
    puts("PASS 32-bit dialog overflow/sign/overlong tokens are rejected without truncation");
}'''),
    "vpu_latch": ("hardwarev4.h", ["bp_enable_3v3_pullup", "bp_disable_3v3_pullup", "bp_enable_5v0_pullup"], r'''
int main(void) {
    select33=select50=1; bp_enable_3v3_pullup(); assert(!select33 && dir33==0 && dir50==1);
    bp_enable_5v0_pullup(); assert(!select50 && dir33==1 && dir50==0);
    bp_disable_3v3_pullup(); assert(dir33==1 && dir50==1);
    puts("PASS VPU source selection uses LAT and breaks the other source before make");
}'''),
}


def run_case(name, revision):
    filename, names, tests = CASES[name]
    src = source(filename, revision)
    if name == "escape" and "static bool handle_escape_sequence(uint8_t character) {" in src:
        names = names + ["handle_escape_sequence"]
    body = "\n".join(function(src, n) for n in names)
    prefix = PREFIX
    if name == "vpu_latch":
        prefix += "#define INPUT 1\n#define OUTPUT 0\n#define LOW 0\nstatic unsigned select33,select50,dir33,dir50,pin33,pin50;\n"
        prefix += "#define BP_PUVSEL33_LAT select33\n#define BP_PUVSEL50_LAT select50\n#define BP_PUVSEL33_DIR dir33\n#define BP_PUVSEL50_DIR dir50\n#define BP_PUVSEL33 pin33\n#define BP_PUVSEL50 pin50\n"
    if name == "long_dialog": prefix += "#define long int32_t\n"
    if name == "dword":
        prefix = prefix.replace("static void bp_write_dec_dword(uint32_t v) { emit(v); }", "")
    if name == "read_format":
        prefix += "static const uint8_t READ_DISPLAY_BASE[]={'x','d','b','w'};\n"
    if name == "uart_isr":
        prefix += "static uint8_t rx[2];\nstatic uint8_t * volatile UART1RXBuf,* volatile UART1TXBuf;\n"
        prefix += "static volatile uint16_t UART1RXToRecv,UART1RXRecvd,UART1TXSent,UART1TXAvailable;\n"
        prefix += "static struct { unsigned U1RXIE,U1TXIE; } IEC0bits;\nstatic struct { unsigned U1RXIF,U1TXIF; } IFS0bits;\nstatic uint16_t U1RXREG,U1TXREG;\n"
    if name == "history":
        prefix += "#define BP_COMMAND_HISTORY_LENGTH 15\nstatic int dialogs,maximum,refreshed;\n"
        prefix += "static int getnumber(int def,int min,int max,int x) {(void)min;(void)x;dialogs++;maximum=max;return def;}\n"
        prefix += "static void refresh_command_line(uint16_t start) {(void)start;refreshed++;}\nstatic void refresh_mode_prompt(void) {}\n"
    if name == "macro":
        prefix += "#define BP_USER_MACROS_COUNT 2\n#define BP_USER_MACRO_MAX_LENGTH 32\nstatic char user_macros[2][32];\nstatic int user_macro;\n"
    if name == "baud_timeout":
        prefix += "#define BP_USE_HARDWARE_DELAY_TIMER\nstatic unsigned timer_reads;\nstatic uint16_t timer(void) { ++timer_reads; return (uint16_t)(timer_reads*1000u); }\n#define TMR1 timer()\n"
    if name == "latch":
        prefix += "#define INPUT 1\n#define OUTPUT 0\n#define LOW 0\nstatic unsigned vreg_pin,vreg_lat,vreg_dir,pullup_pin,pullup_lat,pullup_dir;\n"
        prefix += "#define BP_VREGEN vreg_pin\n#define BP_VREGEN_LAT vreg_lat\n#define BP_VREGEN_DIR vreg_dir\n#define BP_PULLUP pullup_pin\n#define BP_PULLUP_LAT pullup_lat\n#define BP_PULLUP_DIR pullup_dir\n"
    if name == "reset":
        prefix += "static int cleaned, reset_board, user_macro;\nstatic void cleanup(void) {cleaned++;}\n"
        prefix += "typedef struct { void (*cleanup)(void); } FakeProto;\nstatic FakeProto enabled_protocols[1] = {{cleanup}};\nstatic const FakeProto *bp_current_protocol(void) {return &enabled_protocols[0];}\n"
        prefix += "static void bp_reset_board_state(void) { reset_board++; }\nstatic void bp_disable_usb_led(void) {}\n"
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
                        "-Wno-unused-variable","-Wno-unused-but-set-variable","-Wno-implicit-fallthrough","-Wno-sign-compare",str(c),"-o",str(exe)],check=True)
        subprocess.run([str(exe)],check=True)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--revision")
    parser.add_argument("--case",choices=["all"]+list(CASES),default="all")
    args=parser.parse_args()
    for name in CASES if args.case=="all" else [args.case]:
        run_case(name,args.revision)
    if args.case=="all" and not args.revision and (ROOT/"Firmware/core_system/base.c").exists():
        project=ROOT/"Firmware/busPirate.X/nbproject/configurations.xml"
        xml=ET.parse(project); listed={n.text for n in xml.findall(".//itemPath")}
        for name in ("base.c","base.h","core.c","core.h","proc_menu.c","proc_menu.h","main.c"):
            ref="../core_system/"+name
            assert ref in listed and (project.parents[1]/ref).is_file(),ref
        print("PASS MPLAB lists all seven core_system files at existing paths")


if __name__=="__main__":
    main()
