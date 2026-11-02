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

void xsvf_setup(void){
        bufBytes=0;
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

void readByte(unsigned char *data){
        unsigned int i;
        unsigned char bh, bl;

        if(bufBytes==0){
                do{
                user_serial_transmit_character(0xff);
       //--- while(U1STAbits.URXDA == 0);  //--- dont use it as already in UART1RX => dead lock
                bh=user_serial_read_byte();//get up to 255 bytes each time
                bl=user_serial_read_byte();//get up to 255 bytes each time
                bufBytes=(bh<<8)|bl;
                for(i=0; i<bufBytes; i++){
                        unsigned char incoming=user_serial_read_byte();

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
                }while(bufBytes==0);
        }

        (*data)=buf[bufPointer];
        bufPointer++;
        bufBytes--;
}

unsigned char readTDOBit(){return JTAGTDO;}

void waitTime(long microsec){
    volatile long i;

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