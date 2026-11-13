"""Production UART setup/dispatch regressions with deterministic host input.

These native checks do not model PIC24 UART timing or replace the live batteries.
"""
import argparse
from pathlib import Path
import re
import subprocess

from test_core_system import function

ROOT = Path(__file__).resolve().parents[2]

PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define ON 1
#define OFF 0
#define YES 1
#define NO 0
#define FCY 16000000UL
#define UART2_9_N 3
#define BP_COMMAND_BUFFER_SIZE 256
#define CMDLENMSK 255
static struct { unsigned speed, command_error, numbits, int16; unsigned high_impedance:1; } mode_configuration;
static char cmdbuf[256];
static unsigned cmdstart, prompts, header;
static bool setup_defaults_mode;
static bool half_normal_prompts;
static unsigned header_hiz;
static uint16_t U2BRG;
static struct { unsigned UARTEN; } U2MODEbits;
static struct { unsigned OERR, UTXBF; } U2STAbits, U1STAbits;
static uint16_t U2TXREG, U1TXREG;
static const uint8_t *input;
static unsigned input_left, output_count;
static uint8_t output[128];
static unsigned setups;
static unsigned tx_calls, button_polls;
static unsigned baud_calls;
static bool mapped, single_wire;
static uint16_t configured_brg;
static void consumewhitechars(void) { while (cmdbuf[cmdstart]==' ') ++cmdstart; }
static int getnumber(int def, int min, int max, int exit) {
  (void)exit; ++prompts; assert(def>=min && def<=max);
  return half_normal_prompts && (prompts==5 || prompts==6) ? 2 : def;
}
static unsigned long getlong(unsigned long def, unsigned long min, unsigned long max, int exit) {
  (void)exit; ++prompts; assert(min>=62 && def>=min && def<=max); return def;
}
static void UARTsettings(void) { ++header; header_hiz=mode_configuration.high_impedance; }
static bool uart2_rx_ready(void) { return false; }
static bool getInReady(void) { return true; }
static uint8_t uart2_rx_pending(void) { return 0; }
static uint16_t uart2_rx(void) { assert(0); return 0; }
static void uart2_set_single_wire(bool b) { single_wire=b; }
static void uart2_setup(uint16_t b, bool o, bool i, uint8_t d, bool s) {
  (void)o; (void)i; (void)d; (void)s; ++setups; U2BRG=configured_brg=b; mapped=true;
}
static void uart2_enable(void) { U2MODEbits.UARTEN=1; }
static void uart2_disable(void) { mapped=single_wire=false; U2MODEbits.UARTEN=0; }
static void uart2_tx(uint16_t c) { (void)c; ++tx_calls; }
static uint16_t uart2_rx_overrun_count(void) { return 0; }
static void bp_disable_mode_led(void) {}
static uint32_t uart_get_baud_rate(bool quiet) { (void)quiet; ++baud_calls; return 9600; }
static bool agree(void) { return true; }
static unsigned BP_LEDMODE;
static uint16_t U1BRG;
static bool user_serial_ready_to_read(void) { assert(input_left); return true; }
static uint8_t user_serial_read_byte(void) { assert(input_left); --input_left; return *input++; }
static void user_serial_transmit_character(uint8_t c) { assert(output_count<sizeof(output)); output[output_count++]=c; }
static void bp_binary_io_peripherals_set(uint8_t c) { (void)c; }
static uint8_t bp_binary_io_pullup_control(uint8_t c) { (void)c; return 1; }
#define BP_BUTTON_ISDOWN() (++button_polls>1)
#define LOW 0
#define INPUT 1
#define OUTPUT 0
static unsigned FTDI_CTS_DIR,FTDI_RTS_DIR,BP_CS_DIR,BP_CLK_DIR,BP_CLK,FTDI_RTS,FTDI_CTS,BP_CS;
#define bpBR ((void)0)
#define REPORT_IO_SUCCESS() user_serial_transmit_character(1)
#define REPORT_IO_FAILURE() user_serial_transmit_character(0)
'''

SETUP_TEST = r'''
static void load(const char *s) {
  strcpy(cmdbuf,s); cmdstart=prompts=header=0;
  mode_configuration.command_error=0; U2BRG=123;
}
int main(void) {
#ifdef BUSPIRATEV4
  const char *invalid[]={"10 0 1 1 1 2 1", "10 1 1 1 1 2 1", "10 61 1 1 1 2 1"};
#else
  const char *invalid[]={"10 0 1 1 1 2 1", "10 32768 1 1 1 2 1"};
#endif
  for (unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {
    load(invalid[i]); uart_setup_prepare();
    assert(prompts && !header && mode_configuration.speed==0);
  }
  load("10 9600 1 1 1 2 1"); uart_setup_prepare();
  assert(!prompts && header==1);
#ifdef BUSPIRATEV4
  assert(U2BRG==416);
  load("10 65535 1 1 1 2 1");uart_setup_prepare();assert(!prompts && U2BRG==60);
#else
  assert(U2BRG==9600);
  load("10 416 1 1 1 2 1");uart_setup_prepare();assert(!prompts && U2BRG==416);
  load("10 1 1 1 1 2 1");uart_setup_prepare();assert(!prompts && U2BRG==1);
#endif
  load("5 1 1 1 2 1"); uart_setup_prepare();
  assert(!prompts && header==1 && mode_configuration.speed==4);
  load("5 1 1 1 2"); uart_setup_prepare();
  assert(!prompts && header==1 && !mode_configuration.command_error && !uart_settings.half_duplex);
  load("5 1 1 1 2 0"); uart_setup_prepare();assert(prompts && !header);
  load("5 1 1 1 2 65536"); uart_setup_prepare();assert(prompts && !header);
  load("5 1 1 2 1 2");uart_setup_prepare();assert(prompts && !header);
  load("5 1 1 2 1 1");uart_setup_prepare();
  assert(!prompts && header==1 && !uart_settings.half_duplex && uart_settings.receive_polarity);
  load("5 1 1 1 2 2");uart_setup_prepare();
  assert(!prompts && header==1 && uart_settings.half_duplex && mode_configuration.high_impedance && header_hiz);
  load("0");half_normal_prompts=true;uart_setup_prepare();half_normal_prompts=false;
  assert(prompts==6 && uart_settings.half_duplex && mode_configuration.high_impedance);
  load("5 1 1 1 2");uart_setup_prepare();
  assert(!uart_settings.half_duplex && !mode_configuration.high_impedance);
  puts("PASS setup: zero/missing-rate sentinel and unrepresentable baud refuse before divide; valid presets/custom unchanged");
}
'''

BINARY_TEST = r'''
int main(void) {
  for (unsigned code=0;code<16;++code) {
    uint8_t commands[]={0x60|code, 1, 0};
    input=commands;input_left=sizeof(commands);setups=output_count=0;
    configured_brg=0;binUART();
    assert(!input_left && output_count==1);
    if (code<sizeof(UART_BRG_SPEED)/sizeof(*UART_BRG_SPEED)) {
      assert(output[0]==1 && setups==2 && configured_brg==UART_BRG_SPEED[code]);
    } else {
      assert(output[0]==0 && setups==1 && configured_brg==UART_BRG_SPEED[0]);
    }
  }
  puts("PASS binary: all sixteen 0x6x operands; invalid codes NACK without reconfiguration; stream remains aligned");
}
'''

BRIDGE_TEST = r'''
int main(void) {
  const uint8_t c=0x55;input=&c;input_left=1;button_polls=tx_calls=0;
  uart_run_macro(1);
  assert(!input_left && tx_calls==1);
  puts("PASS terminal bridge uses the shared UART2 transmit path and button escape");
}
'''

COMMON_TEST = r'''
int main(void) {
  for(unsigned i=0;i<sizeof(UART_COMMON_BAUD_RATES)/sizeof(*UART_COMMON_BAUD_RATES);++i)
    assert(uart_get_closest_common_rate(UART_COMMON_BAUD_RATES[i])==UART_COMMON_BAUD_RATES[i]);
  assert(uart_get_closest_common_rate(150000)==128000);
  assert(uart_get_closest_common_rate(192000)==128000);
  assert(uart_get_closest_common_rate(192001)==256000);
  assert(uart_get_closest_common_rate(256001)==0);
  assert(uart_get_closest_common_rate(31250)==31250);
  assert(uart_get_closest_common_rate(31936)==31250);
  assert(uart_get_closest_common_rate(30025)==28800);
  assert(uart_get_closest_common_rate(30026)==31250);
  assert(uart_get_closest_common_rate(34825)==31250);
  assert(uart_get_closest_common_rate(34826)==38400);
  puts("PASS common-rate endpoints, MIDI measured estimate/tie boundaries, upper interval and out-of-range detection");
}
'''

RESTORE_TEST = r'''
int main(void) {
  mode_configuration.speed=4;uart_settings.half_duplex=1;
  uart_setup_execute();assert(mapped && single_wire);
  unsigned before=setups;uart_run_macro(4);
  assert(setups>before && mapped && single_wire && U2BRG==416);
  uart_settings.databits_parity=3;uart_setup_execute();
  assert(mode_configuration.numbits==9 && mode_configuration.int16);
  uart_settings.databits_parity=0;uart_setup_execute();
  assert(mode_configuration.numbits==8 && !mode_configuration.int16);
  for(unsigned hd=0;hd<2;++hd)for(unsigned enabled=0;enabled<2;++enabled) {
    uart_settings.half_duplex=hd;mode_configuration.speed=9;U2BRG=127;
    uart_setup_execute();if(!enabled)uart2_disable();
    before=setups;uart_run_macro(4);
    assert(U2MODEbits.UARTEN==enabled && mapped==!!enabled);
    assert(setups==before+enabled && U2BRG==127);
    assert(uart_settings.half_duplex==hd && (!enabled || single_wire==!!hd));
  }
#ifdef BUSPIRATEV4
  uart_settings.autobaud=1;baud_calls=0;uart_setup_execute();
  assert(baud_calls==1 && mapped && single_wire && U2BRG==416);
  uart_setup_execute();assert(baud_calls==1 && mapped && single_wire);
#endif
  puts("PASS macro-4 mapping/OD restore, HDUART selection, one-shot autobaud and 9/8-bit display width");
}
'''

DETECTION_PREFIX = r'''
#define HIGH 1
#define INPUT 1
#define _T4CON_T32_POSITION 3
static union { uint16_t value; struct { unsigned pad:15,TON:1; } bits; } t4, t5;
#define T4CON t4.value
#define T5CON t5.value
#define T4CONbits t4.bits
#define T5CONbits t5.bits
static struct { unsigned T4IE,T5IE; } IEC1bits;
static uint16_t TMR5HLD,TMR5,PR4,PR5,timer_low;
static unsigned BP_MOSI_DIR,BP_MISO_DIR;
static uint32_t clock_ticks,origin;
static unsigned waveform,cancel,reads,mosi_reads,miso_reads;
static bool rx_pin(bool mosi) {
  ++reads;if(mosi)++mosi_reads;else ++miso_reads;
  clock_ticks+=waveform==1?16:16000;
  return waveform==1 ? ((clock_ticks/1600)&1)^uart_settings.receive_polarity : waveform==2;
}
#define BP_MOSI rx_pin(true)
#define BP_MISO rx_pin(false)
static uint16_t *timer_read(void) {
  if(T4CONbits.TON) {timer_low=(uint16_t)clock_ticks;TMR5HLD=clock_ticks>>16;}
  return &timer_low;
}
#define TMR4 (*timer_read())
static uint32_t bp_timer_microseconds(void) { return (origin+clock_ticks/16)&0x7FFFFFFF; }
static bool detection_host_ready(void) { return cancel && reads>256; }
#define user_serial_ready_to_read detection_host_ready
static void bp_write_dec_dword(uint32_t n) { (void)n; }
'''

DETECTION_TEST = r'''
static void reset(unsigned shape) {
  T4CON=0x20;T5CON=0;timer_low=123;TMR5=456;PR4=789;PR5=1011;
  waveform=shape;clock_ticks=reads=mosi_reads=miso_reads=cancel=0;
}
static void restored(void) {
  assert(T4CON==0x20 && timer_low==123 && TMR5==456 && PR4==789 && PR5==1011);
}
int main(void) {
  for(unsigned hd=0;hd<2;++hd)for(unsigned inv=0;inv<2;++inv) {
    reset(1);uart_settings.half_duplex=hd;uart_settings.receive_polarity=inv;
    uint32_t baud=uart_get_baud_rate(true);assert(baud>=9800 && baud<=10200);
    assert(hd?(!miso_reads && mosi_reads):(!mosi_reads && miso_reads));restored();
  }
  for(unsigned shape=0;shape<3;shape+=2) {
    reset(shape);origin=0x7FFFF000;
    assert(!uart_get_baud_rate(false));assert(clock_ticks>=80000000);restored();
  }
  reset(2);cancel=1;uint8_t key='x';input=&key;input_left=1;
  assert(!uart_get_baud_rate(false) && !input_left);restored();
  reset(1);T5CONbits.TON=1;assert(!uart_get_baud_rate(true) && !reads && T5CONbits.TON);
  reset(1);IEC1bits.T5IE=1;assert(!uart_get_baud_rate(true) && !reads);IEC1bits.T5IE=0;
  puts("PASS detection: full/shared RX pins, both polarities, static-line timeout with 31-bit wrap, host cancel, timer ownership and restoration");
}
'''

BAUD_TEST = r'''
static uint64_t error(uint32_t baud, uint32_t divisor) {
  int64_t difference=4000000-(int64_t)baud*divisor;
  return difference<0 ? -difference : difference;
}
int main(void) {
 assert(uart_baud_to_brg(1)==65535 && uart_baud_to_brg(61)==65535);
 assert(uart_baud_to_brg(4000000)==0 && uart_baud_to_brg(4000001)==0);
 assert(uart_baud_to_brg(9600)==416 && uart_baud_to_brg(115200)==34);
 assert(uart_baud_to_brg(31250)==127);
 for(uint32_t baud=62;baud<=999999;++baud) {
   uint32_t divisor=(uint32_t)uart_baud_to_brg(baud)+1;
   /* Frequency is monotone in divisor, so neither neighbor may be better. */
   uint64_t e=error(baud,divisor);
   if(divisor>1)assert(e*(divisor-1)<=error(baud,divisor-1)*divisor);
   if(divisor<65536)assert(e*(divisor+1)<=error(baud,divisor+1)*divisor);
 }
 puts("PASS all 999938 accepted baud values minimize exact frequency error; endpoint clamps and MIDI");
}
'''


def source(path, revision):
    if revision:
        return subprocess.check_output(["git", "show", revision + ":Firmware/" + path],
                                       cwd=ROOT, text=True, encoding="utf-8")
    return (ROOT / "Firmware" / path).read_text(encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    parser.add_argument("--case", choices=("setup", "binary", "bridge", "common", "restore", "detection", "baud", "all"), default="all")
    args = parser.parse_args()
    uart = source("protocol_uart/uart.c", args.revision)
    core = source("core_system/proc_menu.c", args.revision)
    settings = re.search(r"typedef struct \{.*?static UARTSettings uart_settings = \{0\};", uart, re.S).group()
    table = re.search(r"const uint16_t UART_BRG_SPEED\[\] = \{.*?\};", uart, re.S).group()
    common = re.search(r"static const uint32_t UART_COMMON_BAUD_RATES\[\] = \{.*?\};", uart, re.S).group()
    macros = "\n".join("#define " + x + " ((void)0)" for x in set(re.findall(r"\bMSG_\w+", uart)))
    for board in ("V4", "V3"):
        directory = ROOT / "Firmware/build/uart-test" / board
        directory.mkdir(parents=True, exist_ok=True)
        board_macro = "#define BUSPIRATE" + board + "\n"
        for case in ("setup", "binary", "bridge", "common", "restore", "detection", "baud"):
            if args.case not in ("all", case):
                continue
            if case == "bridge" and board == "V3":
                continue  # V3's legacy bridge has no physical button escape.
            restore = "\n".join(function(uart, n) for n in ("uart_get_closest_common_rate", "uart_setup_execute", "uart_run_macro"))
            bodies = {"setup": function(core, "getint") + function(uart, "uart_setup_prepare"),
                       "binary": (function(uart, "uart_binary_service_rx") if "static void uart_binary_service_rx(" in uart else "") + function(uart, "binUART"), "bridge": restore,
                       "common": function(uart, "uart_get_closest_common_rate"), "restore": restore,
                       "baud": ""}
            if case == "detection":
                bodies[case] = "\n".join(function(uart, n) for n in
                                         ("uart_get_closest_common_rate", "uart_rx_level", "uart_wait_rx_level",
                                          "uart_sample_ticks", "uart_get_baud_rate"))
            body = bodies[case]
            test = {"setup": SETUP_TEST, "binary": BINARY_TEST, "bridge": BRIDGE_TEST,
                     "common": COMMON_TEST, "restore": RESTORE_TEST, "detection": DETECTION_TEST, "baud": BAUD_TEST}[case]
            definitions = "\n".join(re.findall(r"^#define UART_.*", uart, re.M))
            c = directory / (case + ".c")
            exe = c.with_suffix(".exe")
            prefix = PREFIX
            if case == "detection":
                prefix = re.sub(r"static uint32_t uart_get_baud_rate[^\n]+\n", "", prefix)
            detection = DETECTION_PREFIX if case == "detection" else ""
            baud_helper = function(uart, "uart_baud_to_brg") if "static uint16_t uart_baud_to_brg(" in uart else ""
            c.write_text("\n".join((board_macro, prefix, settings, table, common, macros, definitions,
                                     baud_helper, detection, body, test)), encoding="ascii")
            subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
                            "-Wno-unused-variable", "-Wno-unused-but-set-variable", "-Wno-implicit-fallthrough",
                            str(c), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=10)
            print(board, case, "PASS", flush=True)


if __name__ == "__main__":
    main()
