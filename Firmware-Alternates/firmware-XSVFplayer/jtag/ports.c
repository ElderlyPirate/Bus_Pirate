//Our open source (CC-0) version of the ports.c from XAPP058
// CC-0, public domain
//
// You'll also need lenval.c and micro.c from XAPP058
// IT doesn't have a stated license so we're hesitant to include it in SVN
// Get it here: http://www.xilinx.com/support/documentation/application_notes/xapp058.zip
#include "../base.h"
#include "ports.h"
#include "../jtag.h"
#define MAX_BUFFER 4096
static unsigned char buf[MAX_BUFFER]; //buffer to hold incoming bytes
static unsigned int bufBytes=0, bufPointer=0;

void xsvf_setup(void){
	bufBytes=0;
	JTAGTDI_TRIS=0;
	JTAGTCK_TRIS=0;
	JTAGTD0_TRIS=1;
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
		UART1TX(0xff);
    	while(U1STAbits.URXDA == 0);
		bh=UART1RX();//get up to 255 bytes each time
		bl=UART1RX();//get up to 255 bytes each time
		bufBytes=(bh<<8)|bl; 
		for(i=0; i<bufBytes; i++){
    		while(U1STAbits.URXDA == 0);
			buf[i]=UART1RX();
		}
		bufPointer=0;
	}

	//(*data)=buf[bufPointer];
	bufPointer++;		
	bufBytes--;
}

unsigned char readTDOBit(){return JTAGTDO;}

void waitTime(long microsec){
    volatile long i; 	
	
    for ( i = 0; i < microsec; ++i ){
		bpDelayUS(1);
    }
}
