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

#ifndef BP_DIO_H
#define BP_DIO_H

#include "configuration.h"

#ifdef BP_ENABLE_DIO_SUPPORT

/**
 * Reads a value from the device.
 * 
 * @return the value being read.
 */
unsigned int dio_read(void);
void dio_setup_prepare(void);

/**
 * Writes a value to the device.
 * 
 * The given value can be either a state or direction.  If bit 7 (0x80) is set, then
 * the value will be interpreted as a state change mask, otherwise it will be
 * interpreted as a direction change mask.
 * 
 * @param[in] value the value to write.
 * 
 * @see binBBpinset
 * @see binBBpindirectionset
 */
unsigned int dio_write(unsigned int value);

#endif /* BP_ENABLE_DIO_SUPPORT */

#endif /* !BP_DIO_H */