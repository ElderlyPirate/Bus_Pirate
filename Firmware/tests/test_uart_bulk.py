"""Production binary UART plus actual RX ring/ISR under a dense loopback schedule.

An IRQ completes each transmitted word before its host ACK. This is one valid
interleaving, not a PIC24 timing/SIE simulation; live diagnostic counters supply
the hardware evidence. No UART/USB driver algorithm is duplicated in the mock.
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
#define BP_ENABLE_UART_SUPPORT
#define ON 1
#define OFF 0
#define NO 0
#define INPUT 1
#define OUTPUT 0
#define OPEN_DRAIN 1
#define PUSH_PULL 0
#define UART2_OPEN_DRAIN 1
#define UART2_RX_FLAG_PARITY 1
#define UART2_RX_FLAG_FRAMING 2
#define BP_MISO_RPIN 11
#define BP_MOSI_RPIN 12
#define U2TX_IO 5
#define MASKBOTTOM8(v,b) ((v)&((1u<<(b))-1))
#define _U2MODE_STSEL_POSITION 0
#define _U2MODE_PDSEL_POSITION 1
#define _U2MODE_BRGH_MASK 8
#define _U2MODE_RXINV_POSITION 4
#define _U2STA_UTXINV_POSITION 14
static struct { unsigned high_impedance:1; } mode_configuration;
static uint16_t BP_MOSI_RPOUT,BP_MOSI_ODC,BP_MOSI_DIR,U2BRG,U1TXREG;
static struct { unsigned OERR,UTXBF; } U1STAbits;
static union { uint16_t value; struct {unsigned pad:15,UARTEN:1;} bits; } mode;
static union { uint16_t value; struct {
 unsigned URXDA:1,OERR:1,FERR:1,PERR:1,RIDLE:1,ADDEN:1,URXISEL:2;
 unsigned TRMT:1,UTXBF:1,UTXEN:1,UTXBRK:1,pad:1,UTXISEL0:1,UTXINV:1,UTXISEL1:1;
} bits; } sta;
static struct { unsigned U2RXR; } RPINR19bits;
static struct { unsigned U2RXIF; } IFS1bits;
static struct { unsigned U2RXIE; } IEC1bits;
static uint16_t txreg,rxreg;
static unsigned pending,in_irq,seen,high_water;
static bool refill;
static uint8_t input[4096],output[8192];
static unsigned input_left,input_position,output_count;
static void hardware_step(void);
static void *status(void) { hardware_step();return &sta.bits; }
static uint16_t *read_rx(void) { assert(sta.bits.URXDA);sta.bits.URXDA=0;return &rxreg; }
static uint16_t *write_tx(void) { hardware_step();pending=1;return &txreg; }
#define U2MODE mode.value
#define U2MODEbits mode.bits
#define U2STA sta.value
#define U2STAbits (*(__typeof__(sta.bits)*)status())
#define U2RXREG (*read_rx())
#define U2TXREG (*write_tx())
#define BP_BUTTON_ISDOWN() 1
static bool user_serial_ready_to_read(void);
static uint8_t user_serial_read_byte(void);
static void user_serial_transmit_character(uint8_t c);
static void bp_binary_io_peripherals_set(uint8_t c) { (void)c; }
static uint8_t bp_binary_io_pullup_control(uint8_t c) { (void)c;return 1; }
static bool getInReady(void) { return true; }
static void bp_disable_mode_led(void) {}
#define REPORT_IO_SUCCESS() user_serial_transmit_character(1)
#define REPORT_IO_FAILURE() user_serial_transmit_character(0)
'''

HOST = r'''
static void hardware_step(void) {
 if (!pending || in_irq) return;
 assert(mode.bits.UARTEN && sta.bits.UTXEN && IEC1bits.U2RXIE);
 pending=0;rxreg=txreg;sta.bits.URXDA=1;++seen;
 in_irq=1;_U2RXInterrupt();in_irq=0;
 unsigned used=(uart2_rx_head-uart2_rx_tail)&127;
 if(used>high_water)high_water=used;
 assert(!sta.bits.URXDA);
}
static bool user_serial_ready_to_read(void) {
 hardware_step();return input_position<input_left || !uart2_rx_ready();
}
static uint8_t user_serial_read_byte(void) {
 hardware_step();
 if(input_position==input_left) {assert(!uart2_rx_ready());return 0;}
 return input[input_position++];
}
static void user_serial_transmit_character(uint8_t c) {
 /* Bulk command and its sixteen payload ACKs must remain contiguous. */
 if(c!=1 && input_position>1)assert((input_position-1)%17==0);
 hardware_step();assert(output_count<sizeof(output));output[output_count++]=c;
 if(refill) {rxreg=0x55;sta.bits.URXDA=1;_U2RXInterrupt();}
}
int main(void) {
 const unsigned sizes[]={16,64,128,256,512,1024};
 for(unsigned echo=0;echo<2;++echo)for(unsigned s=0;s<sizeof(sizes)/sizeof(*sizes);++s) {
  unsigned n=sizes[s];input_left=input_position=output_count=seen=high_water=pending=0;
  input[input_left++]=echo?2:3;
  for(unsigned off=0;off<n;off+=16) {
   input[input_left++]=0x1f;
   for(unsigned i=0;i<16;++i)input[input_left++]=0x40+((off+i)&63);
  }
  binUART();
  assert(seen==n && input_position==input_left && !uart2_rx_ready());
  unsigned acks=0,received=0;
  for(unsigned i=0;i<output_count;++i) {
   if(output[i]==1)++acks;
   else {
#ifndef OBSERVE
    assert(output[i]==0x40+(received&63));
#else
    assert(output[i]>=0x40 && output[i]<0x80);
#endif
    ++received;
   }
  }
  assert(acks==1+n+n/16);
  printf("echo=%u payload=%u ACKs=%u RX=%u software_drops=%u high_water=%u\n",
         echo,n,acks,received,uart2_rx_overrun_count(),high_water);fflush(stdout);
#ifndef OBSERVE
  assert(!uart2_rx_overrun_count() && received==(echo?n:0));
#endif
 }
#ifdef HAS_SERVICE
 /* A producer that replenishes every dequeued word cannot extend this call. */
 uart2_enable();input_position=0;output_count=0;uart_settings.echo_uart=1;
 for(unsigned i=0;i<4;++i) {rxreg=0x55;sta.bits.URXDA=1;_U2RXInterrupt();}
 refill=true;uart_binary_service_rx();refill=false;
 assert(output_count==4 && uart2_rx_pending()==4);
#endif
#ifdef OBSERVE
 puts("OBSERVED production parser+ISR+ring; losslessness NOT graded");
#else
 puts("PASS production parser+ISR+ring: bounded bulk receive service, ACK counts and order");
#endif
}
'''


def source(name, revision):
    path = "Firmware/protocol_uart/" + name
    if revision:
        return subprocess.check_output(["git", "show", revision + ":" + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--revision")
    p.add_argument("--observe", action="store_true", help="Record baseline ring drops without grading losslessness")
    args = p.parse_args()
    uart, driver = source("uart.c", args.revision), source("uart2.c", args.revision)
    driver = re.sub(r'^#include[^\n]*', '', driver, flags=re.M)
    driver = re.sub(r'__attribute__\(\([^)]*\)\)', '', driver)
    driver = re.sub(r'\binline\s+', '', driver)
    settings = re.search(r"typedef struct \{.*?static UARTSettings uart_settings = \{0\};", uart, re.S).group()
    table = re.search(r"const uint16_t UART_BRG_SPEED\[\] = \{.*?\};", uart, re.S).group()
    macros = "\n".join("#define " + m + " ((void)0)" for m in set(re.findall(r"\bMSG_\w+", uart)))
    helpers = function(uart, "uart_binary_service_rx") if "static void uart_binary_service_rx(" in uart else ""
    for board in ("V4", "V3"):
        out = ROOT / "Firmware/build/uart-bulk-test" / board
        out.mkdir(parents=True, exist_ok=True)
        c = out / "bulk.c"
        c.write_text("\n".join(("#define BUSPIRATE" + board,
                                  "#define OBSERVE" if args.observe else "",
                                  "#define HAS_SERVICE" if helpers else "", PREFIX, driver,
                                  settings, table, macros, helpers, function(uart, "binUART"), HOST)), encoding="ascii")
        exe = c.with_suffix(".exe")
        subprocess.run(["gcc", "-std=gnu99", "-O1", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unused-function", "-Wno-unused-variable", str(c), "-o", str(exe)], check=True)
        print(board, flush=True)
        subprocess.run([str(exe)], check=True, timeout=15)


if __name__ == "__main__":
    main()
