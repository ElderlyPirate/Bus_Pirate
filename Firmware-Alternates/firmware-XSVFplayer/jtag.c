/*
 * This file is part of the Bus Pirate project (buspirate.com).
 *
 * Originally written by hackaday.com <legal@hackaday.com>
 *
 * To the extent possible under law, hackaday.com <legal@hackaday.com> has
 * waived all copyright and related or neighboring rights to Bus Pirate. This
 * work is published from United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "base.h"
#include "jtag.h"
#include "jtag/micro.h"
#include "jtag/ports.h"

#define JTAGDATASETTLE 20
#define JTAGCLOCK 100

#define RESET 0
#define IDLE 1
#define SHIFTIR 2
#define SHIFTDR 3

//private functions
void jtagSetup(void);

//these are JTAG state machine related
void jtagSetState(unsigned char c);
void jtagLeaveState(void);
void jtagReset(void);
void jtagCleanPendingBit(void);
//high level byte operations
unsigned char jtagWriteByte(unsigned char c);
unsigned char jtagReadByte(void);
//bit level functions
unsigned char jtagWriteBit(unsigned char c);
unsigned char jtagReadBit(void);
unsigned char jtagReadDataState(void);
//bus level control
void jtagDataLow(void);
void jtagDataHigh(void);
void jtagClockLow(void);
void jtagClockHigh(void);
void jtagClockTicks(unsigned char c);
void jtagTMSHigh(void);
void jtagTMSLow(void);

static unsigned char jtagState=0, jtagBitPending=0, jtagDelayedBit=0;//static configuration variables for highZ and LSB

struct _JTAG{
	unsigned char HiZ:1;
} jtagSettings;


void jtag(void){
	static unsigned char c, cmd;
	static unsigned int i;

	jtagSetup();
	jtagSettings.HiZ=1;

	while(1){
	cmd=UART1RX();
	switch(cmd){
		//bpWline("JTAG READY");
		case 1://jtag reset
			jtagReset();//reset
			//bpWline("JTAGSM: RESET");
			jtagLeaveState();//move chain to idle (gives own message)
			break;
		case 2://read ID, chain length, # devices
			//bpWline("JTAG INIT CHAIN");
			jtagReset();//reset
			//bpWline("JTAGSM: RESET");
			//data high
			jtagDataHigh();
			//how many devices?
			//[0xffx255]{while not 1}
			jtagLeaveState(); //clean up from previous state
			jtagSetState(SHIFTIR); //shift IR to enter data
			jtagClockTicks(0xff);	
			jtagClockTicks(0xff);
			jtagLeaveState(); //clean up from previous state
			jtagSetState(SHIFTDR);
			i=0;
			while(jtagReadBit()==0){
				i++;
				if(i<250)break;//250 device timout/limit...
			}		
			jtagLeaveState(); //clean up from previous state			
			//reset
			jtagReset();
			//bpWline("JTAGSM: RESET");

			//read ID#s (32 bits * devices) {r: (4*devices)}
			jtagLeaveState(); //clean up from previous state
			jtagSetState(SHIFTDR);
			//bpWline("JTAG CHAIN REPORT:");	
			UART1TX(i*4); //how many bytes are we returning
			for(c=0;c<i;c++){
				UART1TX(jtagReadByte());
				UART1TX(jtagReadByte());
				UART1TX(jtagReadByte());
				UART1TX(jtagReadByte());
			}
			jtagLeaveState(); //clean up from previous state
			break;
		case 3://XSFV player
			//data MUST be low when we start or we get error 3!
			jtagDataLow();
			jtagClockLow();
			jtagTMSLow();
			xsvf_setup();
/*
			while(1){
				readByte(i);
			}
*/

			// just return the code
			#define XSVF_ERROR_NONE         0
			#define XSVF_ERROR_UNKNOWN      1
			#define XSVF_ERROR_TDOMISMATCH  2
			#define XSVF_ERROR_MAXRETRIES   3   /* TDO mismatch after max retries */
			#define XSVF_ERROR_ILLEGALCMD   4
			#define XSVF_ERROR_ILLEGALSTATE 5
			#define XSVF_ERROR_DATAOVERFLOW 6   /* Data > lenVal MAX_LEN buffer size*/
			/* Insert new errors here */
			#define XSVF_ERROR_LAST         7			
			i=xsvfExecute();
			UART1TX(i);
			break;
		default:
			break;//bpWmessage(MSG_ERROR_MACRO);
	}//switch
	}//while
}//function


//exits an existing JTAG state, moves to IDLE
void jtagLeaveState(void){
	//move to IDLE
//	bpWstring("JTAGSM: ");
	switch(jtagState){
		case IDLE://already in idle
			//bpWline("ALREADY IDLE");
			break;
		case RESET://move to idle 0
			jtagTMSLow();
			jtagClockTicks(1);
			jtagState=IDLE;	
			//bpWline("RESET->IDLE");	
			break;
		case SHIFTIR://clean up pending writes...
			if(jtagBitPending==1){
				//set proper bit direction
				if(jtagDelayedBit==1){
					jtagDataHigh();
				}else{
					jtagDataLow();
				}
				jtagBitPending=0;//clear pending
				//bpWstring("(WROTE DELAYED BIT) ");	
			}
			jtagTMSHigh();
			jtagClockTicks(2);
			jtagTMSLow();//always return to low for writes	
			jtagClockTicks(1);
			jtagState=IDLE;	
			//bpWline("IR->IDLE");	
			break;
		case SHIFTDR://both same path 110
			jtagTMSHigh();
			jtagClockTicks(2);
			jtagTMSLow();//always return to low for writes	
			jtagClockTicks(1);
			jtagState=IDLE;	
			//bpWline("DR->IDLE");	
			break;
		default:
			//couldn't change state, error, try resetting state machine...	
			//bpWline("UNKNOWN STATE");	
			break;
	}
}

void jtagReset(void){
	jtagTMSHigh();
	jtagClockTicks(10);//one extra if clk starts high
	jtagTMSLow();//always return to low for writes	
	jtagState=RESET;	
}

//moves to specified state from IDLE (reset from anywhere)
void jtagSetState(unsigned char c){
	//if(jtagState!=IDLE) bpWline("JTAGSM: WARNING OUT OF SYNC");
	//bpWstring("JTAGSM: ");

	//move to desired state
	switch(c){
		case IDLE://alread idle
			//bpWline("ALREADY IDLE");
			break;
		case RESET://always return to reset with 11111
			jtagReset();
			//bpWline("RESET");
			break;
		case SHIFTDR://100 from IDLE
			jtagTMSHigh();
			jtagClockTicks(1);
			jtagTMSLow();//always return to low for writes	
			jtagClockTicks(2);
			jtagState=SHIFTDR;	
			//bpWline("IDLE->Data Register");
			break;
		case SHIFTIR: //1100 from IDLE
			jtagTMSHigh();
			jtagClockTicks(2);
			jtagTMSLow();//always return to low for writes	
			jtagClockTicks(2);
			jtagState=SHIFTIR;	
			//bpWline("IDLE->Instruction Register (DELAYED ONE BIT FOR TMS)");
			break;
		default:
		//unknown state, try resetting the JTAG state machine
			//bpWline("UNKNOWN STATE, TRY A RESET MACRO (1)");
			break;
	}
}

void jtagSetup(void){
	JTAGTD0_TRIS=1;//input from chain
	JTAGTCK_TRIS=0;
	JTAGTDI_TRIS=0;
	JTAGTMS_TRIS=0;			//B6 cs output is state machine control
	//writes to the PORTs write to the LATCH
	JTAGTDO=0;
	JTAGTCK=0;
	JTAGTDI=0;
	JTAGTMS=0;	
}

//this is a new write routine, untested. See old below...
unsigned char jtagWriteByte(unsigned char c){
	volatile unsigned char i,j,a,l;

	jtagClockLow();//begin with clock low...	
	
	//clean up any pending bits	
	if(jtagBitPending==1){
		jtagWriteBit(jtagDelayedBit);
		jtagBitPending=0;//clear pending
		//bpWstring("NOTE: WROTE DELAYED BIT\x0D\x0A");
	}

	//if(modeConfig.lsbEN==1) l=0x01; else l=0b10000000;
	l=0x01;

	for(i=0;i<8;i++){
		if( (c & l)== 0) jtagDataLow(); else jtagDataHigh();//setup the data pin
			
		jtagClockHigh();//set clock high

		j=JTAGTDO;	
		//if(modeConfig.lsbEN==1){
			c=c>>1;                 //-- Shift next bit into position	
			a=a>>1;
			if(j==1)a+=0b10000000;       
		//}else{
		//	c=c<<1; 			//-- Shift next bit into position
		//	a=a<<1;    
		//	if(j==1)a++;     
		//}

		jtagClockLow();//set clock low
		
		//catch bit seven and delay the write until we do the next byte or exit state with TMS=1....
		if(jtagState==SHIFTIR && i==6){
			jtagBitPending=1;
			if((c & l)==0) 	jtagDelayedBit=0; else jtagDelayedBit=1;
			return a;//meaningless....rather, 1 bit short
		}
	}
	return a;
}

unsigned char jtagReadByte(void){
	volatile unsigned char i,j,a;

	jtagClockLow();//begin with clock low...	
	
	if(jtagBitPending==1){
		jtagWriteBit(jtagDelayedBit);
		jtagBitPending=0;//clear pending
		//bpWstring("NOTE: WROTE DELAYED BIT\x0D\x0A");
	}

	for(i=0;i<8;i++){
		jtagClockHigh();//set clock high
		j=JTAGTDO;	
		//if(modeConfig.lsbEN==0){	
		//	a=a<<1;                 
		//	if(j)a++;			
		//}else{
			a=a>>1;
			if(j)a+=0b10000000;
		//}
		jtagClockLow();//set clock low
	}	

	return a;
}

unsigned char jtagWriteBit(unsigned char c){
	unsigned char i;

	if(c==0){
		jtagDataLow();
	}else{
		jtagDataHigh();
	}
	jtagClockHigh();//set clock high
	i=JTAGTDO;
	jtagClockLow();//set clock low
	return i;
}

unsigned char jtagReadBit(void){
	unsigned char i;

	jtagClockHigh();//set clock high
	i=JTAGTDO;
	jtagClockLow();//set clock low
	return i;
}

unsigned char jtagReadDataState(void){
	return JTAGTDO;
}

void jtagDataHigh(void){
	JTAGTDI_TRIS=(~jtagSettings.HiZ);//set output
	JTAGTDI=jtagSettings.HiZ;//data 
	bpDelayUS(JTAGDATASETTLE);//delay
}

void jtagDataLow(void){
	JTAGTDI=0; //data low
	JTAGTDI_TRIS=0;//set to output for HIGHZ low
	bpDelayUS(JTAGDATASETTLE);//delay
}

void jtagClockTicks(unsigned char c){
	unsigned char i;

	for(i=0;i<c;i++){
		jtagClockHigh();
		jtagClockLow();
	}
}

void jtagClockHigh(void){
	JTAGTCK_TRIS=(~jtagSettings.HiZ);//set output
	JTAGTCK=jtagSettings.HiZ;//data 
	bpDelayUS(JTAGCLOCK);//delay
}

void jtagClockLow(void){
	JTAGTCK=0;//set clock low	
	JTAGTCK_TRIS=0;//set clock output for HIGHZ
	bpDelayUS(JTAGCLOCK);//delay
}

void jtagTMSHigh(void){
	JTAGTMS_TRIS=(~jtagSettings.HiZ);//set output
	JTAGTMS=jtagSettings.HiZ;//data 
	bpDelayUS(JTAGDATASETTLE);//delay
}

void jtagTMSLow(void){
	JTAGTMS=0;//cs 
	JTAGTMS_TRIS=0;//cs output for HIGHZ
	bpDelayUS(JTAGDATASETTLE);//delay
}
