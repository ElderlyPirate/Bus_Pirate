"""Host-compile production USB functions with fake BDs; not an SIE simulator.

python Firmware/tests/test_usb.py [--revision ebefa34]
Requires gcc. Each assertion uses the production function text, not a model.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def source(path, revision):
    if revision:
        return subprocess.check_output(["git", "show", revision + ":Firmware/" + path],
                                       cwd=ROOT, text=True)
    usb = ROOT / "Firmware/core_usb"
    if not usb.exists():
        usb = ROOT / "Firmware/dp_usb"
    return (usb / Path(path).name).read_text()


def function(text, name):
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
    (void)usb_bdt;
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


def compile_run(prefix, body, tests, name):
    with tempfile.TemporaryDirectory(prefix="bp-usb-", dir=str(Path.home() / "AppData/Local/Temp/opencode")) as tmp:
        c = Path(tmp) / (name + ".c")
        exe = c.with_suffix(".exe")
        c.write_text(prefix + body + tests, encoding="ascii")
        subprocess.run(["gcc", "-std=c99", "-Wall", "-Wextra", "-Werror", str(c), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--revision")
    parser.add_argument("--case", choices=("all", "line", "baud", "endpoint", "bounds", "setup"), default="all")
    args = parser.parse_args()
    cdc = source("dp_usb/cdc.c", args.revision)
    headers = source("dp_usb/usb_stack.h", args.revision) + source("dp_usb/cdc.h", args.revision)
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
    stack = source("dp_usb/usb_stack.c", args.revision)
    ep_defines = "\n".join(line for line in headers.splitlines()
                           if re.match(r"#define USB_(b|w|REQUEST_)", line))
    for name, tests in (("endpoint", EP_TEST), ("bounds", BOUNDS_TEST)):
        if args.case in ("all", name):
            compile_run(EP_PREFIX + ep_defines + "\n",
                        function(stack, "usb_handle_StandardEndpointRequest"), tests, name)
    if args.case in ("all", "setup"):
        setup_defines = "\n".join(line for line in headers.splitlines()
                                  if re.match(r"#define USB_(bm|b|w)", line))
        compile_run(SETUP_PREFIX + setup_defines + "\n", function(stack, "usb_handle_setup"), SETUP_TEST, "setup")


if __name__ == "__main__":
    main()
