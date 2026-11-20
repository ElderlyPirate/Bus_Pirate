#include "base.h"
/*  Bus Pirate init function */
//bus pirate initialization
//setup clock, terminal UART, pins, LEDs, and display version info
void Initialize(void){

	CLKDIVbits.RCDIV0=0; //clock divider to 0
	AD1PCFG = 0xFFFF;                 // Default all pins to digital
	OSCCONbits.SOSCEN=0;	

	BP_LEDMODE_DIR=0; //MODE LED output

	IODIR|=(AUX+MOSI+CLK+MISO+CS);//AUX, MOSI, CLK, MISO, CS pins to input
	IOLAT&=(~AUX+MOSI+CLK+MISO+CS); //low pin

	#if defined(ENABLE_PULLUP_RESISTORS)
		BP_PULLUP_ON();	//enable pullups
	#else
		BP_PULLUP_OFF();//disable pullups
	#endif

	#if defined(ENABLE_VREG)
		BP_VREG_ON();		//enable vreg
	#else
		BP_VREG_OFF();		//disable vreg
	#endif

	//set pin configuration using peripheral pin select
	BP_TERM_RX=BP_TERM_RX_RP;   //Inputs UART1 RX RPINR18bits.U1RXR=4;
	BP_TERM_TX_RP=BP_TERM_TX; 		// Outputs UART1 TX RPOR1bits.RP3R=U1TX_IO;
}

//delay in MS
void bpDelayMS(const unsigned int delay){
	volatile unsigned char i;
	volatile unsigned int c;
	
	for(c=0;c<delay;c++){
		for(i=0;i<4;i++) bpDelayUS(250);
	}

}

//delay in US
void bpDelayUS(const unsigned char delay)
{
	volatile unsigned char i;
	
	for(i=0;i<delay;i++){//@32MHz = 16MIPS, 16I/uS
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		//asm( "nop" );
		//asm( "nop" );
		//asm( "nop" );
		//asm( "nop" );
	}
}

