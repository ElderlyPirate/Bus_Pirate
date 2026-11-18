"""Focused #237 PIC setup-parser fold equivalence (no board I/O)."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build/pic-setup-tests"
out.mkdir(parents=True, exist_ok=True)
source = (root / "programmer_pic/pic.c").read_text()
start = source.index("void pic_setup_prepare(void) {")
depth = 0
for end in range(source.index("{", start), len(source)):
    depth += (source[end] == "{") - (source[end] == "}")
    if depth == 0:
        function = source[start:end + 1]
        break
assert function.count("getint_with_spaces()") == 2
legacy = function.replace("pic_setup_prepare", "legacy_setup").replace(
    "int mode = getint_with_spaces();", "consumewhitechars(); int mode = getint();"
).replace(
    "int delay = getint_with_spaces();", "consumewhitechars(); int delay = getint();"
)
parser = (root / "core_system/proc_menu.c").read_text()
helper = re.search(r"int getint_with_spaces\(void\)\s*\{[^}]+\}", parser)[0]
prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#define NO 0
#define YES 1
#define PIC_MODE_6BITS 0
#define MOSI 1
#define CLK 2
#define PIC_PIN_DELAY 100
static struct { int opcode_bits, delay; } mode_info;
static struct { int command_error, high_impedance, int16; } mode_configuration;
static int values[2], pos, events[32], count, defaults;
static void note(int x) { events[count++]=x; }
static void consumewhitechars(void) { note(defaults ? 10 : 11); }
static int getint(void) { note(20); return defaults ? 0 : values[pos++]; }
static int getnumber(int a,int b,int c,int d) { (void)b;(void)c;(void)d;note(30);return a; }
static void pic_reset_mode_state(void) { note(40); }
static void pic_set_display_width(void) { note(41); }
static void bp_write_dec_byte(int x) { note(100+x); }
static void bp_write_line(const char *s) { (void)s;note(42); }
static void bitbang_set_pins_low(int p,int d) { assert(p==3 && d==100);note(43); }
#define MSG_PIC_MODE_PROMPT note(50)
#define MSG_PIC_DELAY_PROMPT note(51)
#define MSG_PIC_MODE_HEADER note(52)
#define bpSP note(53)
'''
main = r'''
int main(void) {
  unsigned cases=0;
  for(defaults=0;defaults<2;defaults++)
    for(int m=-2;m<=4;m++) for(int d=-2;d<=5;d++) {
      values[0]=m;values[1]=d;pos=count=0;
      memset(&mode_info,0,sizeof mode_info);
      memset(&mode_configuration,0,sizeof mode_configuration);
      pic_setup_prepare();
      int saved[32],n=count,op=mode_info.opcode_bits,delay=mode_info.delay;
      int error=mode_configuration.command_error,hi=mode_configuration.high_impedance;
      memcpy(saved,events,sizeof saved);pos=count=0;
      memset(&mode_info,0,sizeof mode_info);
      memset(&mode_configuration,0,sizeof mode_configuration);
      legacy_setup();
      assert(n==count && !memcmp(saved,events,n*sizeof(int)));
      assert(op==mode_info.opcode_bits && delay==mode_info.delay);
      assert(error==mode_configuration.command_error && hi==mode_configuration.high_impedance);
      cases++;
    }
  printf("PASS PIC setup: %u inline/invalid/default cases, identical parser call order and state\n",cases);
}
'''
(out / "test.c").write_text(prefix + helper + function + legacy + main)
exe = out / "pic-setup.exe"
subprocess.run(["gcc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                str(out / "test.c"), "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)
