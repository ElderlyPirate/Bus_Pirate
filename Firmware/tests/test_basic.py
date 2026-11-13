"""Run production BASIC tokenizer/interpreter with deterministic hardware stubs.

Like the core fixtures, this compiles the actual C, not a Python interpreter.
Native int is wider than XC16 int: target overflow/codegen still needs XC16.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
TEMP = Path.home() / "AppData/Local/Temp/opencode"

PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BP_ENABLE_BASIC_SUPPORT
#define BP_BASIC_PROGRAM_SPACE 1024
#define BP_BASIC_NESTED_FOR_LOOP_COUNT 4
#define BP_BASIC_STACK_FRAMES_DEPTH 10
#define YES 1
#define NO 0
#define ON 1
#define OFF 0
#define PWM_MINIMUM_FREQUENCY 0
#define PWM_MAXIMUM_FREQUENCY 4000
#define PWM_MINIMUM_DUTY_CYCLE 1
#define PWM_MAXIMUM_DUTY_CYCLE 100
#define BP_ADC_PROBE 1
#define CMDLENMSK 255
static char cmdbuf[256];
static unsigned int cmdstart,cmdend;
typedef struct { unsigned quiet,bus_mode,basic; } bus_pirate_configuration_t;
typedef struct { unsigned little_endian,numbits,alternate_aux; } mode_configuration_t;
bus_pirate_configuration_t bus_pirate_configuration;
mode_configuration_t mode_configuration;
static unsigned BP_PULLUP=1,BP_VREGEN,pwm_calls,pwm_frequency,pwm_duty,polls;
static char output[100000];
static size_t output_size;
static void emit(const char *s) {
 if (bus_pirate_configuration.quiet) return;
 assert(output_size+strlen(s)<sizeof(output));
 strcpy(output+output_size,s);output_size+=strlen(s);
}
static void bp_write_string(const char *s) { emit(s); }
static void bp_write_line(const char *s) { emit(s);emit("\r\n"); }
static void bp_write_dec_word(uint16_t n) { char s[16];snprintf(s,sizeof(s),"%u",n);emit(s); }
static void bp_write_hex_byte(uint8_t n) { char s[8];snprintf(s,sizeof(s),"0x%02X",n);emit(s); }
#define bpBR emit("\r\n")
#define bpSP emit(" ")
static void user_serial_transmit_character(char c) { char s[]={c,0};emit(s); }
static bool user_serial_ready_to_read(void) { assert(++polls<100000);return false; }
static char user_serial_read_byte(void) { return 0; }
static uint16_t getint(void) {
 unsigned n=0;while(cmdbuf[cmdstart]>='0' && cmdbuf[cmdstart]<='9') {
 n=n*10+cmdbuf[cmdstart]-'0';cmdstart=(cmdstart+1)&CMDLENMSK;
 } return n;
}
static void consumewhitechars(void) {
 while(cmdstart!=cmdend && (cmdbuf[cmdstart]==' ' || cmdbuf[cmdstart]=='\t'))
 cmdstart=(cmdstart+1)&CMDLENMSK;
}
static int getnumber(int a,int b,int c,int d) { (void)a;(void)b;(void)c;(void)d;return 42; }
static uint16_t bp_reverse_integer(uint16_t n,unsigned width) {
 uint16_t r=0;while(width--) { r=(r<<1)|(n&1);n>>=1; }return r;
}
static uint16_t bus_send(uint16_t n) { return n; }
static uint16_t bus_read(void) { return 128; }
static void nothing(void) {}
static void macro(unsigned n) { (void)n; }
static struct {
 const char *name;
 uint16_t (*send)(uint16_t),(*read)(void),(*data_state)(void),(*read_bit)(void);
 void (*start)(void),(*start_with_read)(void),(*stop)(void),(*stop_from_read)(void);
 void (*data_high)(void),(*data_low)(void),(*clock_low)(void),(*clock_high)(void),(*clock_pulse)(void);
 void (*run_macro)(unsigned);
} protocol={"HiZ",bus_send,bus_read,bus_read,bus_read,nothing,nothing,nothing,nothing,
 nothing,nothing,nothing,nothing,nothing,macro};
#define bp_current_protocol() (&protocol)
static bool bp_aux_pin_read(void) { return false; }
static void bp_aux_pin_set_high(void) {}
static void bp_aux_pin_set_low(void) {}
static void bp_set_voltage_regulator_state(bool n) { BP_VREGEN=n; }
static void bp_set_pullup_state(bool n) { BP_PULLUP=!n; }
static uint32_t bp_timer_microseconds(void) { return 64000; }
static void bp_enable_adc(void) {}
static void bp_disable_adc(void) {}
static uint16_t bp_read_adc(unsigned n) { (void)n;return 512; }
static void bp_update_pwm(unsigned f,unsigned d) { ++pwm_calls;pwm_frequency=f;pwm_duty=d; }
static void bp_update_duty_cycle(unsigned d) { ++pwm_calls;pwm_duty=d; }
static void bp_delay_ms(unsigned n) { (void)n; }
static int bp_change_mode(int n) {
 if(n<1 || n>10) return 1;
 bus_pirate_configuration.bus_mode=n-1;
 memset(&mode_configuration,0,sizeof(mode_configuration));return 0;
}
void bp_basic_initialize(void);
'''

TEST = r'''
static unsigned checks;
static void command(const char *text) {
 size_t n=strlen(text);assert(n<255);memcpy(cmdbuf,text,n+1);
 cmdstart=0;cmdend=n+1;bp_basic_enter_interactive_interpreter();
}
static void reset(void) {
 bus_pirate_configuration.quiet=0;output_size=0;output[0]=0;polls=0;
 pwm_calls=0;pwm_frequency=pwm_duty=0;bp_basic_initialize();
}
static void expect(bool ok,const char *name) {
 ++checks;if(!ok) { fprintf(stderr,"FAIL: %s; output=%s\n",name,output);exit(1); }
}
static bool output_is(const char *s) {
 size_t n=output_size;while(n && (output[n-1]=='\r' || output[n-1]=='\n')) --n;
 return strlen(s)==n && !memcmp(output,s,n);
}
static void pwm(void) {
 const char *forms[]={"10 FREQ 1000 50","10 FREQ 1000,50",
 "10 FREQ 500+500 25*2","10 FREQ (500+500),(25*2)","10 FREQ A B"};
 for(unsigned i=0;i<sizeof(forms)/sizeof(*forms);++i) {
  reset();command("1 LET A=1000");command("2 LET B=50");command(forms[i]);command("RUN");
  expect(pwm_calls==1 && pwm_frequency==1000 && pwm_duty==50,forms[i]);
 }
 reset();command("10 FREQ 1000 50");command("20 DUTY 25");command("RUN");
 expect(pwm_calls==2 && pwm_frequency==1000 && pwm_duty==25,"DUTY operand position");
 const char *bad[]={"10 FREQ 100050","10 FREQ","10 DUTY"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i) {
  reset();command(bad[i]);command("RUN");expect(!pwm_calls && strstr(output,"Error(3)"),bad[i]);
 }
}
static void pullup(void) {
 reset();command("10 PULLUP 0");command("20 LET A=PULLUP");command("RUN");
 expect(basic_variables[0]==0,"PULLUP off is logical zero");
 reset();command("10 PULLUP 1");command("20 LET A=PULLUP");command("RUN");
 expect(basic_variables[0]==1,"PULLUP on is logical one");
}
static void debug(void) {
 reset();command("10 PRINT 42");command("DEBUG");
 expect(strstr(output,"0xE3 0x00 0x0A 0x88 0x34 0x32 ")!=NULL,"DEBUG stored bytes");
 expect(output_size==BP_BASIC_PROGRAM_SPACE*5,"DEBUG dumps exactly the program area");
}
static void auxpin(void) {
#ifdef BUSPIRATEV4
 const unsigned max=3;
#else
 const unsigned max=1;
#endif
 for(unsigned pin=0;pin<=max;++pin) {
  char s[32];reset();snprintf(s,sizeof(s),"10 AUXPIN %u",pin);command(s);command("RUN");
  expect(mode_configuration.alternate_aux==pin && !strstr(output,"Error("),s);
 }
 const char *bad[]={"10 AUXPIN -1","10 AUXPIN 4","10 AUXPIN 256"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i) {
  reset();mode_configuration.alternate_aux=1;command(bad[i]);command("RUN");
  expect(mode_configuration.alternate_aux==1 && strstr(output,"Error(3)"),bad[i]);
 }
}
static void loops(void) {
 reset();command("10 FOR A=1 TO 1");command("20 NEXT A");command("30 NEXT A");command("RUN");
 expect(strstr(output,"Error(5)")!=NULL,"NEXT cannot reuse a completed loop");
 reset();command("10 FOR A=1 TO 2");command("20 FOR B=1 TO 2");
 command("30 FOR C=1 TO 2");command("40 FOR D=1 TO 2");command("50 PRINT 1;");
 command("60 NEXT D");command("70 NEXT C");command("80 NEXT B");command("90 NEXT A");command("RUN");
 expect(output_is("1111111111111111"),"four nested loops fit and execute 16 times");
 reset();command("10 FOR A=1 TO 1");command("20 FOR B=1 TO 1");
 command("30 FOR C=1 TO 1");command("40 FOR D=1 TO 1");command("50 FOR E=1 TO 1");command("RUN");
 expect(strstr(output,"Error(4)")!=NULL,"fifth nested loop fails before writing");
 reset();command("10 FOR A=-3 TO -1");command("20 PRINT A+4;");command("30 NEXT A");command("RUN");
 expect(output_is("123"),"signed negative loop bound");
 reset();command("10 NEXT 1");command("RUN");
 expect(strstr(output,"Error(5)")!=NULL,"invalid NEXT variable rejected");
 reset();command("10 NEXT A");command("RUN");
 expect(strstr(output,"Error(5)")!=NULL,"NEXT after RUN has no stale FOR state");
}
int main(int argc,char **argv) {
 assert(argc==2);
 if(!strcmp(argv[1],"pwm")) pwm();
 else if(!strcmp(argv[1],"pullup")) pullup();
 else if(!strcmp(argv[1],"debug")) debug();
 else if(!strcmp(argv[1],"auxpin")) auxpin();
 else if(!strcmp(argv[1],"loops")) loops();
 else { fprintf(stderr,"unknown test group\n");return 2; }
 printf("PASS %u production BASIC checks (%s)\n",checks,argv[1]);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    parser.add_argument("--group", default="pwm")
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    path = "Firmware/tool_basic/basic.c"
    text = (subprocess.check_output(["git", "show", f"{args.revision}:{path}"], cwd=ROOT).decode()
            if args.revision else (ROOT / path).read_text())
    text = re.sub(r'^#include "[^\n]+"\s*$', '', text, flags=re.M)
    messages = "\n".join(f'#define {m} emit("{m}")' for m in sorted(set(re.findall(r'BPMSG\d+', text))))
    messages += '\n#undef BPMSG1047\n#define BPMSG1047 emit("Error(")\n'
    messages += '#undef BPMSG1048\n#define BPMSG1048 emit(") @line:")\n'
    messages += '#undef BPMSG1049\n#define BPMSG1049 emit(" @pgmspace:")\n'
    with tempfile.TemporaryDirectory(prefix="bp-basic-", dir=TEMP) as tmp:
        c = Path(tmp) / "basic.c"
        exe = c.with_suffix(".exe")
        c.write_text(PREFIX + ("\n#define BP_BASIC_DEBUG_COMMAND\n" if args.debug else "")
                     + messages + text + TEST, encoding="ascii")
        for board in ("BUSPIRATEV4", "BUSPIRATEV3"):
            subprocess.run(["gcc", "-std=gnu99", "-O1", "-Wall", "-Wextra", "-Werror",
                            "-Wno-unused-function", f"-D{board}", str(c), "-o", str(exe)], check=True)
            subprocess.run([str(exe), args.group], check=True, timeout=15)


if __name__ == "__main__":
    main()
