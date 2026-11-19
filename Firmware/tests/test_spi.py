"""Production binary SPI dispatch/CS paths with register and host-I/O mocks.

Checks RA-3 direction/latch ordering, not electrical edges or PIC24 timing.
python Firmware/tests/test_spi.py [--revision commit] [--case manual|auto|preserve]
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

from test_core_system import function

ROOT = Path(__file__).resolve().parents[2]
TEMP = Path.home() / "AppData/Local/Temp/opencode"


def source(path, revision):
    candidates = (path, "protocol_binary/" + path) if path == "binary_io.c" else (path,)
    for candidate in candidates:
        if revision:
            result = subprocess.run(
                ["git", "show", revision + ":Firmware/" + candidate], cwd=ROOT,
                text=True, encoding="utf-8", capture_output=True)
            if not result.returncode:
                return result.stdout
        elif (ROOT / "Firmware" / candidate).exists():
            return (ROOT / "Firmware" / candidate).read_text(encoding="utf-8")
    raise RuntimeError("Missing source: " + path)


PREFIX = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#define ON 1
#define OFF 0
#define YES 1
#define NO 0
#define LOW 0
#define HIGH 1
#define INPUT 1
#define OUTPUT 0
#define SPI_CLOCK_IDLE_LOW 0
#define SPI_CLOCK_IDLE_HIGH 1
#define SPI_TRANSITION_FROM_IDLE_TO_ACTIVE 0
#define SPI_TRANSITION_FROM_ACTIVE_TO_IDLE 1
#define SPI_SAMPLING_ON_DATA_OUTPUT_MIDDLE 0
#define SPI_SAMPLING_ON_DATA_OUTPUT_END 1
#define SPI_SNIFF_ALWAYS true
#define SPI_SNIFF_ON_CS_LOW false
#ifdef BUSPIRATEV4
#define CS (1u << 4)
#define AUX (1u << 5)
#else
#define CS (1u << 6)
#define AUX (1u << 10)
#endif
static struct { unsigned high_impedance, speed; } mode_configuration;
static struct { unsigned clock_polarity, clock_edge, data_sample_timing, cs_line_state; } spi_state;
static const uint8_t spi_bus_speed[12] = {};
static bool spi_bulk_fault;
static unsigned odc, regulator, pullup, aux_level;
static unsigned cfg, peripheral, opcode, seed, readpos, wordpos, acks, writes, reads;
static uint16_t length_write, length_read, baseline_lat, baseline_dir;
struct Event { char kind; uint16_t value; };
static std::vector<Event> events;
struct Register {
  char kind;
  uint16_t value;
  void store(uint16_t v) { value=v; events.push_back({kind,v}); }
  void operator|=(unsigned v) { store(value | v); }
  void operator&=(unsigned v) { store(value & v); }
  operator uint16_t() const { return value; }
};
static Register latch={'L',0}, direction={'D',0};
#define IOLAT latch
#define IODIR direction
struct Pin {
  unsigned mask;
  void operator=(unsigned v) { if(v) latch|=mask; else latch&=~mask; }
};
static Pin SPICS={CS}, BP_AUX0={AUX};
struct AuxDirection {
  void operator=(unsigned v) { if(v) direction|=AUX; else direction&=~AUX; }
};
static AuxDirection BP_AUX0_DIR;
static void bp_set_voltage_regulator_state(bool v) { regulator=v; }
static void bp_set_pullup_state(bool v) { pullup=v; }
static void assert_selected(void) {
  assert(!(latch.value & CS) && !(direction.value & CS));
}
static void spi_setup(uint8_t) {
  latch.value|=CS; direction.value&=~CS;
  odc=mode_configuration.high_impedance ? CS : 0;
}
static void spi_disable_interface(void) {}
static void spi_sniffer(bool, bool) { assert(false); }
static unsigned bp_binary_io_pullup_control(uint8_t) { assert(false);return 0; }
static uint8_t spi_write_byte(uint8_t) { assert(false);return 0; }
static void bp_delay_us(unsigned) {}
static void user_serial_wait_transmission_done(void) {}
static unsigned FTDI_CTS;
static void user_serial_ringbuffer_flush(void) {}
static void user_serial_transmit_character(uint8_t) { assert(false); }
#define MSG_SPI_MODE_IDENTIFIER ((void)0)
#define REPORT_IO_SUCCESS() (++acks)
#define REPORT_IO_FAILURE() assert(false)
static void spi_read_write_io(bool);
static void spi_write_from_host(uint16_t n) {
  if(opcode==4) assert_selected();
  else assert(latch.value==baseline_lat && direction.value==baseline_dir);
  writes+=n;
}
static void spi_read_to_host(uint16_t n) {
  if(opcode==4) assert_selected();
  else assert(latch.value==baseline_lat && direction.value==baseline_dir);
  reads+=n;REPORT_IO_SUCCESS();
}
static uint16_t user_serial_read_big_endian_word(void) {
  assert(wordpos<2);return wordpos++ ? length_read : length_write;
}
static uint8_t user_serial_read_byte(void) {
  switch(readpos++) {
  case 0: return cfg;
  case 1:
    latch.value=(uint16_t)(seed*21845u);
    direction.value=(uint16_t)((3-seed)*21845u);
    return peripheral;
  case 2:
    assert(regulator==!!(peripheral&8) && pullup==!!(peripheral&4));
    assert((latch.value&AUX)==((peripheral&2)?AUX:0));
    assert(!(direction.value&AUX));
    if((peripheral&1) && mode_configuration.high_impedance) {
      assert(direction.value&CS);
      assert((latch.value&CS)==((seed*21845u)&CS));
    } else {
      assert(!(direction.value&CS));
      assert((latch.value&CS)==((peripheral&1)?CS:0));
    }
    baseline_lat=latch.value;baseline_dir=direction.value;events.clear();
    return opcode;
  case 3: return 0;
  default: assert(false);return 0;
  }
}
'''

TEST = r'''
static void expected_cs(std::vector<Event>& expected, uint16_t& l, uint16_t& d, bool high) {
  if(high && mode_configuration.high_impedance) {
    d|=CS;expected.push_back({'D',d});l|=CS;expected.push_back({'L',l});
  } else {
    if(high) l|=CS;else l&=~CS;
    expected.push_back({'L',l});d&=~CS;expected.push_back({'D',d});
  }
}
int main(int argc, char** argv) {
  assert(argc==2);unsigned test=std::strtoul(argv[1],nullptr,10),checks=0;
  for(unsigned impedance : {1u,0u})
  for(peripheral=0x4f;peripheral>=0x40;--peripheral)
  for(seed=0;seed<4;++seed)
  for(unsigned request=0;request<12;++request) {
    opcode=request<2 ? 2+request : request<7 ? 4 : 5;
    if((test==0 && opcode>3) || (test==1 && opcode!=4) ||
       (test==2 && opcode!=5)) continue;
    unsigned shape=request<2 ? 0 : request<7 ? request-2 : request-7;
    length_write=shape==0 ? 1 : shape==1 ? 0 : shape==2 ? 3 : 0;
    length_read=shape==0 ? 0 : shape==1 ? 1 : shape==2 ? 5 : 0;
    if(shape==3) { length_write=1;length_read=2; }
    cfg=impedance ? 0x82 : 0x8a;
    readpos=wordpos=acks=writes=reads=0;
    events.clear();spi_enter_binary_io();
    assert(readpos==4 && acks==3);
    assert(odc==(impedance ? CS : 0));
    std::vector<Event> expected;
    uint16_t l=baseline_lat,d=baseline_dir;
    if(opcode==2 || opcode==3) expected_cs(expected,l,d,opcode==3);
    if(opcode==4 && (length_write || length_read)) {
      expected_cs(expected,l,d,false);expected_cs(expected,l,d,true);
    }
    assert(latch.value==l && direction.value==d);
    assert(events.size()==expected.size());
    for(unsigned i=0;i<expected.size();++i) {
      assert(events[i].kind==expected[i].kind && events[i].value==expected[i].value);
      uint16_t old=events[i].kind=='L' ? baseline_lat : baseline_dir;
      assert((events[i].value&~CS)==(old&~CS));
    }
    assert(writes==(opcode>=4 ? length_write : 0));
    assert(reads==(opcode>=4 ? length_read : 0));
    assert(wordpos==(opcode>=4 ? 2 : 0));
    ++checks;
  }
  printf("PASS %u production SPI cases: 0x4x contract, CS ordering, pad direction, sibling preservation, framing/no-op\n",checks);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision")
    parser.add_argument("--case", choices=("manual", "auto", "preserve"))
    args = parser.parse_args()
    spi = source("protocol_spi/spi.c", args.revision)
    binary = source("binary_io.c", args.revision)
    enums = "\n".join(re.findall(r"typedef enum \{[^}]*\} spi_(?:base_command|command)_t;", spi))
    assert enums.count("typedef enum") == 2
    helper = function(spi, "spi_binary_set_cs") if "static void spi_binary_set_cs(" in spi else ""
    code = (PREFIX + enums + function(binary, "bp_binary_io_peripherals_set") +
            helper + function(spi, "spi_read_write_io") +
            function(spi, "spi_enter_binary_io") + TEST)
    for board in ("BUSPIRATEV4", "BUSPIRATEV3"):
        with tempfile.TemporaryDirectory(prefix="bp-spi-", dir=TEMP) as tmp:
            cpp = Path(tmp) / "spi.cpp"
            exe = cpp.with_suffix(".exe")
            cpp.write_text("#define " + board + "\n" + code, encoding="ascii")
            subprocess.run(["g++", "-std=c++17", "-O1", "-Wall", "-Wextra", "-Werror",
                            "-Wno-unused-function", "-Wno-unused-variable",
                            str(cpp), "-o", str(exe)], check=True)
            for n, case in enumerate(("manual", "auto", "preserve")):
                if args.case and args.case != case:
                    continue
                subprocess.run([str(exe), str(n)], check=True, timeout=5)
                print("PASS", board, case, flush=True)


if __name__ == "__main__":
    main()
