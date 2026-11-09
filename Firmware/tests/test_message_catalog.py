"""Packed-message/generated-catalog contracts; actual production C, not a PIC simulator."""
import argparse
import ast
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]
TEMP=Path.home()/'AppData/Local/Temp/opencode'


def source(name,revision=None):
    for directory in ('core_messages/','core_system/','core_usb/','protocol_uart/',''):
        path='Firmware/'+directory+name
        if revision:
            r=subprocess.run(['git','show',revision+':'+path],cwd=ROOT,capture_output=True,text=True,encoding='utf-8')
            if not r.returncode:return r.stdout
        elif (ROOT/path).exists():return (ROOT/path).read_text(encoding='utf-8')
    raise RuntimeError(name)


def function(text,name):
    # Remove comments without changing string literals or brace positions.
    mask=re.sub(r'/\*.*?\*/|//[^\n]*',lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),text,flags=re.S)
    m=re.search(r'(?m)^[\w *]+\b'+name+r'\([^;]*?\)\s*\{',mask)
    assert m,name
    end,depth=m.end(),1
    while depth:
        depth+=(mask[end]=='{')-(mask[end]=='}');end+=1
    return text[m.start():end]


def assembly(text):
    entries={}
    for name,value in re.findall(r'(?m)^_(\w+)_str:\s*\n\s*\.pasciz\s+([^\n]+)',text):
        assert name not in entries,name
        result=bytearray()
        terminated=False
        for token in re.findall(r'"(?:\\.|[^"\\])*"|<\d+>',value):
            assert not terminated,(name,'data follows a NUL-terminated .pasciz string operand')
            if token.startswith('<'):result.append(int(token[1:-1]))
            else:
                result.extend(ast.literal_eval(token).encode('latin1'))
                terminated=True
        assert terminated,(name,'missing quoted NUL-terminated operand')
        entries[name]=bytes(result)
    assert len(entries)==len(re.findall(r'(?m)^_\w+_str:',text))
    return entries


def header(text):
    entries={}
    for name,call,label in re.findall(r'(?m)^#define (\w+) (bp_message_write_(?:buffer|line))\(__builtin_tbladdress\((\w+)_str\)\)',text):
        assert name==label and name not in entries,name
        entries[name]=(call=='bp_message_write_line')
    assert len(entries)==len(re.findall(r'(?m)^void \w+_str\(void\);',text))
    return entries


def compile_run(prefix,body,test,name):
    with tempfile.TemporaryDirectory(prefix='bp-messages-',dir=TEMP) as tmp:
        p=Path(tmp)/(name+'.c');exe=p.with_suffix('.exe')
        p.write_text(prefix+'\n'+body+'\n'+test,encoding='ascii')
        subprocess.run(['gcc','-std=c99','-O1','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)


PREFIX=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
static uint8_t TBLPAG;
static const uint8_t *data;
static size_t length,reads,used;
static unsigned long base;
static uint8_t output[4096];
static const char *label;
static unsigned long lookup(const char *s){if(strcmp(label,s)){fprintf(stderr,"expected %s got %s\n",label,s);assert(0);}return base;}
#define __builtin_tbladdress(s) lookup(#s)
#define LO8(v) ((uint8_t)(v))
#define HI8(v) ((uint8_t)((v)>>8))
static uint8_t byte_at(unsigned long addr,unsigned offset){
 unsigned long absolute=((unsigned long)TBLPAG<<16)|(addr&65535);
 assert(!(absolute&1) && absolute>=base);
 size_t word=(absolute-base)/2;
 assert(word<(length+3)/3);++reads;
 size_t index=word*3+offset;
 return index<length?data[index]:(index==length?0:0xa5);
}
static uint16_t low(unsigned long addr){
 uint16_t lo=byte_at(addr,0),hi=byte_at(addr,1);return lo|(hi<<8);
}
static uint16_t upper(unsigned long addr){return byte_at(addr,2);}
#define __builtin_tblrdl(s) low(s)
#define __builtin_tblrdh(s) upper(s)
static void user_serial_transmit_character(uint8_t c){assert(used<sizeof(output));output[used++]=c;}
static uint8_t answer;
static uint8_t user_serial_read_byte(void){return answer;}
#define bpBR do{user_serial_transmit_character(13);user_serial_transmit_character(10);}while(0)
'''


def packed(src,hdr,entries,flags,board):
    vectors=[];cases=[]
    for i,(name,value) in enumerate(entries.items()):
        values=','.join(str(v) for v in value) or '0'
        vectors.append('static const uint8_t data%d[]={%s};'%(i,values))
        cases.append('case %d:data=data%d;length=%d;label="%s_str";break;'%(i,i,len(value),name))
    invoke='static void invoke(unsigned n){switch(n){'+''.join('case %d:%s;break;'%(i,name) for i,name in enumerate(entries))+'}}'
    sizes='static const unsigned line_flags[]={'+','.join(str(int(flags[n])) for n in entries)+'};'
    test='\n'.join(vectors)+'\n'+sizes+'\n'+invoke+r'''
int main(void){
 for(unsigned i=0;i<@@COUNT@@;++i)for(unsigned page=0;page<3;++page){
  switch(i){@@CASES@@}
  base=page==0?0x200:page==1?0x1fffe:0x2fffc;
  used=reads=0;TBLPAG=0x73;invoke(i);
  assert(TBLPAG==0x73 && used==length+(line_flags[i]?2:0));
  assert(!memcmp(output,data,length));
  if(line_flags[i])assert(output[length]==13 && output[length+1]==10);
 }
 uint8_t all[512];for(unsigned n=0;n<sizeof(all);++n)all[n]=(n%255)+1;
 for(length=0;length<=sizeof(all);++length){
  data=all;base=0x1fffe;used=reads=0;TBLPAG=0x42;
  bp_message_write_buffer(base);
  assert(used==length && !memcmp(output,data,length) && TBLPAG==0x42);
 }
 puts("PASS @@BOARD@@ packed bytes/macros: @@WRITER_CASES@@ cases, all three NUL lanes, page crossing, no next-section read, TBLPAG restore");
}'''.replace('@@COUNT@@',str(len(entries))).replace('@@CASES@@',''.join(cases)).replace('@@BOARD@@',board).replace('@@WRITER_CASES@@',str(len(entries)*3+513))
    prefix=PREFIX.replace('static uint8_t user_serial_read_byte(void){return answer;}','')
    prefix=prefix.replace('static uint8_t answer;','')
    body=hdr+'\n'+function(src,'bp_message_write_buffer')+'\n'+function(src,'bp_message_write_line')
    compile_run(prefix,body,test,board)


def cdc_zero(revision):
    prefix=r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t BYTE;
#define BUSPIRATEV4
#define CDC_BUFFER_SIZE 64
static BYTE buffers[2][64],*InPtr;
static BYTE cdc_In_len,cdc_timeout_count,ZLPpending,lock;
static unsigned irq=1,active,used,calls,packets;
static BYTE output[10000];
static unsigned irq_state(void){assert(++calls<100000);return irq;}
#define TestGlobalUsbInterruptEnable() irq_state()
#define DisableGlobalUsbInterrupt() (irq=0)
#define EnableUsbGlobalInterrupt() (irq=1)
static void putda_cdc(BYTE n){
 assert(irq && lock);memcpy(output+used,buffers[active],n);used+=n;
 ++packets;active^=1;InPtr=buffers[active];
}
'''
    cdc=source('cdc.c',revision)
    names=('CDC_Flush_In_Now','setLock','freeLock','getinbuf_cdc','write_cdc')
    body='\n'.join(function(cdc,n) for n in names)+'\n'+function(source('base.c',revision),'bp_write_buffer')
    test=r'''
int main(void){
 static BYTE payload[4096];for(unsigned i=0;i<4096;++i)payload[i]=i;
 const unsigned sizes[]={0,1,63,64,65,128,4096};
 for(unsigned fill=0;fill<=64;++fill)for(unsigned j=0;j<sizeof(sizes)/sizeof(sizes[0]);++j){
  memset(buffers,0xee,sizeof(buffers));active=used=calls=packets=0;irq=1;
  cdc_In_len=fill;InPtr=buffers[0]+fill;lock=0;
  bp_write_buffer(payload,sizes[j]);
  assert(!lock && irq && cdc_In_len==0 && used==fill+sizes[j]);
  for(unsigned i=0;i<fill;++i)assert(output[i]==0xee);
  assert(!memcmp(output+fill,payload,sizes[j]));
 }
 puts("PASS core bulk writer: all fills 0..64, zero-space write_cdc(0) publishes full packet and progresses; no spin or payload loss");
}'''
    compile_run(prefix,body,test,'cdc_zero')


def nested(src):
    prefix=PREFIX
    prefix=prefix.replace('static const char *label;','')
    start=prefix.index('static unsigned long lookup(')
    end=prefix.index('\n#define LO8',start)
    prefix=prefix[:start]+prefix[end:]
    prefix=prefix.replace(function(prefix,'byte_at'),r'''
static const uint8_t inner[]={0x80,'I',0xff};
static uint8_t byte_at(unsigned long addr,unsigned offset){
 unsigned long absolute=((unsigned long)TBLPAG<<16)|(addr&65535);
 const uint8_t *p=data;size_t n=length;unsigned long origin=base;
 if(absolute>=0x30000){p=inner;n=sizeof(inner);origin=0x30000;}
 assert(!(absolute&1) && absolute>=origin);
 size_t word=(absolute-origin)/2;assert(word<(n+3)/3);++reads;
 size_t index=word*3+offset;return index<n?p[index]:(index==n?0:0xa5);
}''')
    prefix=prefix.replace('static void user_serial_transmit_character(uint8_t c){assert(used<sizeof(output));output[used++]=c;}',r'''
void bp_message_write_line(unsigned long strptr);
static bool inserted;
static void user_serial_transmit_character(uint8_t c){
 assert(used<sizeof(output));output[used++]=c;
 if(!inserted){inserted=true;uint8_t saved=TBLPAG;bp_message_write_line(0x30000);assert(TBLPAG==saved);}
}''')
    prefix=prefix.replace('static uint8_t answer;','').replace('static uint8_t user_serial_read_byte(void){return answer;}','')
    body=function(src,'bp_message_write_buffer')+'\n'+function(src,'bp_message_write_line')
    test=r'''
int main(void){
 const uint8_t outer[]="outer page crossing";
 data=outer;length=sizeof(outer)-1;base=0x1fffe;TBLPAG=0x72;
 bp_message_write_buffer(base);assert(TBLPAG==0x72 && used==length+5);
 assert(output[0]=='o'&&!memcmp(output+1,inner,3)&&output[4]==13&&output[5]==10);
 assert(!memcmp(output+6,outer+1,length-1));
 puts("PASS nested message writer restores caller TBLPAG and preserves upper-byte payloads across pages");
}'''
    compile_run(prefix,body,test,'nested')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--revision')
    p.add_argument('--case',choices=('inventory','regen','colors','crlf','packed','agree','cdc_zero','uart_fields','generator','help','nested'))
    p.add_argument('--dump',type=Path)
    args=p.parse_args()
    src=source('messages.c',args.revision)
    tables={b:assembly(source('messages_'+b+'.s',args.revision)) for b in ('v3','v4')}
    flags={b:header(source('messages_'+b+'.h',args.revision)) for b in tables}
    def wanted(name):return args.case in (None,name)
    if args.dump:
        args.dump.write_text(json.dumps({b:{n:dict(line=flags[b][n],text=v.decode('latin1')) for n,v in t.items()} for b,t in tables.items()},indent=2),encoding='utf-8')
    if wanted('inventory'):
        for board in tables:
            assert tables[board].keys()==flags[board].keys()
            assert all(b'\0' not in v for v in tables[board].values())
            assert all(flags[board]['HLP%d'%i] for i in range(1000,1023))
            print('PASS',board,len(tables[board]),'unique message identifiers, paired declarations/macros, no embedded NUL',flush=True)
        if not args.revision:
            project=ROOT/'Firmware/busPirate.X/nbproject/configurations.xml'
            xml=ET.parse(project)
            directory='core_messages/' if (ROOT/'Firmware/core_messages/messages.c').exists() else ''
            listed={n.text for n in xml.findall('.//itemPath')}
            for name in ('messages.c','messages.h','messages_v3.h','messages_v3.s','messages_v4.h','messages_v4.s'):
                assert '../'+directory+name in listed
            excluded={n.attrib['path'] for n in xml.findall('.//item') if n.attrib.get('ex')=='true'}
            for b in ('v3','v4'):
                for ext in ('.h','.s'):assert '../'+directory+'messages_'+b+ext in excluded
            if directory:assert project.read_text(encoding='utf-8').count('../core_messages')>=12
            print('PASS MPLAB lists all six message files and preserves per-board exclusions',flush=True)
    if wanted('regen'):
        assert not args.revision,'regeneration gate uses the current catalog'
        with tempfile.TemporaryDirectory(prefix='bp-regenerate-',dir=TEMP) as tmp:
            tools=ROOT/'tools/packstrings'
            for board in tables:
                out=Path(tmp)/('messages_'+board)
                subprocess.run([sys.executable,str(tools/'packstrings.py'),'bus_pirate_'+board+'_strings.txt',str(out),board],cwd=tools,check=True)
                for suffix in ('.s','.h'):
                    assert out.with_suffix(suffix).read_text(encoding='utf-8')==source('messages_'+board+suffix),'generator drift '+board+suffix
                print('PASS exact '+board+' regeneration (assembly and header)',flush=True)
    if wanted('colors'):
        assert tables['v3']['BPMSG1233']==b'1.(Br)\t2.(Rd)\t3.(Or)\t4.(Yw)\t5.(Gn)\t6.(Bu)\t7.(Pu)\t8.(Gy)\t9.(Wt)\t0.(Bk)'
        row=tables['v4']['BPMSG1256']
        assert row.count(b'\t')==11 and row.endswith(b'2.(Bk)\t1.(Wt)') and b'\x01' not in row
        print('PASS per-board colour rows, v4 twelve columns and no accidental SOH',flush=True)
    if wanted('help'):
        for b in tables:assert b';\tBits to read/write e.g. 0x55;2' in tables[b]['HLP1021']
        assert b'#\tReset' in tables['v3']['HLP1005']
        assert tables['v4']['HLP1018'].startswith(b'e\t')
        print('PASS both-board bit-width help and intentional board-specific controls',flush=True)
    if wanted('crlf'):
        for board,table in tables.items():
            assert all(b'\n\r' not in v for v in table.values()),board
        print('PASS both-board catalog line endings are CRLF, not reversed LFCR',flush=True)
    if wanted('packed'):
        for board in tables:packed(src,source('messages_'+board+'.h',args.revision),tables[board],flags[board],board)
    if wanted('nested'):nested(src)
    if wanted('agree'):
        prefix=PREFIX.replace('static unsigned long lookup(const char *s){assert(!strcmp(label,s));return base;}','')
        prefix=prefix.replace('static const char *label;','')
        # Unused table fixtures are unnecessary for this serial-only function.
        prefix=prefix[:prefix.index('static uint8_t TBLPAG;')]+r'''
static uint8_t answer,output[8];static unsigned used;
static uint8_t user_serial_read_byte(void){return answer;}
static void user_serial_transmit_character(uint8_t c){output[used++]=c;}
#define BPMSG1135 ((void)0)
#define bpBR do{user_serial_transmit_character(13);user_serial_transmit_character(10);}while(0)
'''
        compile_run(prefix,function(src,'agree'),r'''
int main(void){for(unsigned i=0;i<256;++i){answer=i;used=0;assert(agree()==(i=='y'||i=='Y'));assert(used==3&&output[0]==i&&output[1]==13&&output[2]==10);}puts("PASS agree all 256 input bytes, echo, y/Y and CRLF");}
''','agree')
    if wanted('cdc_zero'):cdc_zero(args.revision)
    if wanted('generator'):
        tools=ROOT/'tools/packstrings'
        invalid=['MSG_BAD\t0\t"bad\\q"\n','MSG_BAD\t0\t"bad\\x00"\n',
                 'MSG_BAD\t0\t"bad\\x0"\n','MSG_BAD\t0\t"bad\u20ac"\n',
                 '1BAD\t0\t"bad"\n','MSG_BAD\t2\t"bad"\n','MSG_BAD\t0\n',
                 'MSG_BAD\t0\tbad\n','MSG_BAD\t0\t"a"\nMSG_BAD\t0\t"b"\n']
        with tempfile.TemporaryDirectory(prefix='bp-generator-',dir=TEMP) as tmp:
            tmp=Path(tmp)
            for i,text in enumerate(invalid):
                src=tmp/('invalid%d.txt'%i);src.write_text(text,encoding='utf-8')
                out=tmp/('invalid%d'%i)
                r=subprocess.run([sys.executable,str(tools/'packstrings.py'),str(src),str(out),'test'],cwd=ROOT,capture_output=True,text=True)
                assert r.returncode and not out.with_suffix('.s').exists() and not out.with_suffix('.h').exists(),(i,r.stdout,r.stderr)
            (tmp/'leaf.txt').write_text('MSG_OK\t0\t"ok\\x1b"\nMSG_EMPTY\t1\t""\nMSG_BYTES\t0\t"\\x08A\\xFF9"\n',encoding='utf-8')
            src=tmp/'top.txt';src.write_text('#include leaf.txt\n',encoding='utf-8')
            out=tmp/'valid'
            subprocess.run([sys.executable,str(tools/'packstrings.py'),str(src),str(out),'test'],cwd=ROOT,check=True)
            assert assembly(out.with_suffix('.s').read_text(encoding='utf-8'))=={'MSG_OK':b'ok\x1b','MSG_EMPTY':b'','MSG_BYTES':b'\x08A\xff9'}
        print('PASS generator rejects malformed/duplicate/non-byte/NUL inputs before output; includes resolve relative to source',flush=True)
    if wanted('uart_fields'):
        uart=source('uart.c',args.revision)
        name='UARTsettings' if 'void UARTsettings(' in uart else 'uart_print_settings'
        body=function(uart,name)
        labels=re.search(rb'UART \((.*?)\)',tables['v4']['MSG_UART_MODE_HEADER'])[1].split()
        values=len(re.findall(r'\bbp_write_dec_(?:byte|word)\(',body))
        assert len(labels)==values,(labels,values)
        print('PASS UART settings serializer label/value field counts match',flush=True)


if __name__=='__main__':main()
