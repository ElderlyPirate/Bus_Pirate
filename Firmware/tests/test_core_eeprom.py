"""Production EEPROM driver with deterministic register/device fixtures, not a PIC simulator."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]
TEMP=Path.home()/'AppData/Local/Temp/opencode'


def source(name,revision=None):
    for directory in ('core_eeprom/',''):
        path='Firmware/'+directory+name
        if revision:
            r=subprocess.run(['git','show',revision+':'+path],cwd=ROOT,capture_output=True,text=True,encoding='utf-8')
            if not r.returncode:return r.stdout
        elif (ROOT/path).exists():return (ROOT/path).read_text(encoding='utf-8')
    raise RuntimeError(name)


def function(text,name):
    mask=re.sub(r'/\*.*?\*/|//[^\n]*',lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),text,flags=re.S)
    m=re.search(r'(?m)^[\w *]+\b'+name+r'\([^;]*?\)\s*\{',mask);assert m,name
    end,depth=m.end(),1
    while depth:depth+=(mask[end]=='{')-(mask[end]=='}');end+=1
    return text[m.start():end]


def compile_run(body,name):
    with tempfile.TemporaryDirectory(prefix='bp-eeprom-',dir=TEMP) as tmp:
        p=Path(tmp)/(name+'.c');exe=p.with_suffix('.exe')
        p.write_text(body,encoding='ascii')
        subprocess.run(['gcc','-std=c99','-O1','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)


PREFIX=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define BUSPIRATEV4
#define HIGH 1
#define LOW 0
#define INPUT 1
#define OUTPUT 0
#define ON 1
#define OFF 0
#define LO8(v) ((uint8_t)(v))
#define Nop() ((void)0)
#define _I2C1CON_SEN_MASK 1u
#define _I2C1CON_RSEN_MASK 2u
#define _I2C1CON_PEN_MASK 4u
#define _I2C1CON_RCEN_MASK 8u
#define _I2C1CON_ACKEN_MASK 16u
#define _I2C1CON_ACKDT_MASK 32u
#define _I2C1CON_SMEN_MASK 256u
#define _I2C1CON_DISSLW_MASK 512u
#define _I2C1CON_A10M_MASK 1024u
#define _I2C1CON_SCLREL_MASK 4096u
#define _I2C1CON_I2CEN_MASK 32768u
#define _I2C1STAT_TBF_MASK 1u
#define _I2C1STAT_RBF_MASK 2u
#define _I2C1STAT_I2COV_MASK 64u
#define _I2C1STAT_IWCOL_MASK 128u
#define _I2C1STAT_BCL_MASK 1024u
#define _I2C1STAT_TRSTAT_MASK 16384u
#define _I2C1STAT_ACKSTAT_MASK 32768u
typedef union {
 uint16_t word;
 struct {unsigned short LATD0:1,LATD1:1,LATD2:1,LATD3:1,LATD4:1,LATD5:1,LATD6:1,LATD7:1,LATD8:1,LATD9:1,LATD10:1,LATD11:1,LATD12:1,LATD13:1,LATD14:1,LATD15:1;} bits;
} latch_t;
typedef union {
 uint16_t word;
 struct {unsigned short TRISD0:1,TRISD1:1,TRISD2:1,TRISD3:1,TRISD4:1,TRISD5:1,TRISD6:1,TRISD7:1,TRISD8:1,TRISD9:1,TRISD10:1,TRISD11:1,TRISD12:1,TRISD13:1,TRISD14:1,TRISD15:1;} bits;
} direction_t;
static latch_t latch;
static direction_t direction;
#define LATDbits latch.bits
#define BP_EE_SDA_DIR direction.bits.TRISD9
#define BP_EE_SCL_DIR direction.bits.TRISD10
#define BP_EE_WP_DIR direction.bits.TRISD11
enum {F_NONE,F_START,F_RESTART,F_STOP,F_TX,F_RX,F_RBF,F_ACK,F_BCL,F_CLOCK,F_DATA,F_NACK};
static unsigned fault,accesses,phase,tx_count,nack_at,starts,restarts,stops,nacks,programs,payloads,pulses;
static uint16_t con,stat,trn,rcv,brg,add,msk,pointer,address_hi,write_address;
static uint8_t memory[8192],before[8192],staged;
static bool active,tx_pending,has_data,reading,pending_program,freeze_clock,stranded,last_gpio_clock;
static bool fail_pattern_read,fail_restore,force_wp_high;
static uint64_t ticks,busy_until,write_ticks=1000;
static unsigned ms_delays;
static void step(void);
static unsigned protected_now(void){return latch.bits.LATD11||force_wp_high;}
static uint16_t fixture_pins(void){
 unsigned clock=direction.bits.TRISD10?fault!=F_CLOCK:latch.bits.LATD10;
 if(!last_gpio_clock&&clock&&stranded){assert(protected_now());if(++pulses>=9)stranded=false;}
 last_gpio_clock=clock;
 unsigned data=direction.bits.TRISD9?(fault!=F_DATA&&!stranded):latch.bits.LATD9;
 return (clock?1024:0)|(data?512:0)|(protected_now()?2048:0);
}
#define IOPOR fixture_pins()
#define BP_EE_SCL ((fixture_pins()&1024)!=0)
#define BP_EE_SDA ((fixture_pins()&512)!=0)
#define BP_EE_WP protected_now()
static uint16_t *fixture_control(void){step();return &con;}
static uint16_t *fixture_status(void){step();return &stat;}
static uint16_t *transmit(void){step();tx_pending=true;return &trn;}
static uint16_t *receive(void){step();stat&=~2u;return &rcv;}
#define I2C1CON (*fixture_control())
#define I2C1STAT (*fixture_status())
#define I2C1TRN (*transmit())
#define I2C1RCV (*receive())
#define I2C1BRG brg
#define I2C1ADD add
#define I2C1MSK msk
static void step(void){
 assert(++accesses<2000000u);
 if(pending_program&&ticks>=busy_until){memory[write_address]=staged;pending_program=false;}
 if(!(con&32768)){active=tx_pending=has_data=false;return;}
 if(fault==F_BCL){stat|=1024;return;}
 if(con&1){
  if(fault==F_START||fault==F_CLOCK||fault==F_DATA||stranded)return;
  assert(!active);active=true;reading=false;phase=0;has_data=false;++starts;con&=~1u;ticks+=4;
 }
 if(con&2){
  if(fault==F_RESTART)return;
  assert(active);reading=false;phase=0;has_data=false;++restarts;con&=~2u;ticks+=4;
 }
 if(tx_pending){
  if(fault==F_TX){stat|=16385;return;}
  assert(active);++tx_count;bool ack=true;
  if(fault==F_NACK||tx_count==nack_at){ack=false;phase=99;}
  else if(phase==0){
   if(ticks<busy_until||(trn!=0xa0&&trn!=0xa1)){ack=false;phase=99;}
   else{reading=(trn&1)!=0;phase=reading?4:1;}
  }else if(phase==1){address_hi=trn;phase=2;}
  else if(phase==2){pointer=((address_hi<<8)|trn)&8191;phase=3;}
  else if(phase==3){
   assert(!reading&&!has_data);
   if(fail_restore&&programs==1){ack=false;phase=99;}
   else{staged=trn;write_address=pointer;has_data=true;++payloads;}
  }
  else ack=false;
  stat=(stat&~32768u)|(ack?0:32768u);stat&=~16385u;tx_pending=false;ticks+=20;
 }
 if(con&8){
  if(fail_pattern_read&&programs==1){fail_pattern_read=false;stat|=1024;return;}
  if(fault==F_RX)return;
  assert(active&&reading);rcv=memory[pointer];pointer=(pointer+1)&8191;
  con&=~8u;if(fault!=F_RBF)stat|=2u;ticks+=20;
 }
 if(con&16){
  if(fault==F_ACK)return;
  assert(con&32);++nacks;con&=~16u;ticks+=4;
 }
 if(con&4){
  if(fault==F_STOP)return;
  if(has_data&&!protected_now()){
   ++programs;pending_program=true;busy_until=ticks+write_ticks;
  }
  has_data=false;active=false;con&=~4u;++stops;ticks+=4;
 }
}
static void delay_short_inline(uint16_t us){ticks+=(uint32_t)us*2;step();(void)fixture_pins();}
static uint32_t bp_timer_microseconds(void){return freeze_clock?0:(uint32_t)((ticks>>1)&0x7fffffffUL);}
static void reset_fixture(void){
 fault=accesses=phase=tx_count=nack_at=starts=restarts=stops=nacks=programs=payloads=pulses=ms_delays=0;
 con=stat=trn=rcv=brg=add=msk=pointer=address_hi=write_address=0;
 active=tx_pending=has_data=reading=pending_program=freeze_clock=stranded=false;
 fail_pattern_read=fail_restore=force_wp_high=false;
 ticks=busy_until=0;write_ticks=1000;last_gpio_clock=true;
 latch.word=0x0955;direction.word=0xf7ff;
 memset(before,0,sizeof(before));
 for(unsigned i=0;i<8192;++i)memory[i]=(uint8_t)(i*73+19);
}
'''

CASES={
'preserve':r'''
int main(void){
 for(unsigned wp=0;wp<2;++wp)for(unsigned original=0;original<256;++original){
  reset_fixture();memory[0]=original;memcpy(before,memory,8192);latch.bits.LATD11=wp;
  assert(eeprom_test());assert(!memcmp(before,memory,8192));
  assert(latch.bits.LATD11==wp && !(con&32768) && programs==2 && ms_delays==0);
  assert(direction.bits.TRISD9&&direction.bits.TRISD10);
 }
 puts("PASS 512 selftest values/WP states preserve all 8192 bytes, use two completed writes and release I2C1");
}''',
'faults':r'''
int main(void){
 for(unsigned f=F_START;f<=F_NACK;++f){
  reset_fixture();fault=f;memcpy(before,memory,8192);
  assert(!eeprom_test());assert(!memcmp(before,memory,8192));
  assert(programs==0 && !(con&32768) && latch.bits.LATD11);
  assert(direction.bits.TRISD9&&direction.bits.TRISD10);
 }
 for(unsigned at=2;at<=9;++at){
  reset_fixture();nack_at=at;memcpy(before,memory,8192);
  assert(!eeprom_test());assert(!memcmp(before,memory,8192));assert(!(con&32768));
 }
 reset_fixture();fail_pattern_read=true;latch.bits.LATD11=0;memcpy(before,memory,8192);
 assert(!eeprom_test()&&!memcmp(before,memory,8192)&&!latch.bits.LATD11&&!(con&32768));
 reset_fixture();fail_restore=true;memcpy(before,memory,8192);
 assert(!eeprom_test()&&memory[0]!=before[0]&&latch.bits.LATD11&&!(con&32768));
 reset_fixture();force_wp_high=true;memory[0]=0x10;memcpy(before,memory,8192);
 assert(!eeprom_test()&&!memcmp(before,memory,8192)&&!programs);
 puts("PASS bounded START/RSEN/STOP/TX/RX/RBF/ACK/collision/held-line/missing-device faults and checked address/data NACKs");
}''',
'bounds':r'''
int main(void){
 reset_fixture();memcpy(before,memory,8192);assert(eeprom_i2c_setup());assert(eeprom_ready());
 latch.bits.LATD11=0;
 const unsigned addresses[]={0,31,32,8191};
 for(unsigned i=0;i<4;++i){unsigned a=addresses[i];uint8_t value;
  assert(eeprom_write_byte(0xa5,a));assert(eeprom_read_byte(a,&value)&&value==0xa5);
  assert(eeprom_write_byte(before[a],a));
 }
 assert(!memcmp(before,memory,8192));unsigned old=tx_count;uint8_t value=0x67;
 assert(!eeprom_read_byte(8192,&value)&&value==0x67);
 assert(!eeprom_read_byte(65535,&value)&&value==0x67);
 assert(!eeprom_read_byte(0,NULL));
 assert(!eeprom_write_byte(0x11,8192)&&!eeprom_write_byte(0x22,65535));
 assert(old==tx_count);
 for(unsigned address=0;address<8192;++address){
  uint8_t got;assert(eeprom_read_byte(address,&got)&&got==before[address]);
 }
 puts("PASS first/page-edge/last byte writes with STOP+completion polling; invalid addresses/NULL rejected before bus activity");
}''',
'ready':r'''
int main(void){
 for(unsigned wrap=0;wrap<2;++wrap){
  reset_fixture();ticks=wrap?0xffffff00ULL:0;assert(eeprom_i2c_setup());
  uint64_t started=ticks;busy_until=ticks+4000;
  assert(eeprom_ready()&&ticks-started>=4000&&programs==0);
 }
 reset_fixture();assert(eeprom_i2c_setup());fault=F_NACK;freeze_clock=true;
 assert(!eeprom_ready() && tx_count<=EEPROM_READY_POLLS && !(con&32768));
 puts("PASS address-only write-cycle polling, 31-bit timestamp wrap and finite budget with frozen test clock");
}''',
'bus_clear':r'''
int main(void){
 reset_fixture();stranded=true;latch.bits.LATD11=0;memcpy(before,memory,8192);
 assert(eeprom_test()&&pulses>=9&&!memcmp(before,memory,8192)&&!latch.bits.LATD11);
 puts("PASS protected nine-clock internal bus clear/STOP recovers a stranded device without sibling GPIO writes");
}''',
'init':r'''
int main(void){
 reset_fixture();latch.word=0x1755;uint16_t old=latch.word;con=32768;
 assert(!eeprom_test() && con==32768 && latch.word==old && !programs);
 eeprom_initialize();assert(!con && latch.word==((old&~0xe00u)|0x800u));
 assert(direction.bits.TRISD9&&direction.bits.TRISD10&&!direction.bits.TRISD11);
 assert(eeprom_i2c_setup());assert(con==(_I2C1CON_SCLREL_MASK|_I2C1CON_I2CEN_MASK));
 assert(brg==37&&!add&&!msk&&!stat);
 puts("PASS LAT-only initialization, protection before ownership release, unowned-I2C1 precondition and deterministic 400kHz config");
}''',
}


def before_proof():
    old=source('onboard_eeprom.c','aadf2d1')
    assert len(re.findall(r'while\s*\(',old))==5
    assert 'eeprom_start();' in function(old,'eeprom_read_byte')
    prefix=r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
static uint8_t memory[8192];static bool wp=true;
#define BP_EE_WP wp
#define LOW 0
#define I2C_WRITE_DELAY_TIME 10
static void eeprom_i2c_setup(void){}
static void bp_delay_ms(unsigned n){(void)n;}
static bool eeprom_write_byte(uint8_t v,uint16_t a){memory[a]=v;return false;}
static uint8_t eeprom_read_byte(uint16_t a){return memory[a];}
'''
    test=r'''
int main(void){memory[0]=0x3c;assert(eeprom_test());assert(memory[0]==0xff&&wp);puts("BEFORE CONFIRMED: real selftest body returns success but replaces saved 0x3c with 0xff; five unbounded primitive loops and SEN-only read sequence remain");}
'''
    compile_run(prefix+function(old,'eeprom_test')+test,'before')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--case',choices=list(CASES))
    p.add_argument('--before',action='store_true')
    args=p.parse_args()
    if args.before:before_proof();return
    strip=lambda s:re.sub(r'^#include[^\n]*','',s,flags=re.M)
    body=PREFIX+'\n'+strip(source('onboard_eeprom.h'))+'\n'+strip(source('onboard_eeprom.c'))
    for name,test in CASES.items():
        if args.case and args.case!=name:continue
        compile_run(body+'\n'+test,name)
    if not args.case:
        v3='#define BUSPIRATEV3\n'+strip(source('onboard_eeprom.h'))+'\n'+strip(source('onboard_eeprom.c'))+'\nint main(void){return 0;}\n'
        compile_run(v3,'v3_guard')
        print('PASS v3 excludes the v4-only driver and its public storage definitions',flush=True)
        paths=[ROOT/'Firmware/i2c.c',ROOT/'Firmware/protocol_i2c/i2c.c']
        caller=next(p for p in paths if p.exists()).read_text(encoding='utf-8')
        assert 'i2c_state.to_eeprom = false;' in caller
        assert 'Onboard EEPROM requires hardware I2C' in caller
        assert 'I2C1CONbits.I2CEN = OFF;' in function(caller,'i2c_cleanup')
        print('PASS accepted route reset, software rejection and I2C1 cleanup remain present',flush=True)
        project=ROOT/'Firmware/busPirate.X/nbproject/configurations.xml'
        xml=ET.parse(project)
        directory='core_eeprom/' if (ROOT/'Firmware/core_eeprom/onboard_eeprom.c').exists() else ''
        listed={n.text for n in xml.findall('.//itemPath')}
        excluded={n.attrib['path'] for n in xml.findall('.//item') if n.attrib.get('ex')=='true'}
        for name in ('onboard_eeprom.c','onboard_eeprom.h'):
            assert '../'+directory+name in listed and '../'+directory+name in excluded
        if directory:assert project.read_text(encoding='utf-8').count('../core_eeprom')>=6
        print('PASS EEPROM MPLAB paths, include export and v3 exclusions follow module placement',flush=True)


if __name__=='__main__':main()
