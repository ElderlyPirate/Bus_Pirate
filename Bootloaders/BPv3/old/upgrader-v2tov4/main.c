/*
 * This file is part of the Bus Pirate project (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has
 * waived all copyright and related or neighboring rights to Bus Pirate. 
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

// CvD: buspirate subs copied from the bus pirate svn
// CvD: Did I already mention it is highly alpha?? use with caution and only if you have a programmer to undo this!!
 

#include "hardware.h"
#include "flash.h"
#include "firmware.h"

// functions
void BpInit(void);
int main(void);
void bpWstring(char *s);
void bpWinthex(unsigned int c);
void bpWhex(unsigned int c);
void bpWstring(char *s);
void bpWline(char *s);
void UART1TX(char c);
void bpDelayUS(const unsigned char delay);
void bpDelayMS(const unsigned char delay);
unsigned char UART1RXRdy(void);
unsigned char UART1RX(void);
void usermessage(void);
unsigned char checkChar(unsigned char c);

// cvD: needed??
_CONFIG2(FNOSC_FRCPLL & OSCIOFNC_ON &POSCMOD_NONE & I2C1SEL_PRI)		// Internal FRC OSC = 8MHz
_CONFIG1(JTAGEN_OFF & GCP_OFF & GWRP_OFF & COE_OFF & FWDTEN_OFF & ICS_PGx1) //turn off junk we don't need

/*
	Store delay timeout value & user reset vector at start of user space

	Value of userReset should be the start of actual program code since 
	these variables will be stored in the same area.

	some heritage from the old bootloader
*/
unsigned int userReset  __attribute__ ((space(prog),section(".init"))) = 0xC04 ;
unsigned char timeout  __attribute__ ((space(prog),section(".init"))) = 0x00 ;


// it all happens from here
int main(void)
{	int i,j;
	unsigned char buf[3*64];

	BpInit();
	BP_LEDMODE=1;

	//wait for confirmation
	while(1){
		usermessage();
		if(checkChar('Y'))					// absorb 'Yes' and echo it back
		{									// if any typo is made it should cancel and start over again
			if(checkChar('E'))
			{	
				if(checkChar('S')) 
				{
					bpWline("");
					break;
				}
			}
		}

		bpWline("    Upgrade cancelled.");
		bpWline("");
	}
	bpWline("");
	bpWline("Sit back and enjoy the show");

	// the user is really sure
	// erase the page before the bootloader 
	bpWstring("Erasing ");
	bpWinthex(FWLOCATION-0x0400);
	bpWstring("...");					// I love dots ;)
	Ferase(FWLOCATION);
	bpWline(" done");

	// erase the bootloader location
	bpWstring("Erasing ");
	bpWinthex(FWLOCATION);
	bpWstring("...");					// I love dots ;)
	Ferase(FWLOCATION);
	bpWline(" done");

	// write 8 rows (8*64=512 'words')
	bpWstring("Writing row ");
	for(i=0; i<8; i++)
	{	UART1TX(0x30+i);
		UART1TX(' ');

		for(j=0; j<3*64; j++)			// fill buffer
		{	buf[j]=firmware[(i*192)+j];
		}
		Fwrite(buf, FWLOCATION+(i*128));	// 1 row = 64 words 	
	}
	bpWline("done");

	#ifndef DEBUG
		// erase 1st page and write jump to new bootloader
		bpWstring("Erasing 0x0000...");
		Ferase(0x0000);
		bpWline(" done");
	
		bpWstring("Writing row 0... ");
		for(j=0; j<(3*64); j++) buf[j]=0xFF;// 1's are easier to write
		buf[0]=0x04;							// goto FWLOCATION
		buf[1]=(FWLOCATION&0x00FF);	
		buf[2]=((FWLOCATION&0xFF00)>>8);
		buf[3]=0x00;							// goto FWLOCATION
		buf[4]=0x00;	
		buf[5]=0x00;
		Fwrite(buf, 0); 
		bpWline("done");	
	#endif

	// finish up 
	bpWline("");
	bpWline("Success!");
	bpWline("Upload v4+ firmware with the ds30 Loader app to complete the upgrade.");// Yo American! can the buspirate sniff serial comma's? :P

	while(1)
	{	BP_LEDMODE=1;						// we want some attention!
		bpDelayMS(250);
		bpDelayMS(250);
		BP_LEDMODE=0;
		bpDelayMS(250);
		bpDelayMS(250);
	}

	return 0;
}

unsigned char checkChar(unsigned char c){
	unsigned char d;
	d=UART1RX();
	UART1TX(d);	
	if(d>='a' && d<='z') d-=0x20; //convert to upper
	if(c==d) return 1;
	return 0;
}

void usermessage(void){
	bpWline("");
	bpWline("DS30 Loader installer v0.2 (ALPHA)");
	bpWline("(C)hris 2010");
	bpWline("Released under Creative Commons null license.");
	#ifdef DEBUG
	bpWline("RUNNING IN DEBUG MODE!!!!");
	#endif
	bpWline("This will install bootloader v4.1.");
	bpWline("Type 'Yes' to program the new bootloader.");
	bpWline("Disconnect power if unsure (no harm done).");
	bpWline("");
}

// mostly copied from base.c, baseIO.c and baseUI.h
// CvD: how to keep synchronized with changes in the buspirate trunk??

void BpInit(void)
{
	CLKDIVbits.RCDIV0=0; //clock divider to 0
	AD1PCFG = 0xFFFF;                 // Default all pins to digital
	OSCCONbits.SOSCEN=0;	

	BP_TERM_RX=BP_TERM_RX_RP;		//Inputs UART1 RX RPINR18bits.U1RXR=4;
	BP_TERM_TX_RP=BP_TERM_TX;		// Outputs UART1 TX RPOR1bits.RP3R=U1TX_IO;

	BP_MOSI_DIR=1;
	BP_CLK_DIR=1; 
	BP_MISO_DIR=1;
	BP_CS_DIR=1; 
	BP_AUX_DIR=1; 
	BP_LEDMODE=0; //mode LED OFF
	BP_LEDMODE_DIR=0;
	#ifdef BUSPIRATEV1A
		BP_PIN6_DIR=1; //AUX2... unused
		BP_LEDPWR=1; //light power led
		BP_LEDPWR_DIR=0;
	#endif

	#if defined( BUSPIRATEV0A) || defined( BUSPIRATEV2)
		BP_PULLUP_OFF();
	#endif

	#if defined( BUSPIRATEV1A) || defined( BUSPIRATEV2)
		
		BP_VREG_OFF();//disable the VREG

		//setup voltage monitoring on ADC. Should be moved to hardwarevx.h!
		//AD1PCFGbits.PCFG9=0; //B15/AN9/ADC4/50V
		//AD1PCFGbits.PCFG10=0;//B14/AN10/ADC3/33V
		//AD1PCFGbits.PCFG11=0;//B13/AN11/ADC2/Vextpullup
		//AD1PCFGbits.PCFG12=0;//B12/AN12/ADC1/EXT
		//AD1PCFG&=(~0b11110);//analog pins
	
		//configure the ADC
		//AD1CON1bits.SSRC  = 0b111;// SSRC<3:0> = 111 implies internal
									// counter ends sampling and starts
									// converting.
		//AD1CSSL = 0;
		//AD1CON3 = 0x1F02; // Sample time = 31Tad,
							// Tad = 2 Tcy
		//AD1CON2 = 0;
	#endif

    U1BRG = 34;					//13332=300, 1666=2400, 416=9600, 34@32mhz=115200....
    U1MODE = 0;
    U1MODEbits.BRGH = 1;
    U1STA = 0;
    U1MODEbits.UARTEN = 1;
    U1STAbits.UTXEN = 1;
    IFS0bits.U1RXIF = 0;

}

//Write a string to the user terminal
void bpWstring(char *s){
    char c;
    while((c = *s++)) UART1TX(c);
}

//write a string to the user terminal, finish with a line break
void bpWline(char *s){
    char c;
    while((c = *s++)) UART1TX(c);
	UART1TX(0x0d);
	UART1TX(0x0a);

}

//output an 8bit/byte hex value to the user terminal
const unsigned char HEXASCII[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
void bpWhex(unsigned int c){
    unsigned int b;

	bpWstring("0x");
    b = (c>>4) & 0x0F;
    UART1TX(HEXASCII[b]);
    b = c & 0x0F;
    UART1TX(HEXASCII[b]);
    return;
}

// output a 16bit hex value to the user terminal
void bpWinthex(unsigned int c) {
    unsigned int b;
    
    bpWstring("0x");
    b = (c>>12) & 0x0F;
    UART1TX(HEXASCII[b]);
    b = (c>>8) & 0x0F;
    UART1TX(HEXASCII[b]);
    b = (c>>4) & 0x0F;
    UART1TX(HEXASCII[b]);
    b = c & 0x0F;
    UART1TX(HEXASCII[b]);
    return;
}

//add byte to buffer, pause if full
//uses PIC 4 byte UART FIFO buffer
void UART1TX(char c){
	while(U1STAbits.UTXBF == 1); //if buffer is full, wait
    U1TXREG = c;
}

//delay in MS
void bpDelayMS(const unsigned char delay){
	volatile unsigned int i;
	volatile unsigned char c;
	
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
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
		asm( "nop" );
	}
}



unsigned char UART1RXRdy(void){
    return U1STAbits.URXDA;
}

//get a byte from UART
unsigned char UART1RX(void){

    while(U1STAbits.URXDA == 0);
	return U1RXREG;
}





