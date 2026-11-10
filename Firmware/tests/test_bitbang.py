"""Native production-source/MMIO contracts, not a PIC24 simulator.

python Firmware/tests/test_bitbang.py [--revision 237d175] [--case name]
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def source(name, revision=None):
    for directory in ('core_bitbang/', ''):
        path='Firmware/'+directory+name
        if revision:
            r=subprocess.run(['git','show',revision+':'+path],cwd=ROOT,capture_output=True,text=True)
            if not r.returncode: return r.stdout
        elif (ROOT/path).exists(): return (ROOT/path).read_text()
    raise RuntimeError(name)


PREFIX=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#define ON 1
#define OFF 0
#define HIGH 1
#define LOW 0
#define MOSI 1u
#define CLK 2u
#define MISO 4u
#define CS 8u
#define BP_USE_HARDWARE_DELAY_TIMER
typedef struct {uint16_t numbits,high_impedance;} mode_configuration_t;
mode_configuration_t mode_configuration={8,ON};
static uint16_t lat,dir=15;
static uint32_t elapsed,release_after,nops,reads,delays;
static uint16_t stamp;
static unsigned clock_pin(void) {++reads; return elapsed>=release_after;}
uint16_t timer(void) {++elapsed;return stamp++;}
static uint16_t port(void) {return MOSI|MISO|(clock_pin()?CLK:0);}
#define BP_CLK clock_pin()
#define BP_MOSI 1
#define TMR1 timer()
#define IOLAT lat
#define IODIR dir
#define IOPOR port()
#define Nop() (++nops)
static void delay_short_inline(uint16_t us) {if(us){elapsed+=us*2-1;stamp+=us*2-1;}++delays;}
'''

CASES={
'setup':r'''
int main(void){
 for(unsigned pins=2;pins<=3;++pins)for(unsigned speed=0;speed<8;++speed){
  bitbang_setup(pins,speed);
  assert(delay_profile==&BITBANG_DELAYS[speed>3?3:speed]);
  assert(miso_pin==(pins==3?MISO:MOSI));
 }
 puts("PASS explicit setup selects pin/profile and clamps high speed indices");
}''',
'bound':r'''
int main(void){
 bitbang_setup(2,BITBANG_SPEED_5KHZ);
 for(unsigned start=0;start<65536;start+=7919){
  stamp=start;elapsed=reads=0;release_after=1000000;
  assert(!bitbang_wait_for_clock_release());
  assert(reads==25001 && elapsed==25000);
  for(unsigned hold=0;hold<=24900;hold+=100){
   stamp=start;elapsed=reads=0;release_after=hold;
   assert(bitbang_wait_for_clock_release());
  }
 }
 bitbang_clk_stretch_timeouts=65534;
 elapsed=0;release_after=1000000;bitbang_wait_for_clock_release();
 assert(bitbang_clock_stretch_timeouts()==65535);
 elapsed=0;bitbang_wait_for_clock_release();
 assert(bitbang_clock_stretch_timeouts()==65535);
 puts("PASS finite 25000-iteration bound, synthetic releases and saturating count");
}''',
'waiting':r'''
int main(void){
 bitbang_setup(2,BITBANG_SPEED_5KHZ);
 for(unsigned operation=0;operation<6;++operation){
  bitbang_i2c_clear_error();elapsed=0;release_after=1000000;
  dir=0;lat=0;
  switch(operation){
  case 0:assert(!bitbang_i2c_start_wait(BITBANG_I2C_START_ONE_SHOT));break;
  case 1:assert(!bitbang_i2c_stop_wait());break;
  case 2:assert(!bitbang_i2c_write_bit(LOW));break;
  case 3:bitbang_write_value(0xa5,true);break;
  case 4:assert(bitbang_read_value(true)==65535);break;
  case 5:assert(bitbang_read_bit(true));break;
  }
  assert(bitbang_i2c_error() && (dir&(CLK|MOSI))==(CLK|MOSI));
  unsigned old=elapsed;
  assert(!bitbang_i2c_start_wait(BITBANG_I2C_RESTART));
  assert(elapsed==old);
  bitbang_i2c_clear_error();release_after=0;
  assert(bitbang_i2c_start_wait(BITBANG_I2C_START_ONE_SHOT));
  assert(bitbang_i2c_stop_wait());
 }
 puts("PASS waiting START/STOP/ACK/data/bit abort-release, sticky fault and recovery");
}''',
'raw':r'''
int main(void){
 for(unsigned impedance=0;impedance<2;++impedance)
 for(unsigned speed=0;speed<4;++speed)
 for(unsigned bits=1;bits<=16;++bits){
  bitbang_setup(2,speed);mode_configuration.numbits=bits;
  mode_configuration.high_impedance=impedance;
  i2c_error=true;release_after=1000000;elapsed=reads=nops=0;
  delays=0;
  bitbang_write_value(0xa5,false);assert(!reads);
  if(speed<3)assert(delays==3*bits);
  delays=0;
  assert(bitbang_read_value(false)==((1u<<bits)-1));
  if(speed<3)assert(delays==2*bits);
  assert(bitbang_read_bit(false));
  bitbang_write_bit(1);bitbang_advance_clock_ticks(16);
  assert(i2c_error && bitbang_clock_stretch_timeouts()==0);
  assert(elapsed<50000);
  bitbang_setup(3,speed);
  assert(bitbang_read_with_write(0xa5)==((1u<<bits)-1));
 }
 puts("PASS all profiles, widths 1..16 and drive modes retain non-waiting raw contract");
}''',
}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--revision')
    p.add_argument('--case',choices=list(CASES))
    args=p.parse_args()
    src=source('bitbang.c',args.revision)
    header=source('bitbang.h',args.revision)
    if not args.case:
        assert 'Call bitbang_setup() before' in header
        assert 'high_impedance must be ON' in header
        assert 'opposite of bitbang_i2c_start_wait()' in header
        assert 'to the requested state' in header
        print('PASS setup/open-drain/return-convention/delay API contracts are explicit',flush=True)
    src=re.sub(r'^#include[^\n]*','',src,flags=re.M)
    for name,body in CASES.items():
        if args.case and args.case!=name: continue
        with tempfile.TemporaryDirectory(prefix='bp-bitbang-',dir=Path.home()/'AppData/Local/Temp/opencode') as tmp:
            path=Path(tmp)/'test.c';exe=Path(tmp)/'test.exe'
            path.write_text(PREFIX+header+'\n'+src+'\n'+body,encoding='utf-8')
            subprocess.run(['gcc','-std=c99','-O1','-Wall','-Werror',str(path),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
    if not args.case:
        reference=subprocess.check_output(['git','show','237d175:Firmware/bitbang.c'],cwd=ROOT,text=True,encoding='utf-8')
        def executable(text):
            text=re.sub(r'^#include[^\n]*','',text,flags=re.M)
            text=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
            return re.sub(r'\s+','',text)
        assert executable(src)==executable(reference)
        print('PASS entire accepted bitbang implementation unchanged (raw AND waiting paths)',flush=True)
        for name,function in [('raw2wire.c','raw2wire_setup_prepare'),('raw3wire.c','raw3wire_setup_prepare'),('i2c.c','i2c_setup_prepare')]:
            candidates=[ROOT/'Firmware'/name,ROOT/'Firmware/protocol_wire'/name,ROOT/'Firmware/protocol_i2c'/name]
            text=next(p for p in candidates if p.exists()).read_text(encoding='utf-8')
            start=text.index('void '+function+'(')
            end=text.index('\nvoid ',start+1)
            assert 'bitbang_setup(' in text[start:end],name
        print('PASS accepted profile-at-entry prerequisites exist before W in all three clients',flush=True)
        xml=(ROOT/'Firmware/busPirate.X/nbproject/configurations.xml').read_text(encoding='utf-8')
        directory='core_bitbang/' if (ROOT/'Firmware/core_bitbang/bitbang.c').exists() else ''
        for name in ('bitbang.c','bitbang.h'):
            assert '../'+directory+name in xml
        if directory: assert '../core_bitbang' in xml
        print('PASS MPLAB paths and exported header directory match module placement',flush=True)


if __name__=='__main__':main()
