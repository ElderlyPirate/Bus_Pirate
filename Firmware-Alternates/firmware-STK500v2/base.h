/*
 * This file is part of the Bus Pirate project (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has
 * waived all copyright and related or neighboring rights to Bus Pirate. This
 * work is published from United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef BASE 
#define BASE

#include <xc.h>

#define BUSPIRATEV3 

//these compile time options enable various features
#define ENABLE_VREG 			//voltage regulators always on
//#define OUTPUT_HIGH_IMPEDANCE	//use high impedance outputs for multi-voltage interfacing
#ifdef OUTPUT_HIGH_IMPEDANCE	
#define ENABLE_PULLUP_RESISTORS //enable on-board pullup resistors (only if Hi-Z, only on hardware v2, v3)
#endif

//
//
//
// END CONFIG AREA
//
//
//
#define BUSPIRATEV2
#include "hardwarev3.h"

void Initialize(void);

//delays used by many libraries
void bpDelayMS(const unsigned int delay);
void bpDelayUS(const unsigned char delay);
#endif
