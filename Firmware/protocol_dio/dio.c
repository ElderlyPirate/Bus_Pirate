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
#include "protocol_binary/binary_io.h"

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

void dio_setup_prepare(void) {
	mode_configuration.write_with_read = NO;
}

void dio_cleanup(void) {
    uint16_t park_mask = AUX | MOSI | CLK | MISO | CS;
#ifdef BUSPIRATEV4
    /* k/K can leave both previous AUX targets driven, regardless of selection. */
    park_mask |= AUX1 | AUX2;
#endif /* BUSPIRATEV4 */
    /* Release the header before removing bias or clearing output latches. */
    IODIR |= park_mask;
    bp_disable_pullup();
#ifdef BUSPIRATEV4
    bp_disable_3v3_pullup();
#endif /* BUSPIRATEV4 */
    IOLAT &= ~park_mask;
    mode_configuration.numbits = 8;
    mode_configuration.int16 = NO;
}

#endif /* BP_ENABLE_DIO_SUPPORT */
