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

#ifndef BP_RAW3WIRE_H
#define BP_RAW3WIRE_H

#include "configuration.h"

#ifdef BP_ENABLE_RAW_3WIRE_SUPPORT

#include <stdbool.h>
#include <stdint.h>

uint16_t raw3wire_read(void);
uint16_t raw3wire_write(const uint16_t value);
void raw3wire_start_with_read(void);
void raw3wire_start(void);
void raw3wire_stop(void);
void raw3wire_setup_execute(void);
void raw3wire_setup_prepare(void);
void raw3wire_print_pins_state(void);
void raw3wire_print_settings(void);

#endif /* BP_ENABLE_RAW_3WIRE_SUPPORT */

#endif /* !BP_RAW3WIRE_H */
