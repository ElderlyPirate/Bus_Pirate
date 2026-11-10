"""Native checks for the PIC ICSP bit engine (no board I/O).

Compiles the extracted `read_bits`/`write_bits` from Firmware/pic.c against
stub pin primitives, so the two historical failures are pinned without a
board or a scope:

  * `read_bits` must assemble the sampled stream LSB-first.  The audit
    finding P1 was a shift count of `1 << (bits - 1)` (8192 for the 14-bit
    device-ID read) instead of `bits - 1`; per DS70000157G "SL" a shift
    count above 15 loads the destination with 0, so every device-ID read
    returned 0x0000/0x00.
  * `write_bits` must clock out exactly `bits` data bits, LSB-first, from a
    16-bit mask.  The mask used to be `uint8_t`, so bits 8..13/14 of a
    14/16-bit transfer wrapped to 0 and were sent as 1.

Run directly (like the sibling tests):  python Firmware/tests/test_pic.py
"""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build/pic-tests"
OUT.mkdir(parents=True, exist_ok=True)


def extract(name, signature):
    source = (ROOT / "pic.c").read_text()
    match = re.search(re.escape(signature) + r"\s*\{", source)
    if not match:
        raise RuntimeError("shape changed: %s not found; update harness" % name)
    start = match.start()
    depth = 0
    for i in range(start, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise RuntimeError("unterminated %s" % name)


read_bits = extract("read_bits", "uint16_t read_bits(const uint8_t bits)")
write_bits = extract("write_bits", "void write_bits(const uint16_t data, const uint8_t bits)")

(OUT / "test.c").write_text(r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define CLK 0x02
#define MOSI 0x01
#define HIGH 1
#define PIC_PIN_DELAY 100

/* read_bits scripts: the value bitbang_read_pin() returns, in call order. */
static int read_queue[64];
static int read_pos;
int bitbang_read_pin(int pin) { (void)pin; return read_queue[read_pos++]; }

/* write_bits sink: the level passed to bitbang_set_pins(), in call order. */
static int driven[64];
static int driven_pos;
static int half_period_ok = 1;
void bitbang_set_pins(int high, int pin, int delay) {
    (void)pin;
    if (delay != PIC_PIN_DELAY / 4) half_period_ok = 0;
    driven[driven_pos++] = high ? 1 : 0;
}
void bitbang_set_pins_high(int pins, int delay) { (void)pins; (void)delay; }
void bitbang_set_pins_low(int pins, int delay) { (void)pins; (void)delay; }
''' + read_bits + "\n" + write_bits + r'''

int main(void) {
    unsigned checks = 0;

    /* read_bits: sample k lands at bit k (LSB-first); the old 1<<(bits-1)
       shift made every sample land out of range and collapse to 0. */
    for (int bits = 1; bits <= 16; bits++) {
        read_pos = 0;
        for (int k = 0; k < bits; k++)
            read_queue[k] = (k % 2) ? 1 : 0;
        uint16_t expected = 0;
        for (int k = 0; k < bits; k++)
            if (k % 2) expected |= (uint16_t)(1u << k);
        uint16_t got = read_bits((uint8_t)bits);
        assert(got == expected);
        checks++;
    }

    /* all-high samples -> all ones, all-low -> zero (the bug gave zero). */
    read_pos = 0;
    for (int k = 0; k < 14; k++) read_queue[k] = 1;
    assert(read_bits(14) == 0x3FFF);
    read_pos = 0;
    for (int k = 0; k < 14; k++) read_queue[k] = 0;
    assert(read_bits(14) == 0x0000);
    checks += 2;

    /* write_bits: exactly `bits` data bits, LSB-first, 16-bit mask.
       A zero write must drive all zeros; the old uint8_t mask sent bits
       8..13/14 as 1. */
    for (int bits = 1; bits <= 16; bits++) {
        driven_pos = 0;
        write_bits(0x0000, (uint8_t)bits);
        assert(driven_pos == bits);
        for (int k = 0; k < bits; k++)
            assert(driven[k] == 0);
        checks++;
    }
    driven_pos = 0;
    write_bits(0xAAAA, 14);
    assert(driven_pos == 14);
    for (int k = 0; k < 14; k++)
        assert(driven[k] == ((0xAAAA >> k) & 1));
    driven_pos = 0;
    write_bits(0xFFFF, 16);
    for (int k = 0; k < 16; k++)
        assert(driven[k] == 1);
    assert(half_period_ok);
    checks += 3;

    puts("PIC bit engine: LSB-first reads and a 16-bit write mask PASS");
    printf("%u checks\n", checks);
    return 0;
}
''')

exe = OUT / "pic-tests.exe"
subprocess.run(["gcc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                str(OUT / "test.c"), "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)