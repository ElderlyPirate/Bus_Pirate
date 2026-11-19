"""Exercise the production raw-wire bulk handler with native serial/wire mocks."""
import argparse
from pathlib import Path
import shutil
import subprocess

from test_core_system import function


ROOT = Path(__file__).resolve().parents[2]
PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#define YES 1
#define NO 0
#define BINARY_IO_2_WIRES 0
#define BINARY_IO_3_WIRES 1
static struct { unsigned little_endian, numbits; } mode_configuration;
static struct { unsigned wires; } io_state;
static uint8_t input[17], sampled[16], wire[16], output[33];
static unsigned used, replies, reads, writes, wire_bytes, clocks;
static uint8_t user_serial_read_byte(void) {
  assert(replies >= 1 && used < sizeof input);
  return input[used++];
}
static void user_serial_transmit_character(uint8_t value) {
  assert(replies < sizeof output); output[replies++] = value;
}
#define REPORT_IO_SUCCESS() user_serial_transmit_character(1)
static uint16_t bp_reverse_integer(uint16_t value, unsigned bits) {
  uint16_t result = 0;
  while(bits--) { result = (result << 1) | (value & 1); value >>= 1; }
  return result;
}
static uint16_t bitbang_read_with_write(uint16_t value) {
  assert(reads < 16 && wire_bytes < 16);
  wire[wire_bytes++] = value; clocks += 8;
  return sampled[reads++];
}
static void bitbang_write_value(uint16_t value, bool reverse) {
  assert(!reverse && wire_bytes < 16);
  wire[wire_bytes++] = value; ++writes; clocks += 8;
}
'''

TEST = r'''
int main(void) {
  unsigned cases = 0;
  for(unsigned wires = 0; wires < 2; ++wires)
    for(unsigned lsb = 0; lsb < 2; ++lsb)
      for(unsigned count = 1; count <= 16; ++count) {
        mode_configuration.little_endian = lsb;
        mode_configuration.numbits = 8;
        io_state.wires = wires;
        used = replies = reads = writes = wire_bytes = clocks = 0;
        for(unsigned i = 0; i < count; ++i) {
          input[i] = (uint8_t)(0x3c + i * 17);
          sampled[i] = (uint8_t)(0xc3 ^ (i * 29));
        }
        input[count] = 0x7f;
        handle_bulk_byte_transfer(0x10 | (count - 1));
        assert(used == count && wire_bytes == count && clocks == count * 8);
        assert(replies == count + 1 && output[0] == 1);
        assert(reads == (wires ? count : 0));
        assert(writes == (wires ? 0 : count));
        for(unsigned i = 0; i < count; ++i) {
          unsigned sent = lsb ? bp_reverse_integer(input[i], 8) : input[i];
          unsigned got = lsb ? bp_reverse_integer(sampled[i], 8) : sampled[i];
          assert(wire[i] == sent);
          assert(output[i + 1] == (wires ? got : 1));
        }
        assert(user_serial_read_byte() == 0x7f);
        ++cases;
      }
  printf("PASS %u bulk cases: x1..x16, 2/3 wires, both orders, ACK/sample framing and no extra clocks\n", cases);
  return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    args = parser.parse_args()
    path = "Firmware/protocol_binary/binary_io.c"
    text = subprocess.check_output(["git", "show", args.revision + ":" + path], cwd=ROOT,
                                   text=True, encoding="utf-8") if args.revision else (ROOT / path).read_text(encoding="utf-8")
    code = PREFIX + function(text, "handle_bulk_byte_transfer") + TEST
    for board in ("V4", "V3"):
        out = ROOT / "Firmware/build/test-binary-bulk-transfer" / board
        out.mkdir(parents=True, exist_ok=True)
        c = out / "bulk.c"
        exe = out / "bulk.exe"
        c.write_text(code, encoding="ascii")
        linker = ["-fuse-ld=bfd"] if shutil.which("ld.bfd") else []
        subprocess.run(["gcc", *linker, "-std=gnu99", "-Wall", "-Wextra", "-Werror",
                        "-DBUSPIRATE" + board, str(c), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=10)


if __name__ == "__main__":
    main()
