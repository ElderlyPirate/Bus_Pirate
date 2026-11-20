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
//http://dangerousprototypes.com/docs/Bus_Pirate_JTAG_XSVF_player
#include "base.h"
#include "jtag.h"

// Internal FRC OSC = 8MHz
#pragma config FNOSC = FRCPLL // Fast RC Oscillator with Postscaler and PLL Module (FRCDIV+PLL)
#pragma config OSCIOFNC = ON  // Primary Oscillator Output Function
#pragma config POSCMOD = NONE // Primary Oscillator is Disabled
#pragma config I2C1SEL = PRI  // I2C1 Pin Location Select
// Turn off junk we don't need
#pragma config JTAGEN = OFF   // JTAG Port
#pragma config GCP = OFF      // General Segment Code Protection
#pragma config GWRP = OFF     // General Code Segment Write Protect
#pragma config FWDTEN = OFF   // Watchdog Timer
#pragma config ICS = PGx1     // Emulator Pin Placement Select bits

#pragma code
//this loop services user input and passes it to be processed on <enter>
int main(void){

	CLKDIVbits.RCDIV0=0; //clock divider to 0
    AD1PCFG = 0xFFFF;                 // Default all pins to digital
	OSCCONbits.SOSCEN=0;	

	bpInit();
	
	InitializeUART1(); //init the PC side serial port (needs bpInit() first for speed setting)

	BP_VREG_ON();
	
	jtag();
	
	while(1); //while
}




