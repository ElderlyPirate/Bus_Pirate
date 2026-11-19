"""Native production-source ATR reset/read ordering and decode contracts."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'Firmware/protocol_wire/raw2wire.c').read_text(encoding='utf-8')
if len(sys.argv) > 1:
    source = subprocess.check_output(
        ['git', 'show', sys.argv[1] + ':Firmware/protocol_wire/raw2wire.c'],
        cwd=ROOT, text=True, encoding='utf-8')


def function(name):
    start = re.search(r'void ' + name + r'\([^;]*?\) \{', source).start()
    end = source.index('\n}', start) + 2
    return source[start:end]


prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#define LOW 0
#define HIGH 1
#define YES 1
#define ATR_HEADER_LENGTH 4
enum { RAW2WIRE_MACRO_MENU, RAW2WIRE_MACRO_ISO_7816_3_ATR,
       RAW2WIRE_MACRO_ISO_7816_3_ATR_PARSE };
typedef int raw2wire_macro_identifier_t;
struct { int little_endian; } mode_configuration;
static int cs, reads, ticks, delay, triggered, printed, decimals;
static uint8_t reply[4] = {0xa0, 0x09, 0x08, 0x4a};
static uint32_t values[2];
static void bitbang_set_cs(int level) {
  if (!level) { assert(!triggered); triggered = 1; }
  else { assert(triggered && ticks == 1 && delay == 255); }
  cs = level;
}
static void bp_delay_us(int us) { assert(cs == LOW); delay += us; }
static void bitbang_advance_clock_ticks(int n) {
  assert(cs == LOW && delay == 255); ticks += n;
}
static uint16_t bitbang_read_value(bool unused) {
  (void)unused; assert(cs == HIGH && reads < 4); return reply[reads++];
}
static uint8_t bp_reverse_byte(uint8_t v) {
  uint8_t r = 0; for (int i=0;i<8;i++) { r=(r<<1)|(v&1); v>>=1; } return r;
}
static void bp_write_hex_byte(uint8_t v) { (void)v; printed++; }
static void bp_write_dec_dword(uint32_t v) { assert(decimals<2); values[decimals++]=v; }
'''
tables = source[source.index('static const uint32_t DATA_UNIT_LENGTH_TABLE'):
                source.index('/**')]
messages = '\n'.join('#define ' + name + ' ((void)0)' for name in
                     sorted(set(re.findall(r'\bMSG_\w+|\bbpSP|\bbpBR', source))))
body = r'''
static void reset(void) {
  cs=HIGH; reads=ticks=delay=triggered=printed=decimals=0;
}
int main(void) {
  for (int endian=0;endian<2;endian++) {
    mode_configuration.little_endian=endian;
    reset(); raw2wire_run_macro(1);
    assert(triggered && ticks==1 && reads==4 && printed==4 && cs==HIGH);
    reset(); raw2wire_run_macro(2);
    assert(!triggered && !ticks && !delay && reads==4 && printed==4 && cs==HIGH);
  }
  mode_configuration.little_endian=0;
  for (unsigned code=0;code<8;code++) {
    reset(); reply[1]=8|code; raw2wire_run_macro(1);
    assert(decimals==2 && values[0]==2 && values[1]==(128u<<code));
  }
  reset(); raw2wire_run_macro(0); assert(!reads && !triggered);
  reset(); raw2wire_run_macro(99); assert(!reads && !triggered);
  puts("PASS ATR reset-low/clock/release-high before four reads; CS stays high; parse-only unchanged; endian and length codes");
}
'''
with tempfile.TemporaryDirectory(prefix='bp-atr-', dir=ROOT / 'Firmware/build') as tmp:
    path = Path(tmp) / 'test.c'
    exe = Path(tmp) / 'test.exe'
    path.write_text(prefix + messages + '\n' + tables +
                    '\nstatic void trigger_atr(void);\nstatic void read_atr_header(void);\n' +
                    '\n'.join(function(name) for name in
                              ('trigger_atr', 'read_atr_header', 'raw2wire_run_macro')) + body,
                    encoding='utf-8')
    subprocess.run(['gcc', '-std=c99', '-Wall', '-Werror', str(path), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
