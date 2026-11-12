"""Production manual ADC transaction with deterministic register histories.

This fixture models a pending old-channel completion, not PIC24 electrical
timing. Actual startup spacing and the first-read artifact have target/bench
evidence; no data-discard retry or arbitrary settling sleep is modeled here.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from test_core_system import function

ROOT = Path(__file__).resolve().parents[2]
TEMP = Path.home() / "AppData/Local/Temp/opencode"

PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define ON 1
#define OFF 0
static struct { unsigned ADON,SAMP,DONE; } control;
static uint16_t mux,result,latched,values[16];
static unsigned observed_on,old_pending,old_left,phase,left,accesses,nops,mux_while_on;
static void step(void) {
 assert(++accesses<250);
 if (!control.ADON) {
  observed_on=old_pending=phase=0;
  control.SAMP=control.DONE=0;
  return;
 }
 if (!observed_on) { observed_on=1; latched=mux; }
 if (old_pending) {
  if (!--old_left) {
   result=values[latched]; control.SAMP=0; control.DONE=1; old_pending=0;
  }
  return;
 }
 if (!phase && control.SAMP) { phase=1;left=3;latched=mux; }
 if (phase==1) {
  if (!--left) { control.SAMP=0;control.DONE=0;phase=2;left=2; }
 } else if (phase==2 && !--left) {
  result=values[latched];control.DONE=1;phase=3;
 }
}
static void *controls(void) { step(); return &control; }
static uint16_t *selection(void) { step();mux_while_on+=!!control.ADON;return &mux; }
static uint16_t *output(void) { step();return &result; }
#define AD1CON1bits (*( __typeof__(control) *)controls())
#define AD1CHS (*selection())
#define ADC1BUF0 (*output())
#define Nop() do { assert(control.ADON);++nops;step(); } while(0)
'''

TEST = r'''
int main(void) {
 unsigned checks=0;
 const unsigned seeds[]={0,1,511,1023};
 for (unsigned history=0;history<2;++history)
 for (unsigned seed=0;seed<4;++seed)
 for (unsigned prior=0;prior<16;++prior)
 for (unsigned offset=1;offset<=16;++offset) {
  unsigned requested=(prior+offset)&15;
  for (unsigned i=0;i<16;++i) values[i]=(i*61+seeds[seed])&1023;
  control.ADON=observed_on=1;control.SAMP=control.DONE=0;
  mux=latched=prior;result=0xffff;old_pending=!history;old_left=4;
  phase=accesses=nops=mux_while_on=0;
  uint16_t got=bp_read_adc(requested);
  assert(got==values[requested]);
  assert(!mux_while_on && mux==requested && control.ADON);
  assert(!control.SAMP && control.DONE && nops==5);
  ++checks;
 }
 printf("PASS %u manual ADC reads: old-channel startup retired, mux selected while off, fresh completion and enabled exit\n",checks);
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--revision")
    args = p.parse_args()
    if args.revision:
        text = subprocess.check_output(["git", "show", args.revision + ":Firmware/core_system/base.c"],
                                       cwd=ROOT, text=True, encoding="utf-8")
    else:
        text = (ROOT / "Firmware/core_system/base.c").read_text(encoding="utf-8")
    body = function(text, "bp_read_adc")
    with tempfile.TemporaryDirectory(prefix="bp-core-adc-", dir=TEMP) as tmp:
        c = Path(tmp) / "adc.c"
        exe = c.with_suffix(".exe")
        c.write_text(PREFIX + body + TEST, encoding="ascii")
        subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Wextra", "-Werror", str(c), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
