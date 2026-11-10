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
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef BP_PC_AT_KEYBOARD_H
#define BP_PC_AT_KEYBOARD_H

#include "configuration.h"

#ifdef BP_ENABLE_PC_AT_KEYBOARD_SUPPORT

#include <stdint.h>

void pc_at_keyboard_prepare(void);
void pc_at_keyboard_execute(void);
uint16_t pc_at_keyboard_read(void);
uint16_t pc_at_keyboard_send(const uint16_t value);
void pc_at_keyboard_run_macro(const uint16_t macro);

#endif /* BP_ENABLE_PC_AT_KEYBOARD_SUPPORT */

#endif /* !BP_PC_AT_KEYBOARD_H */