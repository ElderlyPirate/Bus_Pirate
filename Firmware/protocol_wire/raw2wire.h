/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has waived all copyright and
 * related or neighboring rights to Bus Pirate. This work is published from
 * United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef BP_RAW2WIRE_H
#define BP_RAW2WIRE_H

#include "configuration.h"

#ifdef BP_ENABLE_RAW_2WIRE_SUPPORT

#include <stdbool.h>
#include <stdint.h>

void raw2wire_start(void);
void raw2wire_stop(void);
uint16_t raw2wire_write(const uint16_t value);
uint16_t raw2wire_read(void);
void raw2wire_run_macro(const uint16_t macro_id);
void raw2wire_setup_execute(void);
void raw2wire_setup_prepare(void);
void raw2wire_print_pins_state(void);
void raw2wire_print_settings(void);

#endif /* BP_ENABLE_RAW_2WIRE_SUPPORT */

#endif /* !BP_RAW2WIRE_H */
