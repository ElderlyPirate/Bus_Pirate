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
    parser.add_argument("--case", choices=("all", "line", "baud"), default="all")
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


if __name__ == "__main__":
    main()
