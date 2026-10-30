"""Native tests of the actual packed-message reader on both address models.

Only PIC24 table reads and serial output are mocked. Test artifacts stay in
build/. This does not emulate USB timing or prove physical board behavior.
"""
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build/message-tests"
OUT.mkdir(parents=True, exist_ok=True)
shutil.copyfile(ROOT/"messages.c", OUT/"messages.c")
shutil.copyfile(ROOT/"messages.h", OUT/"messages.h")
(OUT/"configuration.h").write_text("")
for version in (3,4):
    (OUT/("messages_v%d.h" % version)).write_text(
        "#define BPMSG1135 ((void)0)\n" +
        "".join("#define HLP%d ((void)0)\n" % i for i in range(1000,1023)))
(OUT/"base.h").write_text(r'''
#include <stdint.h>
#include <stddef.h>
extern uint8_t TBLPAG;
uint16_t mock_low(uint16_t address);
uint16_t mock_high(uint16_t address);
void user_serial_transmit_character(char value);
uint8_t user_serial_read_byte(void);
#define __builtin_tblrdl(address) mock_low((uint16_t)(address))
#define __builtin_tblrdh(address) mock_high((uint16_t)(address))
#define LO8(value) ((uint8_t)(value))
#define HI8(value) ((uint8_t)((value) >> 8))
#define bpBR do { user_serial_transmit_character('\r'); user_serial_transmit_character('\n'); } while (0)
''')
(OUT/"test.c").write_text(r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "messages.h"
#include "messages.c"
uint8_t TBLPAG;
static uint8_t flash[0x50000], output[2048], input;
static size_t output_length;
static unsigned checks;
static size_t position(uint16_t address) {
    uint32_t full = ((uint32_t)TBLPAG << 16) | address;
    size_t offset = (full / 2) * 3;
    assert(!(full & 1) && offset + 2 < sizeof(flash));
    return offset;
}
uint16_t mock_low(uint16_t address) {
    size_t p = position(address);
    return flash[p] | ((uint16_t)flash[p+1] << 8);
}
uint16_t mock_high(uint16_t address) { return flash[position(address)+2]; }
void user_serial_transmit_character(char value) {
    assert(output_length < sizeof(output));
    output[output_length++] = (uint8_t)value;
    TBLPAG = 0x73; /* A callee may use table memory too. */
}
uint8_t user_serial_read_byte(void) { return input; }
static void run(uint32_t address, const uint8_t *data, size_t length, int line) {
    size_t p = (address / 2) * 3;
    memcpy(flash+p, data, length);
    flash[p+length] = 0;
    output_length = 0;
    TBLPAG = 0x5a;
    if (line) bp_message_write_line(address);
    else bp_message_write_buffer(address);
    /* write_buffer restores its page; line then emits CR/LF via the callee. */
    assert(TBLPAG == (line ? 0x73 : 0x5a));
    assert(output_length == length + (line ? 2 : 0));
    assert(!memcmp(output, data, length));
    if (line) assert(output[length] == '\r' && output[length+1] == '\n');
    checks++;
}
int main(void) {
    uint8_t data[1024];
    for (size_t i=0; i<sizeof(data); i++) data[i] = 1 + (i % 255);
    for (size_t n=0; n<=sizeof(data); n++) {
        run(0x200, data, n, 0);
        run(0x8000, data, n, 1);
    }
#ifndef BUSPIRATEV3
    for (size_t n=0; n<20; n++) {
        run(0xfffc, data, n, 0);
        run(0x1fffe, data, n, 1);
    }
#endif
    input='y'; assert(agree());
    input='Y'; assert(agree());
    input='n'; assert(!agree());
    input=0xff; assert(!agree());
    printf("Packed-message reader: %u cases PASS\n", checks);
}
''')
for version in (3,4):
    exe=OUT/("messages_v%d.exe" % version)
    subprocess.run(["gcc","-std=c99","-Wall","-Wextra","-Werror",
                    "-DBUSPIRATEV%d" % version,"-I",str(OUT),str(OUT/"test.c"),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
