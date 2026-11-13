"""Compile the production UART2 driver against a four-deep FIFO mock.

Models register ordering, ring ownership and delayed interrupts, not electrical
timing. Run the established UART/HDUART live batteries for hardware acceptance.
"""
import argparse
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]

PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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
static uint16_t BP_MOSI_RPOUT, BP_MOSI_ODC, BP_MOSI_DIR, U2BRG;
static union { uint16_t value; struct { unsigned pad:15, UARTEN:1; } bits; } mode;
static union { uint16_t value; struct {
 unsigned URXDA:1,OERR:1,FERR:1,PERR:1,RIDLE:1,ADDEN:1,URXISEL:2;
 unsigned TRMT:1,UTXBF:1,UTXEN:1,UTXBRK:1,pad:1,UTXISEL0:1,UTXINV:1,UTXISEL1:1;
} bits; } sta;
static struct { unsigned U2RXR; } RPINR19bits;
static struct { unsigned U2RXIF; } IFS1bits;
static struct { unsigned U2RXIE; } IEC1bits;
static uint16_t fifo[4], fifo_flags[4], rxreg, txreg;
static unsigned fifo_count, tx_ticks, irq_active, receiving_echo, access_count;
static bool previous_oerr;
void _U2RXInterrupt(void);
static void refresh(void) {
 sta.bits.URXDA=!!fifo_count;
 sta.bits.PERR=fifo_count && (fifo_flags[0]&1);
 sta.bits.FERR=fifo_count && (fifo_flags[0]&2);
}
static void push(uint16_t v, uint8_t flags) {
 assert(fifo_count<4); fifo[fifo_count]=v;fifo_flags[fifo_count++]=flags;
 IFS1bits.U2RXIF=1;refresh();
}
static void *status(void) {
 assert(++access_count<100000);
 if (previous_oerr && !sta.bits.OERR) fifo_count=0;
 previous_oerr=sta.bits.OERR;
 if (tx_ticks && !irq_active) {
   if (++tx_ticks==3 && receiving_echo) push(txreg,0);
   if (tx_ticks>=6) { sta.bits.TRMT=1;tx_ticks=0; }
   if (fifo_count && IEC1bits.U2RXIE) { irq_active=1;_U2RXInterrupt();irq_active=0; }
 }
 refresh();return &sta.bits;
}
static uint16_t *read_rx(void) {
 assert(fifo_count);rxreg=fifo[0];
 for(unsigned i=1;i<fifo_count;++i) {fifo[i-1]=fifo[i];fifo_flags[i-1]=fifo_flags[i];}
 --fifo_count;refresh();return &rxreg;
}
static uint16_t *write_tx(void) { tx_ticks=1;sta.bits.TRMT=0;return &txreg; }
#define U2MODE mode.value
#define U2MODEbits mode.bits
#define U2STA sta.value
#define U2STAbits (*(__typeof__(sta.bits)*)status())
#define U2RXREG (*read_rx())
#define U2TXREG (*write_tx())
'''

TESTS = {
    "overrun": r'''
int main(void) {
 uart2_setup(416,0,0,0,0);uart2_enable();
 for(unsigned i=0;i<4;++i)push(0x40+i,i);
 sta.bits.OERR=1;_U2RXInterrupt();status();
 for(unsigned i=0;i<4;++i) {assert(uart2_rx_ready());assert(uart2_rx()==0x40+i);assert(uart2_rx_flags()==i);}
 assert(!uart2_rx_ready() && uart2_rx_overrun_count()==1);
 push(0x55,0);_U2RXInterrupt();assert(uart2_rx()==0x55);
 puts("PASS hardware overrun retains buffered characters/flags before destructive OERR clear; receiver recovers");
}
''',
    "width": r'''
int main(void) {
 uart2_setup(416,0,0,3,0);uart2_enable();
 for(uint16_t c=0;c<512;++c) {
   push(c,c&3);_U2RXInterrupt();assert(uart2_rx_ready());
   assert(uart2_rx()==c);assert(uart2_rx_flags()==(c&3));
   uart2_tx(c);assert(txreg==c);
 }
 assert(!uart2_rx_ready());puts("PASS all 512 nine-bit RX/TX words and per-character error flags");
}
''',
    "fifo": r'''
int main(void) {
 uart2_setup(416,0,0,0,0);uart2_enable();
 for(unsigned round=0;round<100;++round) {
   for(unsigned i=0;i<4;++i)push((round*4+i)&255,i);
   _U2RXInterrupt();assert(!fifo_count);
   for(unsigned i=0;i<4;++i) {assert(uart2_rx()==((round*4+i)&255));assert(uart2_rx_flags()==i);}
 }
 for(unsigned i=0;i<128;++i) {push(i,0);_U2RXInterrupt();}
 assert(uart2_rx_overrun_count()==1);
 for(unsigned i=0;i<127;++i)assert(uart2_rx()==i);
 assert(!uart2_rx_ready());push(222,3);_U2RXInterrupt();assert(uart2_rx()==222 && uart2_rx_flags()==3);
 puts("PASS FIFO drain, flag-before-data, 100 wraps, ring-full drop and recovery");
}
''',
    "echo": r'''
int main(void) {
 for(unsigned irq=0;irq<2;++irq) {
   uart2_set_single_wire(true);uart2_setup(416,1,0,0,0);uart2_enable();
   push(0x41,1);_U2RXInterrupt();
   /* An already completed peer byte must survive starting our transmission. */
   push(0x42,2);
   IEC1bits.U2RXIE=irq;receiving_echo=1;
   uart2_tx(0x55);assert(IEC1bits.U2RXIE==irq);_U2RXInterrupt();
   assert(uart2_rx()==0x41 && uart2_rx_flags()==1);
   assert(uart2_rx_ready() && uart2_rx()==0x42 && uart2_rx_flags()==2);
   assert(!uart2_rx_ready() && uart2_rx_overrun_count()==0);
   push(0x43,0);_U2RXInterrupt();assert(uart2_rx()==0x43);
   uart2_disable();
 }
 puts("PASS HDUART: queued/pending peer data retained; immediate/delayed self-echo discarded; IRQ mask preserved");
}
''',
}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--revision")
    p.add_argument("--case", choices=(*TESTS, "all"), default="all")
    args = p.parse_args()
    path = "Firmware/protocol_uart/uart2.c"
    text = (subprocess.check_output(["git", "show", args.revision + ":" + path], cwd=ROOT, text=True)
            if args.revision else (ROOT / path).read_text())
    if not args.revision:
        from test_core_system import function
        enable = function(text, "uart2_enable")
        disable = function(text, "uart2_disable")
        assert enable.index("U2RXIE = OFF") < enable.index("uart2_rx_head = 0") < enable.index("UARTEN = ON")
        assert disable.index("BP_MOSI_DIR = INPUT") < disable.index("UARTEN = OFF") < disable.index("UARTTX_PIN = OFF")
    text = re.sub(r'^#include[^\n]*', '', text, flags=re.M)
    text = re.sub(r'__attribute__\(\([^)]*\)\)', '', text)
    text = re.sub(r'\binline\s+', '', text)
    for case, test in TESTS.items():
        if args.case not in (case, "all"):
            continue
        directory = ROOT / "Firmware/build/uart-test/driver"
        directory.mkdir(parents=True, exist_ok=True)
        c = directory / (case + ".c")
        exe = c.with_suffix(".exe")
        c.write_text(PREFIX + "\n#define BP_ENABLE_UART_SUPPORT\n" + text + test, encoding="ascii")
        subprocess.run(["gcc", "-std=gnu99", "-O1", "-Wall", "-Wextra", "-Werror", str(c), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=10)


if __name__ == "__main__":
    main()
