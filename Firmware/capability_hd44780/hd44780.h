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

#ifndef BP_HD44780_H
#define BP_HD44780_H

#include "configuration.h"

#ifdef BP_ENABLE_HD44780_SUPPORT

unsigned int LCDread(void);
unsigned int LCDwrite(unsigned int c);
void LCDstart(void);
void LCDstop(void);
void LCDsetup(void);
void LCDsetup_exc(void);
void LCDmacro(unsigned int c);
void LCDpins(void);

#ifndef BP_ENABLE_SPI_SUPPORT
void spi_disable_interface(void);
#endif /* !BP_ENABLE_SPI_SUPPORT */

#endif /* BP_ENABLE_HD44780_SUPPORT */

#endif /* !BP_HD44780_H */