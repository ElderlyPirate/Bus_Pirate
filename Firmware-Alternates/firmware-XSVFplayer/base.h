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

#ifndef BASE
#define BASE

#include <xc.h>

//this sets the hardware version (v0a, or v1a)
//#define BUSPIRATEV0A //http://hackaday.com/2008/11/19/how-to-the-bus-pirate-universal-serial-interface/
//#define BUSPIRATEV1A //http://hackaday.com/2009/01/22/how-to-bus-pirate-v1-improved-universal-serial-interface/
//#define BUSPIRATEV2A
#define BUSPIRATEV25 //AKA Bus Pirate v2go

#if defined(BUSPIRATEV0A)
	#include "hardwarev0a.h"
#elif defined(BUSPIRATEV1A)
	#include "hardwarev1a.h"
#elif defined(BUSPIRATEV2A)
	#define BUSPIRATEV2
	#include "hardwarev2a.h"
#elif defined(BUSPIRATEV25)
	#define BUSPIRATEV2
	#include "hardwarev25.h"
#else
	#error "No hardware defined in base.h."
#endif

//reset all peripherals before configuring a new bus protocol
void bpInit(void); 

//Initialize the terminal UART for the speed currently set in bpConfig.termSpeed
void InitializeUART1(void); 

//get a byte from UART
unsigned char UART1RX(void); 

//add byte to buffer, pause if full
//uses PIC 4 byte UART FIFO buffer
void UART1TX(char c); 

//delays used by many libraries
void bpDelayMS(const unsigned char delay);
void bpDelayUS(const unsigned char delay);
#endif
