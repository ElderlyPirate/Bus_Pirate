"""Native checks for the live protocol descriptor accessors (no board I/O)."""
from pathlib import Path
import re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"build/protocol-access-tests"
OUT.mkdir(parents=True,exist_ok=True)
shutil.copyfile(ROOT/"core.h",OUT/"core.h")
(OUT/"configuration.h").write_text("".join("#define BP_ENABLE_%s_SUPPORT\n" % name for name in
    ("1WIRE","UART","I2C","SPI","RAW_2WIRE","RAW_3WIRE","PC_AT_KEYBOARD","HD44780","PIC","DIO")))
source=(ROOT/"core.c").read_text()
functions=[]
for name in ("bp_current_protocol","bp_protocol_by_index"):
    match=re.search(r"const bus_pirate_protocol_t \*"+name+r"\([^)]*\)\s*\{[^}]+\}",source)
    if not match:raise RuntimeError("Accessor shape changed; update harness extraction")
    functions.append(match[0])
(OUT/"test.c").write_text(r'''
#include <assert.h>
#include <stdio.h>
#include "core.h"
static bus_pirate_protocol_t enabled_protocols[ENABLED_PROTOCOLS_COUNT];
static bus_pirate_configuration_t bus_pirate_configuration;
''' + "\n".join(functions) + r'''
static uint16_t changing_send(uint16_t data) {
    bus_pirate_configuration.bus_mode = BP_SPI;
    return data ^ 0xa55a;
}
int main(void) {
    unsigned checks=0;
    for (unsigned repeat=0; repeat<100; repeat++) {
        for (unsigned i=0; i<ENABLED_PROTOCOLS_COUNT; i++) {
            bus_pirate_configuration.bus_mode=i;
            const bus_pirate_protocol_t *expected=&enabled_protocols[i];
            assert(bp_current_protocol()==expected);
            assert(bp_protocol_by_index(i)==expected);
            checks+=2;
        }
    }
    enabled_protocols[BP_HIZ].send=changing_send;
    bus_pirate_configuration.bus_mode=BP_HIZ;
    assert(bp_current_protocol()->send(0x1234)==(0x1234 ^ 0xa55a));
    assert(bp_current_protocol()==&enabled_protocols[BP_SPI]);
    puts("Protocol descriptors: live mode/index selection and callback-driven mode change PASS");
    printf("%u pointer identity checks\n",checks);
}
''')
exe=OUT/"protocol-access.exe"
subprocess.run(["gcc","-std=c99","-Wall","-Wextra","-Werror","-I",str(OUT),str(OUT/"test.c"),"-o",str(exe)],check=True)
subprocess.run([str(exe)],check=True)
