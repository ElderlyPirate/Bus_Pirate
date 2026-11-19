"""Terminal raw-BRG policy and real confirmation/rollback control flow.

Host serial/timer mocks verify logic, not a physical V3 UART baud transition.
BRG=0 is hardware-legal; the supported terminal raw-input range starts at 1.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

from test_core_system import function

ROOT = Path(__file__).resolve().parents[2]
TEMP = Path.home() / "AppData/Local/Temp/opencode"

PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NO 0
#define YES 1
#define CMDLENMSK 255
#define BP_COMMAND_BUFFER_SIZE 256
#define BELL 7
static char cmdbuf[256],output[2048];
static unsigned cmdstart,cmdend;
static bool setup_defaults_mode;
static struct { unsigned command_error; } mode_configuration;
static struct { uint8_t terminal_speed; } bus_pirate_configuration;
static uint16_t U1BRG;
static const uint16_t speeds[]={13332,3332,1666,832,416,207,103,68,34};
static const char *input;
static unsigned input_pos,serial_reads,writes,initialisations,flushes,timer_reads,delays;
static uint16_t written[4];
static uint16_t timer_value(void) { ++timer_reads;return (uint16_t)(timer_reads*1000u); }
#define TMR1 timer_value()
static void bp_delay_ms(unsigned ms) { assert(ms==1);++delays; }
static void bp_write_string(const char *s) { assert(strlen(output)+strlen(s)<sizeof(output));strcat(output,s); }
static void bp_write_line(const char *s) { bp_write_string(s);bp_write_string("\r\n"); }
static void user_serial_transmit_character(char c) { char s[]={c,0};bp_write_string(s); }
static void bp_write_dec_byte(uint8_t v) { (void)v; }
static bool user_serial_ready_to_read(void) { return input[input_pos]!=0; }
static uint8_t user_serial_read_byte(void) { assert(input[input_pos]);++serial_reads;return input[input_pos++]; }
static void user_serial_wait_transmission_done(void) { ++flushes; }
static void user_serial_set_baud_rate(uint16_t value) {
 assert(value>=1 && value<=32767 && writes<4);
 written[writes++]=value;U1BRG=value;
}
static void user_serial_initialise(void) {
 ++initialisations;
 if(bus_pirate_configuration.terminal_speed!=9) U1BRG=speeds[bus_pirate_configuration.terminal_speed];
}
#define bpBR ((void)0)
'''

TEST = r'''
static void prepare(const char *command,const char *answers,unsigned old_speed,uint16_t old_brg) {
 memset(cmdbuf,0,sizeof(cmdbuf));strcpy(cmdbuf,command);cmdstart=0;cmdend=strlen(command);
 output[0]=0;input=answers;input_pos=serial_reads=writes=initialisations=flushes=timer_reads=delays=0;
 mode_configuration.command_error=0;bus_pirate_configuration.terminal_speed=old_speed;U1BRG=old_brg;
}
int main(void) {
#ifdef BUSPIRATEV4
 prepare("b10 0","",8,34);set_baud_rate();
 assert(!writes && !initialisations && !serial_reads && !timer_reads && !delays);
 assert(U1BRG==34 && bus_pirate_configuration.terminal_speed==8);
 assert(cmdstart==cmdend-1 && strstr(output,"USB CDC baud rate is host-controlled."));
 puts("PASS V4 CDC command remains host metadata; no UART/confirmation side effects");
#else
 const char *commands[]={"b10 0","b10","b10 1","b10 32767","b10 32768","b10 65535","b10 65536","b10 -1"};
 const char *answers[]={"0\r1\r ","0\r\r "," "," ","34\r ","34\r ","34\r ","34\r "};
 const unsigned expected[]={1,34,1,32767,34,34,34,34};
 for(unsigned i=0;i<sizeof(commands)/sizeof(commands[0]);++i) {
  prepare(commands[i],answers[i],8,34);set_baud_rate();
  assert(writes==1 && written[0]==expected[i] && U1BRG==expected[i]);
  assert(bus_pirate_configuration.terminal_speed==9 && !mode_configuration.command_error);
  assert(initialisations==1 && flushes==1 && !input[input_pos]);
  assert(!strstr(output,"timed out"));
 }
 for(unsigned old_custom=0;old_custom<2;++old_custom) {
  unsigned speed=old_custom ? 9 : 5;
  uint16_t previous=old_custom ? 19 : 207;
  prepare("b10 1","?",speed,previous);set_baud_rate();
  assert(writes==2 && written[0]==1 && written[1]==previous);
  assert(U1BRG==previous && bus_pirate_configuration.terminal_speed==speed);
  assert(initialisations==2 && flushes==1 && serial_reads==1);
  assert(strstr(output,"Baud confirmation timed out; previous rate restored."));
#ifdef BP_USE_HARDWARE_DELAY_TIMER
  assert(timer_reads>=10000 && timer_reads<=10002 && !delays);
#else
  assert(delays==5000 && !timer_reads);
#endif
 }
 puts("PASS V3 raw 1..32767 policy, zero/overflow prompts, confirmation and fixed/custom rollback");
#endif
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    args = parser.parse_args()
    if args.revision:
        menu = subprocess.check_output(["git", "show", args.revision + ":Firmware/core_system/proc_menu.c"],
                                       cwd=ROOT, text=True, encoding="utf-8")
    else:
        menu = (ROOT / "Firmware/core_system/proc_menu.c").read_text(encoding="utf-8")
    body = "\n".join(function(menu, name) for name in ("consumewhitechars", "getint", "getnumber", "wait_baud_confirmation", "set_baud_rate"))
    messages = "\n".join("#define " + name + " ((void)0)" for name in sorted(set(re.findall(r"\b(?:BPMSG\d+|MSG_[A-Z_]+)\b", body))))
    for label, defines in (("v3-timer", "#define BUSPIRATEV3\n#define BP_USE_HARDWARE_DELAY_TIMER\n"),
                           ("v3-software-wait", "#define BUSPIRATEV3\n"),
                           ("v4", "#define BUSPIRATEV4\n")):
        with tempfile.TemporaryDirectory(prefix="bp-core-baud-", dir=TEMP) as tmp:
            c = Path(tmp) / "baud.c"
            exe = c.with_suffix(".exe")
            c.write_text(defines + PREFIX + messages + "\n" + body + TEST, encoding="ascii")
            subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
                            "-Wno-unused-variable", "-Wno-implicit-fallthrough", str(c), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
        print("PASS", label, "terminal baud control", flush=True)


if __name__ == "__main__":
    main()
