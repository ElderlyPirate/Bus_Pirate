/* vim: set sw=8 ts=8 si et: */
/*********************************************
* Serial Peripheral Interface 
* Author: Guido Socher, Copyright: GPL 
* Copyright: GPL
**********************************************/
#include "base.h"
#include "redefines.h" /* Definitions to get the existing code working with the Bus Pirate */

//direction registers
#define SPIMOSI_TRIS 	BP_MOSI_DIR	
#define SPICLK_TRIS 	BP_CLK_DIR	
#define SPIMISO_TRIS 	BP_MISO_DIR	
#define SPICS_TRIS 		BP_CS_DIR	

//pin control registers
#define SPIMOSI 		BP_MOSI
#define SPICLK 			BP_CLK	
#define SPIMISO 		BP_MISO	
#define SPICS 			BP_CS	

//open drain control registers for OUTPUT pins
#define SPIMOSI_ODC 		BP_MISO_ODC	
#define SPICLK_ODC 			BP_CLK_ODC	
#define SPICS_ODC 			BP_CS_ODC	

// timing for software spi:
#define F_CPU 3686400UL  // 3.6864 MHz

static unsigned char sck_dur=1;
static unsigned char spi_in_sw=0;
//static unsigned char sck_dur=12;
//static unsigned char spi_in_sw=1;

// set the speed (used by PARAM_SCK_DURATION, page 31 in AVR068)
// For a clock of 3.6864MHz this is:
// SPI2X SPR1 SPR0 divider result_spi_Freq   avrisp dur
//  0     0    0     f/4      921kHz :        sckdur =0
//  0     0    1     f/16     230kHz :        sckdur =1
//  0     1    0     f/64     57kHz  :        sckdur =2
//  0     1    1     f/128    28kHz  :        sckdur =3
//
//SPI2X=SPSR bit 0
//SPR0 and SPR1 =SPCR bit 0 and 1


unsigned char  spi_set_sck_duration(unsigned char dur)
{
		SPI1STATbits.SPIEN = 0;//disable, just in case...
        spi_in_sw=0;
        if (dur >= 4){
        	// sofware spi very slow
            spi_in_sw=1;
            sck_dur=12;
			SPI1CON1 =0; //30khz
        }else if (dur >= 3){
            // 28kHz
            sck_dur=3;
			SPI1CON1 = 0; //30khz
        }else if (dur >= 2){
            // 57kHz
            sck_dur=2;
			SPI1CON1 =0b1000; //63khz
        }else if (dur == 1){
            // 230kHz
            sck_dur=1;
			SPI1CON1 =0b11100; //250khz
        }else if (dur == 0){
            // 921kHz
            sck_dur=0;
			SPI1CON1 = 0b11101; //1000khz
        }
		/* CKE=1, CKP=0, SMP=0 */
		SPI1CON1bits.MSTEN=1;
		//SPI1CON1bits.CKP=0;
		SPI1CON1bits.CKE=1;		
		//SPI1CON1bits.SMP=0;
	    SPI1CON2 = 0;
	    SPI1STAT = 0;    // clear SPI
	    SPI1STATbits.SPIEN = 1;

        return(sck_dur); 
}

unsigned char spi_get_sck_duration(void)
{
        return(sck_dur);
}

void spi_sck_pulse(void)
{
	RPOR4bits.RP8R=0;// remove SPI CLK from pin
	SPICLK=0; // SCK low
	_delay_ms(0.10);
	SPICLK=1; // SCK high
	_delay_ms(0.10);
	SPICLK=0; // SCK low
	RPOR4bits.RP8R=SCK1OUT_IO; 	//reassign SPI CLK
	Nop();
}

void spi_reset_pulse(void) 
{
        /* give a positive RESET pulse, we can't guarantee
         * that SCK was low during power up (see Atmel's
         * data sheets, search for "Serial Downloading in e.g atmega8 
         * data sheet):
         * the programmer can not guarantee that SCK is held low during
         * Power-up. In this case, RESET must be given a positive pulse of at least two
         * CPU clock cycles duration after SCK has been set to 0."*/
        SPICS=1; // reset = high = not active
        _delay_ms(0.10);
        SPICS=0; // reset = low, stay active
        delay_ms(20); // min stab delay
}

void spi_init(void) 
{
        SPICS=1; 		// pb2 as reset pin is output, this is also the ss pin which
                    	// must be output for master to work.
        SPICS_TRIS=0;	// reset = high = not active

		// now output pins low in case somebody used it as output in his/her circuit
		//MOSI, SCK is output/low
		SPI1STATbits.SPIEN = 0;//disable, just in case...

		#ifdef OUTPUT_HIGH_IMPEDANCE
			SPIMOSI_ODC=1;
			SPICLK_ODC=1; 
			SPICS_ODC=1;
		#else
			SPIMOSI_ODC=0;
			SPICLK_ODC=0; 
			SPICS_ODC=0;
		#endif
		// Inputs 
		RPINR20bits.SDI1R=7; 		//B7 MISO
		// Outputs
		RPOR4bits.RP9R=SDO1_IO; 	//B9 MOSI
		RPOR4bits.RP8R=SCK1OUT_IO; 	//B8 CLK	
		//pps configures pins and this doesn't really matter....
		SPICLK_TRIS=0; 			//B8 sck output
		SPIMISO_TRIS=1; 		//B7 SDI input
		SPIMOSI_TRIS=0; 		//B9 SDO output
		SPICLK=0; 			
		SPIMISO=0; 		
		SPIMOSI=0; 		

        delay_ms(20); // discharge MOSI/SCK

                /* Enable SPI, Master, set clock rate fck/16 */
                spi_set_sck_duration(sck_dur);
                // now SCK and MOSI are under control of SPI

        SPICS=0; // reset = low, stay active
        delay_ms(20); // stab delay
        spi_reset_pulse();
}

// send 8 bit, return received byte
unsigned char spi_mastertransmit(unsigned char data)
{
                /* Start transmission in hardware*/
               	SPI1BUF = data;
                /* Wait for transmission complete */
                while(!IFS0bits.SPI1IF);
				data=SPI1BUF;
				IFS0bits.SPI1IF = 0;
				return data;
}

// send 16 bit, return last rec byte
unsigned char spi_mastertransmit_16(unsigned int data)
{
        spi_mastertransmit((data>>8)&0xFF);
        return(spi_mastertransmit(data&0xFF));
}

// send 32 bit, return last rec byte
unsigned char spi_mastertransmit_32(unsigned long data)
{
        spi_mastertransmit((data>>24)&0xFF);
        spi_mastertransmit((data>>16)&0xFF);
        spi_mastertransmit((data>>8)&0xFF);
        return(spi_mastertransmit(data&0xFF));
}


void spi_disable(void)
{

	SPICS=1;// reset = high, off
	
	// SPI off
	SPI1STATbits.SPIEN = 0;
	RPINR20bits.SDI1R=0b11111;  //B7 MISO
	RPOR4bits.RP9R=0; 			//B9 MOSI
	RPOR4bits.RP8R=0; 			//B8 CLK
	//disable all open drain control register bits
	SPIMOSI_ODC=0;
	SPICLK_ODC=0; 
	SPICS_ODC=0;

	//reset pins
	SPIMOSI_TRIS=1; 		//B9 SDO output
	SPIMOSI=0; 	
	SPICLK_TRIS=1; 			//B8 sck output
	SPICLK=0; 
	SPIMISO_TRIS=1; 		//B7 SDI input			
	SPIMISO=0; 		
	SPICS_TRIS=1;// reset pin as input, high impedance
	SPICS=0;
}

