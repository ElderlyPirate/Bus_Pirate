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

#ifndef BP_SUMP_H
#define BP_SUMP_H

#include "configuration.h"

#ifdef BP_ENABLE_SUMP_SUPPORT

/**
 * Enters into SUMP acquisition mode.
 */
void enter_sump_mode(void);

#endif /* BP_ENABLE_SUMP_SUPPORT */

#endif /* !BP_SUMP_H */