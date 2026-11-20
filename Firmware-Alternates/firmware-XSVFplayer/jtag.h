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

//we need the pin definitions here for use in port.c of the XSVF programmer
#define JTAGTDI_TRIS BP_MOSI_DIR
#define JTAGTCK_TRIS BP_CLK_DIR
#define JTAGTD0_TRIS BP_MISO_DIR
#define JTAGTMS_TRIS BP_CS_DIR

#define JTAGTDI BP_MOSI
#define JTAGTCK BP_CLK
#define JTAGTDO BP_MISO
#define JTAGTMS BP_CS

void jtag(void);




