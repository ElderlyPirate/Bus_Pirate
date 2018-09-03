/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
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

#ifndef BP_RAW_COMMON_H
#define BP_RAW_COMMON_H

#include "configuration.h"

#if defined(BP_ENABLE_RAW_2WIRE_SUPPORT) || defined(BP_ENABLE_RAW_3WIRE_SUPPORT)

#include <stdbool.h>
#include <stdint.h>

bool raw_read_bit(void);
uint16_t raw_read_pin(void);
void raw_set_clock_high(void);
void raw_set_clock_low(void);
void raw_set_data_low(void);
void raw_set_data_high(void);
void raw_advance_clock(void);

#endif /* BP_ENABLE_RAW_2WIRE_SUPPORT || BP_ENABLE_RAW_3WIRE_SUPPORT */

#endif /* !BP_RAW_COMMON_H */
