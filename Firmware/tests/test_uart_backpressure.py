"""Actual UART streaming loops and CDC writers under bounded host stalls.

Native scheduling proof, not a USB SIE or electrical simulation. Core source is
read-only: test-only wait assertions reject an attempted blocking IN write.
"""
import argparse
from pathlib import Path
import re
import subprocess

from test_core_system import function
from test_uart import PREFIX
from test_usb import function as usb_function

ROOT = Path(__file__).resolve().parents[2]

LOOP_TEST = r'''
static unsigned queued, dequeued, polls, writes;
static unsigned mock_overruns, led_events;
static bool host_ready, key_exit;
static bool stream_ready(void) { return queued!=0; }
static uint8_t stream_pending(void) { return queued; }
static uint16_t stream_rx(void) { assert(queued);--queued;++dequeued;return 0x55; }
static bool getInReady(void) { return host_ready; }
static bool button(void) {
 if(!polls)queued=4;
 ++polls;assert(polls<=4);
 if(polls==2)mock_overruns=1;
 if(polls==4) {assert(!dequeued);return true;}
 return false;
}
static bool host_input(void) { return input_left && (polls==0 || polls>=4 || (key_exit && polls>=3)); }
static void stream_output(uint8_t c) {
 if(c!=1)assert(host_ready);
 ++writes;
}
#define BP_BUTTON_ISDOWN() button()
#define uart2_rx_ready stream_ready
#define uart2_rx_pending stream_pending
#define uart2_rx stream_rx
#define user_serial_ready_to_read host_input
#define user_serial_transmit_character stream_output
#define uart2_rx_overrun_count() mock_overruns
#define bp_disable_mode_led() (++led_events)
'''

LOOP_MAIN = r'''
int main(void) {
 for(unsigned macro=1;macro<=2;++macro)for(unsigned key=0;key<=1;++key) {
  if(macro==1 && key)continue;
  queued=dequeued=polls=writes=mock_overruns=led_events=0;host_ready=false;key_exit=key;
  uint8_t x='x';input=&x;input_left=key;
  uart_run_macro(macro);
  assert(queued==4 && !dequeued && !writes && polls==(key?3:4) && led_events==1);
 }
 uint8_t commands[]={15,0};input=commands;input_left=2;
 queued=dequeued=polls=writes=mock_overruns=led_events=0;host_ready=false;key_exit=false;
 binUART();assert(polls==4 && writes==1 && !input_left && led_events==1);
 queued=4;dequeued=0;uart_settings.echo_uart=1;uart_binary_service_rx();
 assert(queued==4 && !dequeued);
 host_ready=true;uart_binary_service_rx();assert(!queued && dequeued==4);
 puts("PASS stalled-IN terminal bridge/monitor/button/key and binary bridge escape; queued RX retained until ready");
}
'''

CDC_PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t BYTE;
typedef struct { BYTE BDSTAT,BDCNT; BYTE *BDADDR; } BDentry;
#define CDC_BUFFER_SIZE 64
#define CDC_FLUSH_MS 4
#define UOWN 0x80
#define DTS 0x40
#define DTSEN 8
#define CONFIGURED_STATE 1
#define USB_INTERRUPTS
static BYTE usb_device_state,cdc_In_len,cdc_timeout_count,ZLPpending,lock,IsInBufferA;
static BYTE cdc_In_bufferA[64],cdc_In_bufferB[64],*InPtr;
static BDentry inbd,*CDC_Inbdp=&inbd;
static unsigned fake_irq,hook_at,hook_count,in_isr;
void CDCFlushOnTimeout(void);
static void schedule(void) {
 if(!fake_irq || in_isr)return;
 if(++hook_count==hook_at) {
  in_isr=1;cdc_timeout_count=4;CDCFlushOnTimeout();in_isr=0;
 }
}
static BYTE irq_state(void) {schedule();return fake_irq;}
static void enable(void) {fake_irq=1;schedule();}
#define TestGlobalUsbInterruptEnable() irq_state()
#define DisableGlobalUsbInterrupt() (fake_irq=0)
#define EnableUsbGlobalInterrupt() enable()
'''

CDC_MAIN = r'''
int main(void) {
 for(unsigned count=0;count<64;++count)for(unsigned hook=0;hook<12;++hook) {
  usb_device_state=1;inbd.BDSTAT=inbd.BDCNT=0;
  cdc_In_len=count;cdc_timeout_count=ZLPpending=lock=0;IsInBufferA=1;
  InPtr=cdc_In_bufferA+count;fake_irq=1;hook_at=hook;hook_count=in_isr=0;
  for(unsigned i=0;i<count;++i)cdc_In_bufferA[i]=i;
  assert(getInReady());schedule();putc_cdc(0x55);
  unsigned sent=(inbd.BDSTAT&UOWN)?inbd.BDCNT:0;
  assert(sent+cdc_In_len==count+1 && !lock && fake_irq);
  for(unsigned i=0;i<count+1;++i) {
   unsigned byte=i<sent?inbd.BDADDR[i]:(InPtr-cdc_In_len)[i-sent];
   assert(byte==(i==count?0x55:i));
  }
 }
 inbd.BDSTAT=UOWN;assert(!getInReady());usb_device_state=0;inbd.BDSTAT=0;assert(!getInReady());
 puts("PASS actual CDC getInReady/putc/putda/SOF: all64 fill levels and12 interrupt schedules, byte conservation, no owned-IN wait");
}
'''


def run(directory, name, text):
    c = directory / (name + ".c")
    c.write_text(text, encoding="ascii")
    exe = c.with_suffix(".exe")
    subprocess.run(["gcc", "-std=gnu99", "-O1", "-Wall", "-Wextra", "-Werror",
                    "-Wno-unused-function", "-Wno-unused-variable", "-Wno-unused-but-set-variable",
                    str(c), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True, timeout=10)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--revision")
    args = p.parse_args()
    path = "Firmware/protocol_uart/uart.c"
    uart = (subprocess.check_output(["git", "show", args.revision + ":" + path], cwd=ROOT, text=True)
            if args.revision else (ROOT / path).read_text())
    directory = ROOT / "Firmware/build/uart-backpressure-test"
    directory.mkdir(parents=True, exist_ok=True)
    settings = re.search(r"typedef struct \{.*?static UARTSettings uart_settings = \{0\};", uart, re.S).group()
    tables = "\n".join(re.findall(r"(?:static )?const uint(?:16|32)_t UART_.*?\};", uart, re.S))
    macros = "\n".join("#define " + m + " ((void)0)" for m in set(re.findall(r"\bMSG_\w+", uart)))
    definitions = "\n".join(re.findall(r"^#define UART_.*", uart, re.M))
    body = "\n".join(function(uart, n) for n in ("uart_baud_to_brg", "uart_get_closest_common_rate",
                   "uart_setup_execute", "uart_run_macro", "uart_binary_service_rx", "binUART"))
    prefix = PREFIX.replace("#define BP_BUTTON_ISDOWN() (++button_polls>1)", "")
    prefix = prefix.replace("static bool getInReady(void) { return true; }", "")
    run(directory, "loops", "\n".join(("#define BUSPIRATEV4", prefix, settings, tables,
                                       macros, definitions, LOOP_TEST, body, LOOP_MAIN)))
    cdc = (ROOT / "Firmware/core_usb/cdc.c").read_text()
    bodies = []
    for name in ("WaitInReady", "getInReady", "putda_cdc", "SendZLP", "CDCFlushOnTimeout", "putc_cdc"):
        text = usb_function(cdc, name).replace("inline ", "")
        if name == "WaitInReady":
            text = text.replace("{", "{ assert(!(CDC_Inbdp->BDSTAT & UOWN));", 1)
        bodies.append(text)
    run(directory, "cdc", "\n".join((CDC_PREFIX, *bodies, CDC_MAIN)))


if __name__ == "__main__":
    main()
