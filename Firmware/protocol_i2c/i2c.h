/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has waived all copyright and
 * related or neighboring rights to Bus Pirate.  This work is published from
 * United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef BP_I2C_H
#define BP_I2C_H

#include "configuration.h"

#ifdef BP_ENABLE_I2C_SUPPORT

#include <stdint.h>

/**
 * Starts I2C operations.
 */
void i2c_start(void);

/**
 * Stops I2C operations.
 */
void i2c_stop(void);

/**
 * Cleans up after I2C operations.
 */
void i2c_cleanup(void);

/**
 * Prints the I2C mode settings to the serial port.
 */
void i2c_print_settings(void);

/**
 * Prints the I2C pins state to the serial port.
 */
void i2c_pins_state(void);

uint16_t i2c_read(void);
uint16_t i2c_write(const uint16_t value);
void i2c_setup_prepare(void);
void i2c_setup_execute(void);
void i2c_macro(const uint16_t macro);

void binary_io_enter_i2c_mode(void);

#endif /* BP_ENABLE_I2C_SUPPORT */

#endif /* !BP_I2C_H */