"""Exhaustive output and parser checks against the available production APIs.

Supports the generic formatter, the #237 prerequisite, and the shared
implementation; scripted defaults are tested only when that feature exists.
"""
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"build/shared-format-tests"
OUT.mkdir(parents=True,exist_ok=True)


def function(source,name):
    match=re.search(r"(?m)^[\w *]+\b"+name+r"\([^;]*?\)\s*(?://[^\n]*)?\s*\{",source)
    if not match:raise RuntimeError("Missing function: "+name)
    start=source.index("{",match.start());depth=0
    for i in range(start,len(source)):
        if source[i]=="{":depth+=1
        elif source[i]=="}":
            depth-=1
            if not depth:return source[match.start():i+1]
    raise RuntimeError("Unterminated function: "+name)


base=(ROOT/"base.c").read_text()
menu=(ROOT/"proc_menu.c").read_text()
body="\n".join(function(base,x) for x in ("bp_write_dec_word","bp_write_dec_byte"))
if "print_decimal16(" in body:
    body=function(base,"print_decimal16")+"\n"+body
elif "print_decimal(" in body:
    body=function(base,"print_decimal")+"\n"+body
body+="\n"+"\n".join(function(menu,x) for x in ("getint","consumewhitechars","getint_with_spaces"))
has_defaults="setup_defaults_mode" in body
(OUT/"test.c").write_text(r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#define YES 1
#define CMDLENMSK 255
static unsigned char cmdbuf[256];
static unsigned cmdstart;
static int setup_defaults_mode;
static struct { int command_error; } mode_configuration;
static char output[32];
static unsigned length;
static void user_serial_transmit_character(char value) { assert(length<sizeof(output)); output[length++]=value; }
''' + "\n#define HAS_SETUP_DEFAULTS %d\n" % has_defaults + body + r'''
int main(void) {
    char expected[32];
    for (unsigned n=0; n<=65535; n++) {
        length=0; bp_write_dec_word((uint16_t)n);
        snprintf(expected,sizeof(expected),"%u",n);
        assert(length==strlen(expected) && !memcmp(output,expected,length));
        length=0; bp_write_dec_byte((uint8_t)n);
        snprintf(expected,sizeof(expected),"%u",n&255);
        assert(length==strlen(expected) && !memcmp(output,expected,length));
    }
    const char *cases[]={""," ","0","1","  1234"," 0xA5"," 0Xff"," 0b10101"," 0B01",
                         " 0012"," 0x"," 0b"," x"," -2"," \t12"," 77:3"," 0xABCDz"};
    unsigned checks=0;
    for (unsigned start=0; start<256; start+=8) {
        for (unsigned which=0; which<sizeof(cases)/sizeof(*cases); which++) {
            unsigned char initial[256],after[256];
            for (unsigned defaults=0; defaults<(HAS_SETUP_DEFAULTS ? 2u : 1u); defaults++) {
                memset(initial,0,sizeof(initial));
                for (unsigned i=0; cases[which][i]; i++) initial[(start+i)&CMDLENMSK]=cases[which][i];
                memcpy(cmdbuf,initial,sizeof(initial)); cmdstart=start;
                setup_defaults_mode=defaults; mode_configuration.command_error=0;
                consumewhitechars(); int value=getint();
                unsigned end=cmdstart; int error=mode_configuration.command_error;
                memcpy(after,cmdbuf,sizeof(after));
                memcpy(cmdbuf,initial,sizeof(initial)); cmdstart=start;
                setup_defaults_mode=defaults; mode_configuration.command_error=0;
                assert(getint_with_spaces()==value);
                assert(cmdstart==end && mode_configuration.command_error==error);
                assert(!memcmp(cmdbuf,after,sizeof(after)));
                checks++;
            }
        }
    }
    printf("Decimal formatter: 131072 outputs PASS; shared parser: %u differential cases PASS\n",checks);
}
''')
exe=OUT/"shared-format.exe"
subprocess.run(["gcc","-std=c99","-O1","-Wall","-Wextra","-Werror",str(OUT/"test.c"),"-o",str(exe)],check=True)
subprocess.run([str(exe)],check=True)
