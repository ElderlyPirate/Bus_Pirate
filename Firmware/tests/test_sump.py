"""Exercise production SUMP C with PIC-width integers and deterministic I/O.

The assembly samplers alone are replaced by bounds-checked sample sources.
This is a ring/parser/transport-state fixture, not a PIC timing/SIE model.
Generated fixtures persist under Firmware/build/sump-test for inspection.
"""
import argparse
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def function(text, name):
    from test_core_system import function as core_function
    # The shared extractor already handles the uint16_t return spelling.
    return core_function(text, name)


PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t BYTE;
#define BP_ENABLE_SUMP_SUPPORT
#define BP_TERMINAL_BUFFER_SIZE BUFFER_SIZE
#define NUM_OF_SUMP_CHANNELS 8
#define ON 1
#define OFF 0
#define BP_HIZ 0
#define HI16(v) ((uint16_t)((uint32_t)(v)>>16))
#define LO16(v) ((uint16_t)(v))
#define AUX 1024
#define AUX0 32
#define AUX1 256
#define AUX2 1
#define MOSI 2
#define CLK 4
#define MISO 8
#define CS 16
#define SUMP_SPARE6 64
#define SUMP_SPARE7 128
static uint16_t IODIR,CNPU1,CNPU2,CNPU4,CNEN1,CNEN2,CNEN4;
static uint16_t T4CON,TMR5HLD,TMR4,PR4,PR5,PORTB,PORTD;
static uint16_t BP_LEDMODE,BP_LEDMODE_LAT;
static struct { unsigned CNIF,T5IF; } IFS1bits;
static struct { unsigned CNIE; } IEC1bits;
static struct { unsigned CNIP; } IPC4bits;
static struct { unsigned USB1IE; } IEC5bits;
static struct { unsigned T32,TON; } T4CONbits;
static union { uint16_t align; uint8_t bytes[BUFFER_SIZE+8192]; } arena;
typedef struct { uint8_t *terminal_input; unsigned bus_mode; } bus_pirate_configuration_t;
bus_pirate_configuration_t bus_pirate_configuration = {arena.bytes,0};
static uint8_t output[300000], cdc[64];
static unsigned output_len, cdc_len, locked, samples_written, sample_calls;
static unsigned abort_calls, ready_calls, max_chunk, event_sample;
static uint32_t now_us;
static bool user_serial_ready_to_read(void) {
    ++ready_calls;
    return abort_calls && sample_calls>=abort_calls;
}
static uint8_t user_serial_read_byte(void) { return 0; }
static uint32_t bp_timer_microseconds(void) { return now_us; }
static void print_version_info(void) {}
static void user_serial_transmit_character(uint8_t b) {
    assert(output_len<sizeof(output)); output[output_len++]=b;
}
static void bp_write_buffer(const uint8_t *p,unsigned n) {
    while(n--) user_serial_transmit_character(*p++);
}
static void setLock(void) { locked=1; }
static void CDC_Flush_In_Now(void) {
    bp_write_buffer(cdc,cdc_len); cdc_len=0;
}
static void freeLock(void) { CDC_Flush_In_Now(); locked=0; }
static BYTE *getinbuf_cdc(uint16_t *n) { *n=64-cdc_len; return cdc+cdc_len; }
static void write_cdc(uint16_t n) {
    assert(locked && n<=64-cdc_len); cdc_len+=n;
    if(cdc_len==64) CDC_Flush_In_Now();
}
static uint8_t sample_value(uint32_t n) {
    return (uint8_t)((n*37u)^(n>>2)^(n>>8)^0x96u);
}
'''

SAMPLERS = r'''
static void checked_span(void *p,unsigned n) {
    assert((uint8_t*)p>=arena.bytes);
    if((uint8_t*)p+n>arena.bytes+BUFFER_SIZE) {
        fprintf(stderr,"sample store exceeds %u-byte buffer: offset=%u bytes=%u\n",
                BUFFER_SIZE,(unsigned)((uint8_t*)p-arena.bytes),n);
        exit(10);
    }
    ++sample_calls;
    if(n>max_chunk) max_chunk=n;
}
static void trigger_event(void) {
    if(event_sample && samples_written>=event_sample && (CNEN1|CNEN2|CNEN4))
        IFS1bits.CNIF=1;
}
static uint16_t sump_sample_bytes(uint8_t *p,uint16_t n) {
    checked_span(p,n); unsigned done=0;
    while(done<n) {
        p[done++]=sample_value(samples_written++);
        trigger_event(); if(IFS1bits.CNIF) break;
    }
    return done;
}
static uint16_t sump_sample_words(uint16_t *p,uint16_t n,uint16_t per,uint16_t shift,uint8_t mask) {
    checked_span(p,n*2u); unsigned done=0;
    while(done<n) {
        uint16_t v=0;
        for(unsigned j=0;j<per;++j) v=(uint16_t)((v<<shift)|(sample_value(samples_written++)&mask));
        p[done++]=v; trigger_event(); if(IFS1bits.CNIF) break;
    }
    return done;
}
static void fast_words(uint16_t *p,uint16_t n) {
    checked_span(p,n*2u);
    while(n--) {
        unsigned v=sample_value(samples_written++);
#ifdef BUSPIRATEV4
        *p++=(v&63u)|((v&128u)<<1);
#else
        *p++=(v&31u)<<6;
#endif
    }
}
static void sump_fast_capture_16mhz(uint16_t *p,uint16_t n) { fast_words(p,n); }
static void sump_fast_capture_8mhz(uint16_t *p,uint16_t n) { fast_words(p,n); }
#ifdef BUSPIRATEV4
static void sump_fast_capture_4mhz(uint16_t *p,uint16_t n) { fast_words(p,n); }
static void sump_fast_capture_2mhz(uint16_t *p,uint16_t n) { fast_words(p,n); }
#else
static void fast_bytes(uint8_t *p,uint16_t n) { checked_span(p,n);while(n--) *p++=sample_value(samples_written++)&31u; }
static void sump_fast_capture_4mhz(uint8_t *p,uint16_t n) { fast_bytes(p,n); }
static void sump_fast_capture_2mhz(uint8_t *p,uint16_t n) { fast_bytes(p,n); }
#endif
'''

TEST = r'''
static void clean(void) {
    memset(&arena,0xa5,sizeof(arena)); output_len=cdc_len=locked=0;
    samples_written=sample_calls=ready_calls=max_chunk=abort_calls=event_sample=0;
    CNEN1=CNEN2=CNEN4=0; IFS1bits.CNIF=0; command_processor_state=RX_COMMAND_IDLE;
    now_us+=2000000u; sump_reset();
}
static void command(uint8_t op,uint32_t value) {
    sump_handle_command_byte(op);
    for(unsigned j=0;j<4;++j) sump_handle_command_byte((uint8_t)(value>>(j*8)));
}
static void counts(uint32_t n) { command(0x81,((n/4-1)<<16)|(n/4-1)); }
static void require(bool good,const char *why) { if(!good) { fprintf(stderr,"FAIL %s\n",why);exit(11); } }
static void readout_matrix(void) {
    unsigned checks=0;
    const unsigned per_list[]={1,4,8,16};
    for(unsigned f=0;f<4;++f) {
        unsigned per=per_list[f],cap=BUFFER_SIZE/(per==1?1:2);
        unsigned shift=(per==1?8:16/per),mask=(per==1?BP_SUMP_SAMPLE_MASK:(1u<<shift)-1u);
        for(unsigned turns=0;turns<3;++turns)
        for(unsigned offset=0;offset<per+3;++offset) {
            clean();
            uint32_t written=(turns+1)*cap*per+offset;
            uint32_t word_count=(written+per-1)/per;
            uint32_t oldest=(word_count>cap?word_count-cap:0)*per;
            uint32_t end=written-offset%per;
            if(end<8) continue;
            unsigned samples=(end<cap*per?end:cap*per)-4;
            uint32_t requested=samples+68;
            if(per==1) {
                oldest=written>cap?written-cap:0;
                for(uint32_t a=oldest;a<written;++a) arena.bytes[a%cap]=sample_value(a)&mask;
                sump_transmit_bytes(samples,requested,end,written,mask);
            } else {
                uint16_t *words=(uint16_t*)arena.bytes;
                for(uint32_t w=word_count>cap?word_count-cap:0;w<word_count;++w) {
                    uint16_t v=0;
                    for(unsigned j=0;j<per;++j) v=(uint16_t)((v<<shift)|(sample_value(w*per+j)&mask));
                    words[w%cap]=v;
                }
                sump_transmit_packed(samples,requested,end,written,per,shift,mask);
            }
            require(output_len==requested,"readout length");
            for(uint32_t i=0;i<requested;++i) {
                uint8_t pat=i%2?0xaa:0x55, expected=pat;
                if(i>=requested-samples) {
                    uint32_t a=end-1-(i-(requested-samples));
                    expected=(pat&~mask)|(a<oldest?0:sample_value(a)&mask);
                }
                require(output[i]==expected,"newest-first readout, ring seam, padding or overwritten sample");
            }
            ++checks;
        }
    }
    printf("PASS %u reference readout windows (byte/4/8/16, seams, overshoot, overwrite, every byte)\n",checks);
}
static void capacity_matrix(void) {
    unsigned checks=0;
    const unsigned sizes[]={4,1024,2048,2052,4096,8192,16384,32768};
    const unsigned divs[]={99,49,24,11,5};
    for(unsigned i=0;i<sizeof(sizes)/sizeof(*sizes);++i)
    for(unsigned j=0;j<sizeof(divs)/sizeof(*divs);++j) {
        clean(); counts(sizes[i]); command(0x80,divs[j]);
        sump_handle_command_byte(1); require(sump_acquire_samples(),"capture runs");
        require(output_len==sizes[i],"capacity-clamped capture preserves host length");
        for(unsigned g=BUFFER_SIZE;g<sizeof(arena.bytes);++g) require(arena.bytes[g]==0xa5,"sample guard");
        ++checks;
    }
    printf("PASS %u complete capture/capacity cases\n",checks);
}
static void count_matrix(void) {
    const uint32_t n[]={32768,65532,65536,65540,131072,262144};
    for(unsigned i=0;i<sizeof(n)/sizeof(*n);++i) {
        clean(); counts(n[i]);
        require(samples_requested==n[i],"16-bit CNT truncation");
        command(0x80,99); sump_handle_command_byte(1); sump_acquire_samples();
        require(output_len==n[i],"wide CNT reply exact");
    }
    puts("PASS six CNT boundaries through 262144 with exact capture/readout length");
}
static void parser_matrix(void) {
    for(unsigned op=0;op<128;++op) {
        if(op==0||op==1||op==2||op==4||op==15||op==17||op==19||op=='#'||op=='$') continue;
        clean(); sump_handle_command_byte(op); sump_handle_command_byte(2);
        require(output_len==4&&!memcmp(output,"1ALS",4),"unknown short command must not consume following ID");
    }
    for(unsigned got=0;got<4;++got) {
        clean(); sump_handle_command_byte(0x81);
        for(unsigned i=0;i<got;++i) sump_handle_command_byte(0);
        now_us+=1000001; sump_handle_command_byte(2);
        require(output_len==4,"one-second parser heal");
    }
    clean(); command(0xc1,0x24230f02); require(!output_len,"exit-valued operands stay data");
    require(sump_handle_command_byte('$'),"dollar requests SUMP exit");
    puts("PASS unknown short commands, timeout heal and exit-valued long operands");
}
static void trigger_matrix(void) {
    clean(); command(0xc0,4); command(0xc0,128);
    require(!(CNEN1|CNEN2|CNEN4),"CN inputs must remain disabled until wait");
    sump_apply_trigger();
#ifdef BUSPIRATEV4
    require(!(CNEN4&8)&&(CNEN4&32),"trigger mask replacement");
#else
    require(!(CNEN2&64),"unsupported mask must not retain CLK enable");
#endif
    sump_disable_trigger(); require(!(CNEN1|CNEN2|CNEN4),"disable all SUMP CN");
    puts("PASS trigger configuration gating and exact mask replacement");
}
static void timer_matrix(void) {
    const unsigned d[]={409599,409600,819199,16777215};
    for(unsigned i=0;i<4;++i) {
        clean(); command(0x80,d[i]);
        require(sump_timer_ticks==(uint32_t)d[i]*4/25+1,"timer period truncated at 65536");
        counts(4096); abort_calls=1;
        unsigned span=sump_samples_per_word==1?16:32/sump_samples_per_word;
        sump_handle_command_byte(1); sump_acquire_samples();
        require(max_chunk==span && output_len==0,"slow-divider fragment/host-abort budget");
    }
    puts("PASS full-width timer period and bounded abort fragments across 65536-tick seams");
}
int main(int argc,char **argv) {
    require(argc==2,"case required");
    if(!strcmp(argv[1],"capacity")) capacity_matrix();
    else if(!strcmp(argv[1],"readout")) readout_matrix();
    else if(!strcmp(argv[1],"count")) count_matrix();
    else if(!strcmp(argv[1],"parser")) parser_matrix();
    else if(!strcmp(argv[1],"trigger")) trigger_matrix();
    else if(!strcmp(argv[1],"timer")) timer_matrix();
    else return 2;
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--revision')
    parser.add_argument('--case', choices=['capacity', 'readout', 'count', 'parser', 'trigger', 'timer'])
    args = parser.parse_args()
    path = 'Firmware/protocol_sump/sump.c'
    source = (subprocess.check_output(['git', 'show', args.revision + ':' + path], cwd=ROOT).decode()
              if args.revision else (ROOT / path).read_text())
    source = source.replace('unsigned int', 'uint16_t')
    prefix = source[:source.index('static uint16_t sump_sample_bytes(')]
    prefix = re.sub(r'^#include[^\n]*\n', '', prefix, flags=re.M)
    funcs = ['sump_wait_for_fast_trigger', 'sump_transmit_fast', 'sump_transmit_bytes',
             'sump_transmit_packed', 'sump_acquire_samples']
    body = prefix + SAMPLERS + '\n'.join(function(source, name) for name in funcs)
    failures = 0
    for board in ('V3', 'V4'):
        for size in (2048, 4096):
            out = ROOT / 'Firmware/build/sump-test' / (args.revision or 'working') / (board + '-' + str(size))
            out.mkdir(parents=True, exist_ok=True)
            c, exe = out / 'sump.c', out / 'sump.exe'
            c.write_text(PREFIX + body + '\n#endif\n' + TEST, encoding='ascii')
            subprocess.run(['gcc', '-std=gnu99', '-O1', '-Wall', '-Wextra', '-Wno-unused-function',
                            '-Wno-unused-variable', '-Wno-implicit-fallthrough',
                            '-DBUSPIRATE' + board, '-DBPv4_SUMP_SOFT_WIRE', '-DBUFFER_SIZE=' + str(size),
                            str(c), '-o', str(exe)], check=True)
            for case in ([args.case] if args.case else ['capacity', 'readout', 'count', 'parser', 'trigger', 'timer']):
                print(board, size, case, flush=True)
                result = subprocess.run([str(exe), case], timeout=20)
                failures += result.returncode != 0
    raise SystemExit(1 if failures else 0)


if __name__ == '__main__':
    main()
