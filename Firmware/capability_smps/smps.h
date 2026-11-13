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

#ifndef BP_SMPS_H
#define BP_SMPS_H

#include "configuration.h"

#ifdef BP_ENABLE_SMPS_SUPPORT

#ifdef BUSPIRATEV4

/**
 * Enables operation of a connected switched mode power supply board connected
 * to the Bus Pirate v4.
 *
 * @param[in] requested_voltage The voltage that should be output by the
 * external board.
 */
void smps_start(unsigned int requested_voltage);

/**
 * Stops operation of the connected switched mode power supply board.
 */
void smps_stop(void);

/**
 * Outputs the last output voltage reading to the serial port as a big-endian
 * 16-bits integer.
 */
void smps_adc(void);

#endif /* BUSPIRATEV4 */

#endif /* BP_ENABLE_SMPS_SUPPORT */

#endif /* !BP_SMPS_H */