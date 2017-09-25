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



/* CvD: most of this is copied and pasted from the svn trunk from the buspirate */
/* CvD: should we link to there or leave as is? */

#ifndef BASE 
#define BASE
#include <p24fxxxx.h>

//#define DEBUG //safe debug release, won't overwrite bootloader v2

//this sets the hardware version
//#define BUSPIRATEV0A //http://hackaday.com/2008/11/19/how-to-the-bus-pirate-universal-serial-interface/
//#define BUSPIRATEV2A
//#define BUSPIRATEV25 //AKA Bus Pirate v2go
//#define BUSPIRATEV1A //http://hackaday.com/2009/01/22/how-to-bus-pirate-v1-improved-universal-serial-interface/
#define BUSPIRATEV3 

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
#elif defined(BUSPIRATEV3)
	#define BUSPIRATEV2
	#include "hardwarev3.h"
#else
	#error "No hardware defined in hardware.h."
#endif

#endif

