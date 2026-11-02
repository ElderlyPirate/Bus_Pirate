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
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include "dio.h"

#ifdef BP_ENABLE_DIO_SUPPORT

#include "base.h"
#include "binary_io.h"

extern mode_configuration_t mode_configuration;

/**
 * Bit 7 (0x80) indicates whether it is to set the pin state or the pin direction.
 */
#define DIO_PIN_SET_STATE_FLAG_MASK 0b0000000010000000

unsigned int dio_read(void) {
	return bitbang_pin_state_get();
}

unsigned int dio_write(unsigned int value) {
    return (value & DIO_PIN_SET_STATE_FLAG_MASK) ? bitbang_pin_state_set(value) :
        bitbang_pin_direction_set(value);
}

#endif /* BP_ENABLE_DIO_SUPPORT */

void dio_setup_prepare(void) {
	mode_configuration.write_with_read = NO;
}
