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
#define BP_COMMAND_BUFFER_SIZE 256
#define CMDLENMSK 255
static struct { unsigned speed, high_impedance, command_error, numbits, int16; } mode_configuration;
static char cmdbuf[256];
static unsigned cmdstart, prompts, header;
static bool setup_defaults_mode;
static uint16_t U2BRG;
static struct { unsigned OERR, UTXBF; } U2STAbits, U1STAbits;
static uint16_t U2TXREG, U1TXREG;
static const uint8_t *input;
static unsigned input_left, output_count;
static uint8_t output[128];
static unsigned setups;
static unsigned tx_calls, button_polls;
static uint16_t configured_brg;
static void consumewhitechars(void) { while (cmdbuf[cmdstart]==' ') ++cmdstart; }
static int getnumber(int def, int min, int max, int exit) {
  (void)exit; ++prompts; assert(def>=min && def<=max); return def;
}
static unsigned long getlong(unsigned long def, unsigned long min, unsigned long max, int exit) {
  (void)exit; ++prompts; assert(min>=62 && def>=min && def<=max); return def;
}
static void UARTsettings(void) { ++header; }
static bool uart2_rx_ready(void) { return false; }
static uint16_t uart2_rx(void) { assert(0); return 0; }
static void uart2_set_single_wire(bool b) { (void)b; }
static void uart2_setup(uint16_t b, bool o, bool i, uint8_t d, bool s) {
  (void)o; (void)i; (void)d; (void)s; ++setups; configured_brg=b;
}
static void uart2_enable(void) {}
static void uart2_disable(void) {}
static void uart2_tx(uint16_t c) { (void)c; ++tx_calls; }
static uint16_t uart2_rx_overrun_count(void) { return 0; }
static void bp_disable_mode_led(void) {}
static uint32_t uart_get_baud_rate(bool quiet) { (void)quiet; return 9600; }
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
  const char *invalid[]={"10 0 1 1 1 2 1", "10 1 1 1 1 2 1", "10 61 1 1 1 2 1"};
  for (unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {
    load(invalid[i]); uart_setup_prepare();
    assert(prompts && !header && mode_configuration.speed==0);
  }
  load("10 9600 1 1 1 2 1"); uart_setup_prepare();
  assert(!prompts && header==1 && U2BRG==415);
  load("5 1 1 1 2 1"); uart_setup_prepare();
  assert(!prompts && header==1 && mode_configuration.speed==4);
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


def source(path, revision):
    if revision:
        return subprocess.check_output(["git", "show", revision + ":Firmware/" + path],
                                       cwd=ROOT, text=True, encoding="utf-8")
    return (ROOT / "Firmware" / path).read_text(encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    parser.add_argument("--case", choices=("setup", "binary", "bridge", "all"), default="all")
    args = parser.parse_args()
    uart = source("protocol_uart/uart.c", args.revision)
    core = source("core_system/proc_menu.c", args.revision)
    settings = re.search(r"typedef struct \{.*?static UARTSettings uart_settings = \{0\};", uart, re.S).group()
    table = re.search(r"const uint16_t UART_BRG_SPEED\[\] = \{.*?\};", uart, re.S).group()
    macros = "\n".join("#define " + x + " ((void)0)" for x in set(re.findall(r"\bMSG_\w+", uart)))
    for board in ("V4", "V3"):
        directory = ROOT / "Firmware/build/uart-test" / board
        directory.mkdir(parents=True, exist_ok=True)
        board_macro = "#define BUSPIRATE" + board + "\n"
        for case in ("setup", "binary", "bridge"):
            if args.case not in ("all", case):
                continue
            if case == "bridge" and board == "V3":
                continue  # V3's legacy bridge has no physical button escape.
            bodies = {"setup": function(core, "getint") + function(uart, "uart_setup_prepare"),
                      "binary": function(uart, "binUART"), "bridge": function(uart, "uart_run_macro")}
            body = bodies[case]
            test = {"setup": SETUP_TEST, "binary": BINARY_TEST, "bridge": BRIDGE_TEST}[case]
            definitions = "\n".join(re.findall(r"^#define UART_MACRO_.*", uart, re.M))
            c = directory / (case + ".c")
            exe = c.with_suffix(".exe")
            c.write_text("\n".join((board_macro, PREFIX, settings, table, macros, definitions, body, test)), encoding="ascii")
            subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
                            "-Wno-unused-variable", "-Wno-unused-but-set-variable", str(c), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=10)
            print(board, case, "PASS", flush=True)


if __name__ == "__main__":
    main()
