"""Host-compile production USB functions with fake BDs; not an SIE simulator.

python Firmware/tests/test_usb.py [--revision ebefa34]
Requires gcc. Each assertion uses the production function text, not a model.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


def source(path, revision):
    if revision:
        for directory in ("core_usb", "dp_usb"):
            result = subprocess.run(["git", "show", revision + ":Firmware/" + directory + "/" + Path(path).name],
                                    cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
            if result.returncode == 0:
                return result.stdout
        raise RuntimeError("USB source not found at " + revision + ": " + path)
    usb = ROOT / "Firmware/core_usb"
    if not usb.exists():
        usb = ROOT / "Firmware/dp_usb"
    return (usb / Path(path).name).read_text()


def function(text, name):
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    match = re.search(r"^(?:static |inline )*(?:void|BYTE|BYTE\*|uint16_t)\s*" + name +
                      r"\([^;]*?\)\s*\{", text, re.M)
    assert match, name
    depth, end = 1, match.end()
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t BYTE;
typedef struct { BYTE BDCNT, BDSTAT; BYTE *BDADDR; } BDentry;
static BYTE inbuf[64], outbuf[64];
static BDentry inbd = {0, 0, inbuf}, outbd = {0, 0, outbuf};
static BDentry *EP0_Inbdp = &inbd, *EP0_Outbdp = &outbd;
static int error, ack, U1BRG;
#define BAUDCLOCK_FREQ 16000000
#define UART_BAUD_setup(x) U1BRG = (x)
#define USB_EP0_BUFFER_SIZE 8
#define UOWN 0x80
#define DTSEN 8
static void usb_RequestError(void) { error++; }
static void usb_ack_dat1(int count) { ack = count; }
static void usb_set_out_handler(int ep, void (*handler)(void)) { (void)ep; (void)handler; }
static void usb_set_in_handler(int ep, void (*handler)(void)) { (void)ep; (void)handler; }
static void usb_unset_out_handler(int ep) { (void)ep; }
static void usb_register_class_setup_handler(void (*handler)(void)) { (void)handler; }
static void cdc_get_line_coding(void) {}
static void cdc_set_line_coding_status(void) {}
static void cdc_set_control_line_state_status(void) {}
void cdc_setup(void);
void cdc_set_line_coding_data(void);
'''

LINE_TEST = r'''
static void setup(BYTE type, BYTE req, BYTE len) {
    memset(outbuf, 0, sizeof(outbuf));
    outbuf[0] = type; outbuf[1] = req; outbuf[6] = len;
    error = 0; ack = -1;
    cdc_setup();
}
int main(void) {
    initCDC();
    memset(inbuf, 0xa5, sizeof(inbuf));
    setup(0xa1, CDC_GET_LINE_CODING, 255);
    assert(!error && ack == 7 && inbuf[7] == 0xa5 && inbuf[8] == 0xa5);
    assert(!memcmp(inbuf, "\x00\xc2\x01\x00\x00\x00\x08", 7));
    puts("PASS line coding is exactly seven bytes, no enum padding/read overrun");
    setup(0x21, CDC_SET_LINE_CODING, 8); assert(error);
    setup(0xa1, CDC_SET_LINE_CODING, 7); assert(error);
    setup(0x21, CDC_SET_LINE_CODING, 7); assert(!error);
    memset(outbuf, 0, 7); outbuf[1] = 0xc2; outbuf[2] = 1; outbuf[6] = 8;
    outbd.BDCNT = 6; cdc_set_line_coding_data(); assert(error);
    error = 0; outbd.BDCNT = 7; cdc_set_line_coding_data(); assert(!error && ack == 0);
    setup(0xa1, CDC_GET_LINE_CODING, 7); assert(!error && ack == 7 && inbuf[6] == 8);
    puts("PASS malformed setup/data lengths rejected; seven-byte round trip");
    (void)U1BRG;
    return 0;
}
'''

BAUD_TEST = r'''
int main(void) {
    initCDC();
    memset(outbuf, 0, sizeof(outbuf)); outbuf[6] = 8; outbd.BDCNT = 7;
    U1BRG = 1234;
    cdc_set_line_coding_data();
    assert(!error && ack == 0 && U1BRG == 1234);
    puts("PASS zero baud is virtual metadata, no division or UART register change");
    cdc_setup();
    return 0;
}
'''

EP_PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t BYTE;
typedef struct { BYTE BDCNT, BDSTAT; BYTE *BDADDR; } BDentry;
typedef volatile unsigned int usb_uep_t;
#define MAX_EPNUM_USED 2
#define USB_CALC_BD(ep,dir,pp) (2*(ep)+(dir))
#define USB_PP_EVEN 0
#define USB_UEP_EPSTALL 2
#define USB_UEP_EPINEN 4
#define USB_UEP_EPOUTEN 8
#define UOWN 0x80
#define DTS 0x40
#define DTSEN 8
#define BSTALL 4
static BDentry usb_bdt[6];
static BYTE usb_halt_saved[6];
static usb_uep_t regs[16];
#define USB_UEP regs
static BYTE reply[8], packet[8];
static BDentry inbd = {0, 0, reply}, reqbd = {0, 0, packet};
static BDentry *EP0_Inbdp = &inbd;
static int error, ack;
static void usb_RequestError(void) { error++; }
static void usb_ack_dat1(int n) { ack = n; }
'''

EP_TEST = r'''
int main(void) {
    (void)usb_halt_saved;
    BYTE states[] = {0x88, 0xc8, 0x24, 0x64, 0x84, 0xc4};
    packet[0] = 0x82; packet[1] = USB_REQUEST_GET_STATUS;
    packet[4] = 0x82; packet[6] = 2; regs[2] = 0x1d;
    for (unsigned int i = 0; i < sizeof(states); ++i) {
        usb_bdt[5].BDSTAT = states[i];
        usb_handle_StandardEndpointRequest(&reqbd);
        assert(!error && ack == 2 && usb_bdt[5].BDSTAT == states[i]);
        assert(reply[0] == (i >= 4) && reply[1] == 0);
    }
    puts("PASS endpoint GET_STATUS is read-only; completed PID is not a halt flag");
    return 0;
}
'''

BOUNDS_TEST = r'''
int main(void) {
    BYTE indices[] = {3, 15, 0x83, 0x8f, 0x72, 0xf2, 0x81};
    packet[0] = 0x82; packet[1] = USB_REQUEST_GET_STATUS; packet[6] = 2;
    regs[2] = 0x1d;
    for (unsigned int i = 0; i < sizeof(indices); ++i) {
        packet[4] = indices[i]; error = 0;
        usb_handle_StandardEndpointRequest(&reqbd);
        assert(error == 1);
    }
    packet[4] = 0x82; packet[5] = 1; error = 0;
    usb_handle_StandardEndpointRequest(&reqbd); assert(error == 1);
    packet[5] = 0; packet[1] = USB_REQUEST_SET_FEATURE;
    packet[0] = 2; packet[6] = 0; packet[2] = 1; error = 0;
    usb_handle_StandardEndpointRequest(&reqbd); assert(error == 1);
    puts("PASS endpoint indices, reserved bits, absent directions and feature selectors rejected");
    (void)usb_bdt; (void)usb_halt_saved;
    return 0;
}
'''

HALT_TEST = r'''
int main(void) {
    (void)usb_halt_saved;
    packet[0] = 2; packet[4] = 0x82; regs[2] = 0x1d;
    for (unsigned int owned = 0; owned < 2; ++owned) {
        usb_bdt[5].BDSTAT = owned ? 0xc8 : 0x64;
        usb_bdt[5].BDCNT = 23;
        packet[1] = USB_REQUEST_SET_FEATURE;
        usb_handle_StandardEndpointRequest(&reqbd);
        assert(!error && usb_bdt[5].BDSTAT == (UOWN | BSTALL));
        usb_handle_StandardEndpointRequest(&reqbd); /* idempotent SET */
        packet[1] = USB_REQUEST_CLEAR_FEATURE;
        usb_handle_StandardEndpointRequest(&reqbd);
        assert(!error && usb_bdt[5].BDCNT == 23);
        assert(usb_bdt[5].BDSTAT == (owned ? (UOWN | DTSEN) : (DTS | DTSEN)));
    }
    puts("PASS SET_HALT arms free BDs; CLEAR_HALT preserves queue ownership and resets DATA0");
    return 0;
}
'''

SETUP_PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint8_t BYTE;
typedef struct { BYTE BDCNT, BDSTAT; BYTE *BDADDR; } BDentry;
#define DTSEN 8
#define DTS 0x40
#define UOWN 0x80
#define BSTALL 4
#define USB_EP0_BUFFER_SIZE 8
#define USB_UEP_EPSTALL 2
static unsigned int USB_UEP0;
static BYTE packet[8], reply[8];
static BDentry outbd = {8, 0, packet}, inbd = {0, 0, reply};
static BDentry *EP0_Outbdp = &outbd, *EP0_Inbdp = &inbd;
static int usbrequesterrorflag, pktdis = 1, pending_in = 1, pending_out = 1;
static void (*class_setup_handler)(void), (*vendor_setup_handler)(void);
#define EnablePacketTransfer() (pktdis = 0)
static void usb_unset_in_handler(int ep) { (void)ep; pending_in = 0; }
static void usb_unset_out_handler(int ep) { (void)ep; pending_out = 0; }
static void usb_RequestError(void) {
    usbrequesterrorflag = 1; inbd.BDSTAT = outbd.BDSTAT = UOWN | BSTALL;
}
static void handled(void) { assert(pktdis); }
static void usb_handle_StandardDeviceRequest(BDentry *p) { (void)p; handled(); }
static void usb_handle_StandardInterfaceRequest(BDentry *p) { (void)p; handled(); }
static void usb_handle_StandardEndpointRequest(BDentry *p) { (void)p; handled(); }
'''

SETUP_TEST = r'''
int main(void) {
    (void)USB_UEP0; (void)usb_unset_in_handler; (void)usb_unset_out_handler;
    packet[0] = 0xa1; packet[6] = 7; class_setup_handler = handled;
    usb_handle_setup();
    assert(!pending_in && !pending_out && !pktdis && !usbrequesterrorflag);
    assert(outbd.BDSTAT == (UOWN | DTS | DTSEN));
    puts("PASS new SETUP aborts callbacks, arms DATA1 status, releases PKTDIS last");
    pktdis = 1; outbd.BDCNT = 8; class_setup_handler = 0;
    usb_handle_setup();
    assert(usbrequesterrorflag && outbd.BDSTAT == (UOWN | BSTALL) && !pktdis);
    pktdis = 1; outbd.BDCNT = 7; usb_handle_setup();
    assert(usbrequesterrorflag && !pktdis);
    puts("PASS unsupported/malformed SETUP stalls and does not overwrite OUT STALL");
    return 0;
}
'''

CDC_PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t BYTE;
#define CDC_BUFFER_SIZE 64
#define CDC_FLUSH_MS 4
static volatile BYTE cdc_In_len, cdc_Out_len, cdc_timeout_count, ZLPpending, lock;
static BYTE buffers[2][64], *InPtr, *OutPtr;
static int fake_irq = 1, busy, packets, zlps, last_count, active, reset_rx, rxready;
static int irq_state(void) {
    if (reset_rx) { cdc_Out_len = 0; OutPtr = buffers[0]; reset_rx = 0; }
    return fake_irq;
}
#define TestGlobalUsbInterruptEnable() irq_state()
#define DisableGlobalUsbInterrupt() (fake_irq = 0)
#define EnableUsbGlobalInterrupt() (fake_irq = 1)
static BYTE getInReady(void) { return !busy; }
static BYTE getOutReady(void) { return rxready; }
static BYTE getda_cdc(void) {
    if (rxready) assert(!fake_irq);
    else assert(fake_irq);
    rxready = 0; cdc_Out_len = 3; OutPtr = buffers[0]; return 3;
}
static void putda_cdc(BYTE n) {
    assert(fake_irq && !busy); packets++; last_count = n; active ^= 1; InPtr = buffers[active];
}
static void SendZLP(void) { assert(!busy && !lock); zlps++; ZLPpending = cdc_timeout_count = 0; }
BYTE *getoutbuf_cdc(uint16_t *n);
static void reset(void) {
    fake_irq = 1; busy = packets = zlps = last_count = active = reset_rx = rxready = 0;
    cdc_In_len = cdc_Out_len = cdc_timeout_count = ZLPpending = lock = 0;
    InPtr = OutPtr = buffers[0];
}
'''

CDC_TEST = r'''
int main(void) {
    uint16_t n;
    BYTE byte, data[64];
    reset(); setLock(); write_cdc(32); write_cdc(32);
    assert(packets == 1 && ZLPpending && !cdc_In_len);
    cdc_timeout_count = 4; CDCFlushOnTimeout(); assert(!zlps);
    CDC_Flush_In_Now(); assert(lock); /* nested flush must retain caller exclusion */
    putc_cdc(1); assert(lock); freeLock();
    assert(packets == 2 && last_count == 1 && !ZLPpending && !lock);
    puts("PASS cumulative bulk fill, nested flush/putc lock, immediate partial close");
    reset(); setLock();
    for (int i = 0; i < 512; ++i) { write_cdc(64); cdc_timeout_count = 4; CDCFlushOnTimeout(); }
    assert(packets == 512 && !zlps); freeLock();
    busy = 1; CDCFlushOnTimeout(); assert(!zlps);
    busy = 0; CDCFlushOnTimeout(); CDCFlushOnTimeout(); assert(zlps == 1);
    puts("PASS 32KiB bulk stream, no locked/busy ISR wait, exactly one idle ZLP");
    reset(); setLock(); write_cdc(32); write_cdc(33);
    assert(cdc_In_len == 32 && !packets); n = 0;
    assert(getinbuf_cdc(&n) == buffers[0] + 32 && n == 32);
    puts("PASS invalid commit cannot corrupt the advertised buffer span");
    reset(); n = 0; assert(getoutbuf_cdc(&n) == buffers[0] && !n);
    cdc_Out_len = 1; OutPtr = buffers[0] + 63; reset_rx = 1;
    assert(!poll_getc_cdc(&byte) && !cdc_Out_len);
    cdc_Out_len = 1; OutPtr = buffers[0] + 63; reset_rx = 1;
    assert(!peek_getc_cdc(&byte) && !cdc_Out_len);
    cdc_Out_len = 3; OutPtr = buffers[0]; memcpy(buffers[0], "abc", 3);
    assert(peek_getc_cdc(&byte) && byte == 'a' && cdc_Out_len == 3);
    assert(getc_cdc() == 'a' && read_cdc(data, 64) == 2 && !memcmp(data, "bc", 2));
    assert(read_cdc(data, 0) == 0 && fake_irq);
    puts("PASS reset interleaving cannot underflow RX count; shared bounded read path");
    rxready = 1; assert(poll_getc_cdc(&byte) && byte == 'a' && fake_irq && cdc_Out_len == 2);
    puts("PASS nonblocking readiness-to-consume is one IRQ-excluded operation");
    (void)irq_state;
    return 0;
}
'''

ROM_PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
typedef uint8_t BYTE;
typedef struct { BYTE BDCNT, BDSTAT; BYTE *BDADDR; } BDentry;
#define USB_EP0_BUFFER_SIZE 8
#define DTS 0x40
#define UOWN 0x80
#define DTSEN 8
static BYTE data[32], reply[8];
static const BYTE *usb_rom_ptr;
static size_t usb_rom_len;
static BYTE usb_rom_need_zlp;
static BDentry inbd = {0, 0, reply}, *EP0_Inbdp = &inbd;
static void (*in_handler)(void);
static void usb_send_rom(void);
static void usb_set_in_handler(int ep, void (*handler)(void)) { (void)ep; in_handler = handler; }
static void usb_unset_in_handler(int ep) { (void)ep; in_handler = 0; }
'''

ROM_TEST = r'''
int main(void) {
    (void)usb_rom_need_zlp;
    const unsigned int actual[] = {8, 16, 16, 18};
    const unsigned int requested[] = {8, 16, 32, 32};
    const unsigned int expected[] = {1, 2, 3, 3};
    for (unsigned int i = 0; i < 4; ++i) {
        unsigned int count = 1;
        usb_rom_ptr = data; usb_rom_len = actual[i];
        usb_start_rom_reply(requested[i]);
        while (in_handler && count < 6) { in_handler(); ++count; }
        assert(count == expected[i] && !in_handler);
        assert(inbd.BDCNT == (i == 2 ? 0 : actual[i] % 8 ? actual[i] % 8 : 8));
    }
    puts("PASS descriptor exact wLength, short tail, and early full-packet ZLP termination");
    return 0;
}
'''

CONFIG_PREFIX = r'''
#define MAX_CHIP_EP 16
#define USB_EP_NONE 0
#define USB_NUM_CONFIGURATIONS 1
#define USB_NUM_INTERFACES 2
#define ADDRESS_STATE 0x10
#define CONFIGURED_STATE 0x20
static BYTE dev[18] = {[17] = 1}, cfg[9] = {9, 2, 9, 0, 2}, strings[4] = {4, 3, 9, 4};
static const BYTE *usb_device_descriptor = dev, *usb_config_descriptor = cfg, *usb_string_descriptor = strings;
static const BYTE *usb_rom_ptr;
static size_t usb_rom_len;
static int usb_num_string_descriptors = 1;
static BYTE usb_addr_pending, usb_device_state;
static unsigned int usb_current_cfg = 1, usb_device_status;
static int usbrequesterrorflag;
#define flag_usb_RequestError() (usbrequesterrorflag = 1)
static void nothing(void) {}
static void (*sof_handler)(void) = nothing, (*class_setup_handler)(void) = nothing;
static void usb_set_address(void) {}
static void usb_set_in_handler(int ep, void (*handler)(void)) { (void)ep; (void)handler; }
static void user_configured_init(void) { regs[2] = 0x1d; sof_handler = nothing; class_setup_handler = nothing; }
static void usb_start_rom_reply(unsigned int n) { (void)n; }
'''

CONFIG_TEST = r'''
int main(void) {
    regs[1] = 5; regs[2] = 0x1d; usb_bdt[4].BDSTAT = 0x88;
    usb_device_state = CONFIGURED_STATE;
    packet[1] = USB_REQUEST_SET_CONFIGURATION;
    usb_handle_StandardDeviceRequest(&reqbd);
    assert(!error && !usb_current_cfg && usb_device_state == ADDRESS_STATE);
    assert(!regs[1] && !regs[2] && !usb_bdt[4].BDSTAT && !sof_handler && !class_setup_handler);
    packet[2] = 1; usb_handle_StandardDeviceRequest(&reqbd);
    assert(!error && usb_current_cfg == 1 && usb_device_state == CONFIGURED_STATE && regs[2] && sof_handler);
    packet[3] = 1; usb_handle_StandardDeviceRequest(&reqbd); assert(error);
    puts("PASS deconfiguration retires data endpoints/SOF, reconfiguration restores them, high value rejected");
    (void)usb_halt_saved;
    return 0;
}
'''

CONTROL_TEST = r'''
int main(void) {
    cfg[4] = 1; usb_device_state = CONFIGURED_STATE;
    packet[0] = 0x81; packet[1] = USB_REQUEST_GET_INTERFACE; packet[4] = 1; packet[6] = 1;
    usb_handle_StandardInterfaceRequest(&reqbd); assert(error);
    error = 0; packet[4] = 0; usb_handle_StandardInterfaceRequest(&reqbd); assert(!error && ack == 1);
    usb_device_state = ADDRESS_STATE; usb_handle_StandardInterfaceRequest(&reqbd); assert(error);
    error = 0; memset(packet, 0, 8); packet[1] = USB_REQUEST_SET_FEATURE; packet[2] = 1;
    usb_handle_StandardDeviceRequest(&reqbd); assert(error && !usb_device_status);
    error = 0; memset(packet, 0, 8); packet[1] = USB_REQUEST_SET_ADDRESS; packet[2] = 7; packet[6] = 1;
    usb_handle_StandardDeviceRequest(&reqbd); assert(error);
    error = 0; memset(packet, 0, 8); packet[1] = USB_REQUEST_GET_STATUS; packet[6] = 2;
    usb_handle_StandardDeviceRequest(&reqbd); assert(error);
    puts("PASS active-descriptor interface bounds, configuration state, standard lengths/directions and advertised wakeup capability");
    (void)usb_halt_saved;
    return 0;
}
'''

READY_PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint8_t BYTE;
typedef struct { BYTE BDCNT, BDSTAT; BYTE *BDADDR; } BDentry;
#define CONFIGURED_STATE 0x20
#define UOWN 0x80
#define CDC_BUFFER_SIZE 64
static BYTE usb_device_state = CONFIGURED_STATE;
static BDentry bd, *CDC_Outbdp = &bd;
static int recycled;
static int fake_irq = 1;
#define TestGlobalUsbInterruptEnable() fake_irq
#define DisableGlobalUsbInterrupt() (fake_irq = 0)
#define EnableUsbGlobalInterrupt() (fake_irq = 1)
static BYTE getda_cdc(void) { ++recycled; bd.BDSTAT = UOWN; return 0; }
'''

READY_TEST = r'''
int main(void) {
    bd.BDCNT = 0; bd.BDSTAT = 0;
    assert(!getOutReady() && recycled == 1 && bd.BDSTAT == UOWN && fake_irq);
    bd.BDCNT = 65; bd.BDSTAT = 0;
    assert(!getOutReady() && recycled == 2 && bd.BDSTAT == UOWN);
    bd.BDCNT = 64; bd.BDSTAT = 0;
    assert(getOutReady() && recycled == 2);
    usb_device_state = 0; assert(!getOutReady());
    puts("PASS OUT ZLP/invalid count recycled without advertising a byte or blocking later OUT");
    (void)getda_cdc;
    return 0;
}
'''


def compile_run(prefix, body, tests, name):
    tmpbase = Path(tempfile.gettempdir()) / "opencode"
    tmpbase.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="bp-usb-", dir=tmpbase) as tmp:
        c = Path(tmp) / (name + ".c")
        exe = c.with_suffix(".exe")
        c.write_text(prefix + body + tests, encoding="ascii")
        subprocess.run(["gcc", "-std=c99", "-Wall", "-Wextra", "-Werror", str(c), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--revision")
    parser.add_argument("--case", choices=("all", "line", "baud", "endpoint", "bounds", "halt", "setup", "cdc", "rom", "config", "control", "hardware", "ready"), default="all")
    args = parser.parse_args()
    cdc = source("cdc.c", args.revision)
    headers = source("usb_stack.h", args.revision) + source("cdc.h", args.revision)
    defines = "\n".join(line for line in headers.splitlines()
                        if re.match(r"#define (USB_b|USB_w|CDC_)", line))
    if "enum stopbits" in cdc:
        decl = cdc[cdc.index("enum stopbits"):cdc.index("#pragma udata")]
        decl += "struct _cdc_ControlLineState { int DTR, RTS; };\nstatic struct _cdc_ControlLineState cls;\nstatic BYTE LineStateUpdated;\n"
    else:
        decl = "static BYTE line_coding[7];\n"
    body = "\n".join(function(cdc, n) for n in ("initCDC", "cdc_setup", "cdc_set_line_coding_data"))
    for name, tests in (("line", LINE_TEST), ("baud", BAUD_TEST)):
        if args.case in ("all", name):
            compile_run(PREFIX + defines + "\n" + decl, body, tests, name)
    stack = source("usb_stack.c", args.revision)
    ep_defines = "\n".join(line for line in headers.splitlines()
                           if re.match(r"#define USB_(b|w|REQUEST_)", line))
    for name, tests in (("endpoint", EP_TEST), ("bounds", BOUNDS_TEST), ("halt", HALT_TEST)):
        if args.case in ("all", name):
            compile_run(EP_PREFIX + ep_defines + "\n",
                        function(stack, "usb_handle_StandardEndpointRequest"), tests, name)
    if args.case in ("all", "setup"):
        setup_defines = "\n".join(line for line in headers.splitlines()
                                  if re.match(r"#define USB_(bm|b|w)", line))
        compile_run(SETUP_PREFIX + setup_defines + "\n", function(stack, "usb_handle_setup"), SETUP_TEST, "setup")
    if args.case in ("all", "cdc"):
        names = ("CDC_Flush_In_Now", "CDCFlushOnTimeout", "setLock", "freeLock", "getinbuf_cdc",
                 "write_cdc", "putc_cdc", "getc_cdc", "getoutbuf_cdc", "read_cdc", "poll_getc_cdc", "peek_getc_cdc")
        compile_run(CDC_PREFIX, "\n".join(function(cdc, n) for n in names), CDC_TEST, "cdc")
    if args.case in ("all", "rom"):
        if "static void usb_start_rom_reply(unsigned int requested) {" in stack:
            start = function(stack, "usb_start_rom_reply")
        else:
            start = "static void usb_start_rom_reply(unsigned int requested) { (void)requested; usb_send_rom(); usb_set_in_handler(0, usb_send_rom); }"
        compile_run(ROM_PREFIX, start + "\n" + function(stack, "usb_send_rom"), ROM_TEST, "rom")
    if args.case in ("all", "config", "control"):
        all_defines = "\n".join(line for line in headers.splitlines()
                                if re.match(r"#define USB_(b|w|REQUEST_|.*DESCRIPTOR_TYPE)", line))
        for name, tests in (("config", CONFIG_TEST), ("control", CONTROL_TEST)):
            if args.case in ("all", name):
                body = function(stack, "usb_handle_StandardDeviceRequest")
                if name == "control":
                    body += "\n" + function(stack, "usb_handle_StandardInterfaceRequest")
                compile_run(EP_PREFIX + "#include <stddef.h>\n" + CONFIG_PREFIX + all_defines + "\n",
                            body, tests, name)
    if args.case in ("all", "hardware"):
        pic = source("picusb.h", args.revision).split("#if defined(PIC_24F)")[1]
        wake = next(line for line in pic.splitlines() if line.startswith("#define WakeupUsb()"))
        assert "ClearUsbInterruptFlag(USB_RESUM)" in wake
        assert re.search(r"#define ClearUsbInterruptFlag\(x\)\s+U1IR = x", pic)
        assert "volatile unsigned char BDCNT" in pic
        print("PASS PIC24 resume uses a word W1C acknowledgement, not bit-clear polling")
        if not args.revision:
            project = ROOT / "Firmware/busPirate.X/nbproject/configurations.xml"
            xml = ET.parse(project)
            listed = {node.text for node in xml.findall(".//itemPath")}
            for name in ("cdc.c", "cdc.h", "usb_stack.c", "usb_stack.h", "usb_stack_globals.h",
                         "picusb.h", "prj_usb_config.h", "descriptors.h"):
                ref = "../core_usb/" + name
                assert ref in listed and (project.parents[1] / ref).is_file(), ref
            print("PASS MPLAB lists every grouped USB source/header at an existing path")
    if args.case in ("all", "ready"):
        compile_run(READY_PREFIX, function(cdc, "getOutReady").replace("inline ", ""), READY_TEST, "ready")


if __name__ == "__main__":
    main()
