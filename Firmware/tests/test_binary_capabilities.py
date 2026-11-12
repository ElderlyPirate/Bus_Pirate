"""Production capability functions with deterministic host register mocks.

Uses the existing native-function fixture convention; this is not an electrical
or timing simulator. XC16 builds/disassembly and the bench battery cover those.
"""
import argparse
import re
import shutil
from pathlib import Path
import subprocess
import tempfile

from test_core_system import function

ROOT = Path(__file__).resolve().parents[2]
TEMP = Path.home() / "AppData/Local/Temp/opencode"


def source(path, revision):
    if revision:
        return subprocess.check_output(
            ["git", "show", revision + ":Firmware/" + path], cwd=ROOT,
            text=True, encoding="utf-8")
    return (ROOT / "Firmware" / path).read_text(encoding="utf-8")


COMMON = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define ON 1
#define OFF 0
#define YES 1
#define NO 0
#define INPUT 1
#define OUTPUT 0
#define HIGH 1
#define LOW 0
#define BUSPIRATEV4
#define BP_ENABLE_SMPS_SUPPORT
'''

ADC = r'''
static struct { unsigned AD1IE; } IEC0bits;
static struct { unsigned high_impedance; } mode_configuration;
static unsigned enables,disables,reads,feeds,delays,outlen;
static unsigned char output[16];
#define BP_ADC_PROBE 4
#define BP_ADC_VPU 12
static void bp_enable_adc(void) { ++enables; }
static void bp_disable_adc(void) { ++disables; }
static uint16_t bp_read_adc(uint16_t c) { (void)c; ++reads;return 512; }
static void bp_disable_3v3_pullup(void) { ++feeds; }
static void bp_enable_3v3_pullup(void) { ++feeds; }
static void bp_enable_5v0_pullup(void) { ++feeds; }
static void bp_delay_ms(unsigned n) { delays+=n; }
static void user_serial_transmit_character(unsigned char c) { assert(outlen<16);output[outlen++]=c; }
static bool user_serial_ready_to_read(void) { return true; }
static uint8_t user_serial_read_byte(void) { return 0; }
'''

ADC_TEST = r'''
int main(void) {
 for(unsigned duty=0;duty<2;++duty) for(unsigned adon=0;adon<2;++adon) {
  /* Ownership is AD1IE, not OC duty or ADON. Both fault/zero-duty states
     must refuse every path without touching ADC or pull-up state. */
  (void)duty;(void)adon;IEC0bits.AD1IE=1;
  enables=disables=reads=feeds=delays=outlen=0;
  mode_configuration.high_impedance=YES;
  assert(!bp_binary_io_pullup_control(0x51));
  assert(!enables && !disables && !reads && !feeds && !delays);
  handle_read_adc_one_shot();
  assert(outlen==2 && output[0]==255 && output[1]==255);
  outlen=0;handle_read_adc_continuously();
  assert(outlen==2 && output[0]==255 && output[1]==255);
  outlen=0;assert(!read_and_transmit_adc_measurement());
  assert(outlen==2 && output[0]==255 && output[1]==255);
  assert(!enables && !disables && !reads && IEC0bits.AD1IE);
 }
 IEC0bits.AD1IE=0;outlen=0;handle_read_adc_one_shot();
 assert(outlen==2 && output[0]==2 && output[1]==0);
 assert(enables==1 && disables==1 && reads==1);
 outlen=0;handle_read_adc_continuously();
 assert(outlen==2 && output[0]==2 && output[1]==0);
 assert(enables==2 && disables==2 && reads==2);
 puts("PASS binary ADC reservation/refusal and available one-shot/stream paths");
}
'''

SELFTEST = r'''
static uint8_t errors;
static struct { bool quiet; } bus_pirate_configuration;
static unsigned pins[32],IODIR,IOLAT,terminal_refusals;
static bool bp_adc_check_foreground(void) { ++terminal_refusals;return false; }
static void bp_write_voltage(unsigned n) { (void)n; }
static void bp_write_dec_byte(unsigned n) { (void)n; }
#define bpSP ((void)0)
#define bpBR ((void)0)
#define ALLIO 15
#define V5L 700
#define V5H 850
#define V33L 460
#define V33H 563
#define eeprom_test() true
#define bp_enable_pullup() (++feeds)
#define bp_disable_pullup() (++feeds)
#define bp_enable_voltage_regulator() (++feeds)
#define bp_disable_voltage_regulator() (++feeds)
#define bp_enable_mode_led() (++feeds)
#define bp_disable_mode_led() (++feeds)
#define bp_enable_usb_led() (++feeds)
#define bp_disable_usb_led() (++feeds)
#define bp_disable_5v0_pullup() (++feeds)
#define bp_reset_board_state() (++feeds)
#define PIN_STATE_TEST_DELAY 100
#define PWR_STATE_TEST_DELAY 2
#define MSG_ANY_KEY_TO_EXIT_PROMPT ((void)0)
'''

SELFTEST_TEST = r'''
int main(void) {
 (void)mode_configuration;(void)user_serial_ready_to_read;
 IEC0bits.AD1IE=1;
 for(unsigned progress=0;progress<2;++progress) {
  assert(perform_selftest(progress,true)==1);
  assert(!enables && !disables && !reads && !feeds && !delays && !outlen);
  assert(!bus_pirate_configuration.quiet && IEC0bits.AD1IE);
  assert(!IODIR && !IOLAT);
  for(unsigned i=0;i<32;++i) assert(!pins[i]);
 }
 assert(terminal_refusals==1);
 perform_adc_test(BP_ADC_PROBE,0,1024);
 assert(errors==1 && !reads && !outlen && IEC0bits.AD1IE);
 IEC0bits.AD1IE=0;perform_adc_test(BP_ADC_PROBE,460,563);
 assert(errors==1 && reads==1 && outlen==2);
 puts("PASS self-test refusal precedes rails/pins/ADC/reset and ADC leaf is guarded");
}
'''

PWM = r'''
static struct { unsigned TCKPS1,TCKPS0,TON; } T2CONbits;
static struct { uint16_t pwm_frequency,pwm_duty_cycle; unsigned mode; } state;
static uint16_t OC5CON,OC5R,OC5RS,PR2,AUXPIN_RPOUT;
#define AUX_MODE_IO 0
#define AUX_MODE_PWM 2
#define OC5_IO 22
#define PWM_DIVISOR_PRESCALER_1_256 62
#define PWM_DIVISOR_PRESCALER_1_64 250
#define PWM_DIVISOR_PRESCALER_1_8 2000
#define PWM_DIVISOR_PRESCALER_1_1 16000
static void stop_timers(void) { memset(&T2CONbits,0,sizeof(T2CONbits)); }
void bp_update_pwm(uint16_t frequency,uint16_t duty_cycle);
'''

PWM_TEST = r'''
int main(void) {
 unsigned checks=0;
 for(unsigned f=1;f<=4000;++f) for(unsigned duty=0;duty<=100;++duty) {
  bp_update_pwm(f,duty);
  unsigned pre=f<4?256:f<31?64:f<245?8:1;
  uint32_t ticks=16000000UL/pre/f;
  assert(PR2==ticks-1 && OC5R==(ticks*duty)/100 && OC5RS==OC5R);
  assert(T2CONbits.TON && AUXPIN_RPOUT==OC5_IO && state.mode==AUX_MODE_PWM);
  assert((T2CONbits.TCKPS1*2+T2CONbits.TCKPS0)==(f<4?3:f<31?2:f<245?1:0));
  ++checks;
 }
 bp_update_pwm(1000,50);assert(PR2==15999 && OC5R==8000);
 bp_update_duty_cycle(25);assert(PR2==15999 && OC5R==4000);
 bp_update_pwm(1,99);assert(PR2==62499 && OC5R==61875);
 bp_update_pwm(1000,65535);assert(OC5R==16000);
 bp_update_pwm(0,0);assert(!T2CONbits.TON && !AUXPIN_RPOUT && !state.mode);
 bp_update_duty_cycle(50);assert(!T2CONbits.TON && !AUXPIN_RPOUT);
 printf("PASS %u Hz/duty combinations, prescaler boundaries, FREQ 1000, DUTY and OFF\n",checks);
}
'''

SMPS_FRAME = r'''
#undef BP_ENABLE_SMPS_SUPPORT
typedef enum { SMPS_COMMAND_GET_OUTPUT_VOLTAGE=0xf0,
 SMPS_COMMAND_STOP=0xf1,SMPS_COMMAND_START=0xf2 } smps_command;
static unsigned reads,writes;
static uint8_t user_serial_read_byte(void) { ++reads;return 0; }
#define REPORT_IO_FAILURE() (++writes)
'''

SMPS_FRAME_TEST = r'''
int main(void) {
 for(unsigned c=0xf0;c<=0xff;++c) {
  reads=writes=0;handle_smps_command((smps_command)c);
  assert(writes==1 && reads==(c>=0xf2));
 }
 puts("PASS all disabled SMPS opcodes consume exactly their declared operands");
}
'''

SMPS = r'''
static struct { unsigned AD1IE; } IEC0bits;
static struct { unsigned AD1IF; } IFS0bits;
static struct { unsigned ADON,ASAM; } AD1CON1bits;
static uint16_t OC5CON1,OC5CON2,OC5R,OC5RS,AD1CHS;
static unsigned BP_AUX1_DIR,BP_AUX1_LAT,BP_AUX1_RPOUT;
static volatile struct { uint16_t voltage_out,voltage_reading;uint8_t pwm_duty_cycle; } smps_state;
static unsigned result,buffer_reads,outlen;
static unsigned char output[2];
static uint16_t adc_buffer(void) { ++buffer_reads;return result; }
#define ADC1BUF0 adc_buffer()
#define OC5_IO 22
#define NULL_IO 0
#define BP_ADC_PROBE 4
static void user_serial_transmit_character(unsigned char c) {
 assert(outlen<2);output[outlen++]=c;smps_state.voltage_reading=0x2ab;
}
'''

SMPS_TEST = r'''
int main(void) {
 for(unsigned v=512;v<=4095;++v) {
  smps_start(v);
  assert(smps_state.voltage_out==(uint32_t)v*45/58);
  assert(smps_state.pwm_duty_cycle==128-64000/v);
  assert(BP_AUX1_DIR==OUTPUT && !BP_AUX1_LAT && BP_AUX1_RPOUT==OC5_IO);
  assert(IEC0bits.AD1IE && AD1CON1bits.ADON && AD1CON1bits.ASAM && !IFS0bits.AD1IF);
  result=smps_state.voltage_out+1;buffer_reads=0;_ADC1Interrupt();
  assert(!OC5R && buffer_reads==1 && IEC0bits.AD1IE);
  result=0;buffer_reads=0;_ADC1Interrupt();
  assert(OC5R==smps_state.pwm_duty_cycle && buffer_reads==1);
  for(unsigned repeat=0;repeat<2;++repeat) {
   smps_stop();
   assert(!IEC0bits.AD1IE && !AD1CON1bits.ADON && !AD1CON1bits.ASAM && !IFS0bits.AD1IF);
   assert(BP_AUX1_DIR==INPUT && !BP_AUX1_LAT && !BP_AUX1_RPOUT);
   assert(!OC5CON1 && !OC5CON2 && !OC5R && !OC5RS);
  }
 }
 smps_state.voltage_reading=0x123;smps_adc();
 assert(outlen==2 && output[0]==1 && output[1]==0x23);
 puts("PASS 3584 SMPS targets, zero-duty feedback ownership, single-buffer ISR reads and repeated STOP");
}
'''

LCD_CLEANUP = r'''
static unsigned SPIMOSI_TRIS,SPICLK_TRIS,SPIMISO_TRIS,SPICS_TRIS;
static struct { unsigned SPIEN; } SPI1STATbits;
static struct { unsigned SDI1R; } RPINR20bits;
static unsigned BP_MOSI_RPOUT,BP_CLK_RPOUT;
static struct { unsigned numbits,int16,selected_mode,high_impedance; } mode_configuration;
#define mode mode_configuration
static unsigned latch,psu,vpu,pullup;
#define NULL_IO 0
'''

LCD_CLEANUP_TEST = r'''
int main(void) {
 for(unsigned seed=0;seed<256;++seed) {
  SPIMOSI_TRIS=SPICLK_TRIS=SPIMISO_TRIS=SPICS_TRIS=0;
  SPI1STATbits.SPIEN=1;RPINR20bits.SDI1R=2;BP_MOSI_RPOUT=7;BP_CLK_RPOUT=8;
  mode.numbits=16;mode.int16=1;mode.selected_mode=seed;mode.high_impedance=seed&1;
  latch=seed;psu=seed&1;vpu=seed&3;pullup=(seed>>1)&1;
  for(unsigned repeat=0;repeat<2;++repeat) {
   hd44780_cleanup();
   assert(SPIMOSI_TRIS && SPICLK_TRIS && SPIMISO_TRIS && SPICS_TRIS);
   assert(!SPI1STATbits.SPIEN && RPINR20bits.SDI1R==31);
   assert(!BP_MOSI_RPOUT && !BP_CLK_RPOUT && mode.numbits==8 && !mode.int16);
   assert(mode.selected_mode==seed && mode.high_impedance==(seed&1));
   assert(latch==seed && psu==(seed&1) && vpu==(seed&3) && pullup==((seed>>1)&1));
  }
 }
 puts("PASS LCD strong cleanup releases pads/SPI, restores width and preserves mode/latches/power/VPU");
}
'''

LCD_SPI = r'''
static struct { unsigned SPIEN,SPIRBF; } SPI1STATbits;
static struct { unsigned SPI1IF; } IFS0bits;
static struct { unsigned command_error; } mode_configuration;
static unsigned counter,reads,cleanup;
static uint16_t buffer;
static uint16_t timer(void) { counter+=100;return counter; }
static uint16_t *spi_buffer(void) { ++reads;return &buffer; }
#define TMR1 timer()
#define SPI1BUF (*spi_buffer())
static void hd44780_cleanup(void) { ++cleanup;SPI1STATbits.SPIEN=0; }
'''

LCD_SPI_TEST = r'''
int main(void) {
 assert(!hd44780_spi_write_byte(0x96));
 assert(mode_configuration.command_error && !reads && !cleanup);
 mode_configuration.command_error=0;SPI1STATbits.SPIEN=1;
 SPI1STATbits.SPIRBF=1;IFS0bits.SPI1IF=1;
 assert(hd44780_spi_write_byte(0x96));
 assert(buffer==0x96 && reads==2 && !IFS0bits.SPI1IF && !cleanup);
 SPI1STATbits.SPIRBF=0;counter=0;reads=0;
 assert(!hd44780_spi_write_byte(0xa5));
 assert(counter==2100 && reads==1 && cleanup==1 && mode_configuration.command_error);
 puts("PASS LCD-local standard SPI byte completion, disabled interface and bounded timeout");
}
'''

G_PWM = r'''
static struct { unsigned command_error; } mode_configuration;
static unsigned cmdstart,args[2],arg_index;
static char cmdbuf[256];
#define CMDLENMSK 255
#define BPMSG1028 ((void)0)
#define BPMSG1029 ((void)0)
#define BPMSG1030 ((void)0)
#define BPMSG1033 ((void)0)
#define BPMSG1034 ((void)0)
static void consumewhitechars(void) {}
static unsigned getint(void) { assert(arg_index<2);return args[arg_index++]; }
static unsigned getnumber(int a,int b,int c,int d) { (void)a;(void)b;(void)c;(void)d;assert(0);return 0; }
'''

G_PWM_TEST = r'''
int main(void) {
 unsigned checks=0;
 for(unsigned f=1;f<=4000;++f) for(unsigned duty=0;duty<100;++duty) {
  state.mode=AUX_MODE_IO;cmdstart=0;cmdbuf[1]=' ';arg_index=0;
  args[0]=f;args[1]=duty;bp_pwm_setup();
  unsigned divisor=f<4?62:f<31?250:f<245?2000:16000;
  unsigned ticks=divisor/f;
  assert(PR2==ticks-1 && OC5R==ticks*duty/100 && OC5RS==OC5R);
  assert(T2CONbits.TON && state.mode==AUX_MODE_PWM && AUXPIN_RPOUT==OC5_IO);
  ++checks;
 }
 printf("PASS %u g-kHz/duty pairs, inline zero duty and inclusive 4MHz endpoint\n",checks);
}
'''

LCD_MACRO = r'''
static unsigned cmdstart,argument,prompt,reset,init,lines;
static struct { bool command_error; } mode_configuration;
#define CMDLENMSK 255
#define CMD_CLEARDISPLAY 1
#define CMD_SETDDRAMADDR 128
#define HD44780_COMMAND 0
#define HD44780_DATA 1
#define DISPLAYLINES1 0
#define DISPLAYLINES2 8
#define MSG_UNKNOWN_MACRO_ERROR ((void)0)
static void consumewhitechars(void) {}
static int getint(void) { return argument; }
static int getnumber(int a,int b,int c,int d) { (void)a;(void)b;(void)c;(void)d;++prompt;return 2; }
static void HD44780_Reset(void) { ++reset; }
static void HD44780_Init(unsigned char n) { ++init;lines=n; }
static void HD44780_WriteByte(unsigned char r,unsigned char b) { (void)r;(void)b; }
static void bp_delay_ms(unsigned n) { (void)n; }
static void user_serial_transmit_character(unsigned c) { (void)c; }
'''

LCD_MACRO_TEST = r'''
int main(void) {
 for(unsigned n=1;n<=2;++n) {
  reset=init=prompt=0;argument=n;LCDmacro(2);
  assert(reset==1 && init==1 && !prompt && lines==(n==1?0:8));
 }
 reset=init=prompt=0;argument=0;LCDmacro(2);
 assert(reset==1 && init==1 && prompt==1 && lines==8);
 reset=init=prompt=0;LCDmacro(1);assert(reset==1 && !init && !prompt);
 puts("PASS LCD macro (2) initializes inline and prompted line counts; reset-only remains unchanged");
}
'''


def cases(revision):
    binary = source("protocol_binary/binary_io.c", revision)
    base = source("core_system/base.c", revision)
    selftest = source("capability_selftest/selftest.c", revision)
    aux = source("capability_aux/aux_pin.c", revision)
    smps = source("capability_smps/smps.c", revision)
    lcd = source("capability_hd44780/hd44780.c", revision)
    pwm_macros = "\n".join("#define " + n + " 0" for n in sorted(set(
        re.findall(r"\b_OC5CON1_\w+_POSITION\b", aux)))) + "\n"
    selftest_macros = "\n".join("#define " + n + " ((void)0)" for n in
                                sorted(set(re.findall(r"\bBPMSG\d+\b", selftest))))
    pins = sorted(set(re.findall(r"\bBP_(?:AUX0|LEDMODE|PULLUP|VREGEN|EE_SCL|EE_SDA|EE_WP|MOSI|MISO|CLK|CS)(?:_LAT|_DIR)?\b", selftest)))
    selftest_macros += "\n" + "\n".join(
        "#define %s pins[%d]" % (n, i) for i, n in enumerate(pins))
    selftest_macros += "\n#define BP_ADC_USB 1\n#define BP_ADC_5V0 2\n#define BP_ADC_3V3 3\n"
    lcd_macros = "\n".join("#define " + n + " ((void)0)" for n in
                            sorted(set(re.findall(r"\bBPMSG\d+\b", lcd)))) + "\n"
    return {
        "adc": COMMON + ADC + function(base, "bp_adc_foreground_available") +
        "\n".join(function(binary, n) for n in (
            "bp_binary_io_pullup_control", "read_and_transmit_adc_measurement",
            "handle_read_adc_one_shot", "handle_read_adc_continuously")) + ADC_TEST,
        "selftest": COMMON + ADC + SELFTEST + selftest_macros + "\n" +
        function(base, "bp_adc_foreground_available") +
        "\n".join(function(selftest, n) for n in (
            "check_result", "perform_pins_state_test", "perform_adc_test",
            "perform_selftest")) + SELFTEST_TEST,
        "pwm": COMMON + PWM + pwm_macros + "\n".join(function(aux, n) for n in (
            "setup_prescaler_divisor", "bp_update_pwm", "bp_update_duty_cycle")) + PWM_TEST,
        "smps-frame": COMMON + SMPS_FRAME + function(binary, "handle_smps_command") + SMPS_FRAME_TEST,
        "smps": COMMON + SMPS + "\n".join(function(smps, n) for n in (
            "smps_start", "smps_stop", "_ADC1Interrupt", "smps_adc")) + SMPS_TEST,
        "lcd-cleanup": COMMON + LCD_CLEANUP + function(lcd, "hd44780_cleanup") + LCD_CLEANUP_TEST,
        "lcd-spi": COMMON + LCD_SPI + function(lcd, "hd44780_spi_write_byte") + LCD_SPI_TEST,
        "g-pwm": COMMON + PWM + G_PWM + pwm_macros +
        function(aux, "setup_prescaler_divisor") + function(aux, "bp_pwm_setup") + G_PWM_TEST,
        "lcd-macro": COMMON + LCD_MACRO + lcd_macros + function(lcd, "LCDmacro") + LCD_MACRO_TEST,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    parser.add_argument("--case")
    args = parser.parse_args()
    for name, code in cases(args.revision).items():
        if args.case and name != args.case:
            continue
        with tempfile.TemporaryDirectory(prefix="bp-capabilities-", dir=TEMP) as tmp:
            c = Path(tmp) / (name + ".c")
            exe = c.with_suffix(".exe")
            c.write_text(code, encoding="ascii")
            linker = (["-fuse-ld=bfd"] if not shutil.which("ld") and shutil.which("ld.bfd") else [])
            subprocess.run(["gcc", *linker, "-std=gnu99", "-Wall", "-Wextra", "-Werror",
                            str(c), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
