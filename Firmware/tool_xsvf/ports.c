//Our open source (CC-0) version of the ports.c from XAPP058
// CC-0, public domain
//
// You'll also need lenval.c and micro.c from XAPP058
// IT doesn't have a stated license so we're hesitant to include it in SVN
// Get it here: http://www.xilinx.com/support/documentation/application_notes/xapp058.zip

#include "../configuration.h"

#ifdef BP_ENABLE_JTAG_SUPPORT

#ifdef BP_JTAG_XSVF_SUPPORT

#include "ports.h"
#include "../protocol_jtag/jtag.h"
#define MAX_BUFFER 4096
static unsigned char buf[MAX_BUFFER]; //buffer to hold incoming bytes
static unsigned int bufBytes=0, bufPointer=0;

#define XSVF_STREAM_TIMEOUT_MS 5000

/* Set when the host stops feeding a chunk; the player then aborts at the
   next command boundary instead of parking the port until a power cycle. */
bool xsvf_stream_stalled = false;

void xsvf_setup(void){
        bufBytes=0;
        xsvf_stream_stalled = false;
        JTAGTDI_TRIS=0;
        JTAGTCK_TRIS=0;
        JTAGTDO_TRIS=1;
        JTAGTMS_TRIS=0;
}

void setPort(short p,short val){
    if (p==TMS) {JTAGTMS = (unsigned char) val;}//  bpDelayUS(10);}
    if (p==TDI) {JTAGTDI = (unsigned char) val;}//  bpDelayUS(10);}
    if (p==TCK) {JTAGTCK = (unsigned char) val;}//  bpDelayUS(50);}
}

static int xsvf_wait_byte(unsigned char *out){
        uint32_t waited = 0;

        if (xsvf_stream_stalled){
                *out = 0xff;            /* consumed as an illegal command */
                return 0;
        }

        while (!user_serial_ready_to_read()){
                if (++waited > XSVF_STREAM_TIMEOUT_MS){
                        /* The host vanished mid-chunk: give up after a
                           generous wait so the port can be recovered with
                           the 0x0F escape instead of a power cycle. */
                        xsvf_stream_stalled = true;
                        *out = 0xff;
                        return 0;
                }
                bp_delay_ms(1);
        }

        *out = user_serial_read_byte();
        return 1;
}

void readByte(unsigned char *data){
        unsigned int i;
        unsigned char bh, bl;

        if(bufBytes==0){
                do{
                user_serial_transmit_character(0xff);
                        xsvf_wait_byte(&bh);    //chunk length, big endian
                        xsvf_wait_byte(&bl);
                bufBytes=(bh<<8)|bl;
                if (xsvf_stream_stalled){
                        bufBytes = 0;
                }
                for(i=0; i<bufBytes; i++){
                        unsigned char incoming;
                        xsvf_wait_byte(&incoming);

                        /* Drain-discard anything past the buffer: a
                           hostile or desynced 16-bit chunk length must
                           never overrun buf[], and the stream must stay
                           aligned for the next chunk. */
                        if(i<MAX_BUFFER){
                                buf[i]=incoming;
                        }
                }
                if(bufBytes>MAX_BUFFER){
                        bufBytes=MAX_BUFFER;
                }
                bufPointer=0;
                /* a zero-length chunk is not data: re-request; it previously
                   underflowed bufBytes (0-1 -> 0xFFFF) and served stale .bss. */
                }while(bufBytes==0 && !xsvf_stream_stalled);
                if (xsvf_stream_stalled && bufBytes==0){
                        /* keep the serve below defined: 0xFF is an illegal
                           command, so the player exits at the next step */
                        buf[0] = 0xff;
                        bufBytes = 1;
                }
        }

        (*data)=buf[bufPointer];
        bufPointer++;
        bufBytes--;
}

unsigned char readTDOBit(){return JTAGTDO;}

void waitTime(long microsec){
    volatile long i;

    /* The host only sends while the player is blocked on a chunk
       request; a byte arriving while the player is busy therefore means
       the host wants out.  Mark the stream stalled (the run unwinds at
       the next command boundary) and skip the rest of the wait. */
    while (user_serial_ready_to_read()){
        (void)user_serial_read_byte();
        xsvf_stream_stalled = true;
    }
    if (xsvf_stream_stalled){
        return;
    }

    /* Coarse chunks first: one calibrated delay builtin per whole
       millisecond instead of one per microsecond (the per-us loop
       below adds its own overhead to every pass, so long XRUNTEST/
       XWAIT waits used to run well past their nominal duration). */
    while (microsec >= 1000){
        bp_delay_us(1000);
        microsec -= 1000;
    }

    for ( i = 0; i < microsec; ++i ){
                bp_delay_us(1);
    }
}

#endif /* BP_JTAG_XSVF_SUPPORT */

#endif /* BP_ENABLE_JTAG_SUPPORT */