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

#ifndef BP_SPI_H
#define BP_SPI_H

#include "configuration.h"

#ifdef BP_ENABLE_SPI_SUPPORT

#include <stdint.h>

/**
 * Start accepting binary I/O commands for SPI operations.
 */
void spi_enter_binary_io(void);

/**
 * Writes the given byte on the SPI bus.
 *
 * @param[in] value the value to write.
 *
 * @return the byte read from the bus after the data write.
 */
//uint8_t spi_write_byte(const uint8_t value);

void spi_start(void);
void spi_start_with_read(void);
void spi_stop(void);
uint16_t spi_read(void);
uint16_t spi_write(const uint16_t value);
void spi_setup_prepare(void);
void spi_setup_execute(void);
void spi_cleanup(void);
void spi_run_macro(const uint16_t macro);
void spi_print_pins_state(void);
void spi_setup(const uint8_t speed);

/**
 * Disable SPI interface.
 */
void spi_disable_interface(void);

void spi_print_settings(void);

#endif /* BP_ENABLE_SPI_SUPPORT */

#endif /* !BP_SPI_H */