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

#ifndef BP_UART_H
#define BP_UART_H

#include "configuration.h"

#ifdef BP_ENABLE_UART_SUPPORT

#include <stdbool.h>
#include <stdint.h>

void uartProcess(void);
void binUART(void);

/**
 * Reads a value from the UART.
 *
 * @return a character read from the UART.
 */
uint16_t uart_read(void);

/**
 * Writes the given value to the UART.
 *
 * @param[in] value the value to write.
 *
 * @return 0 as the function always succeeds.
 */
uint16_t uart_write(const uint16_t value);

/**
 * Cleans up the state of the UART module once its operations are done.
 */
void uart_cleanup(void);

/**
 * Starts UART operations.
 */
void uart_start(void);

/**
 * Stops UART operations.
 */
void uart_stop(void);

/**
 * Prints the current state of hardware pins when in the current protocol mode.
 *
 * @see bus_pirate_protocol_t.print_pins_state
 */
void uart_pins_state(void);

/**
 * Runs the macro identified by the given marker.
 *
 * @param[in] macro the macro identifier.
 *
 * @see bus_pirate_protocol_t.run_macro
 */
void uart_run_macro(const uint16_t macro);

void uart_setup_prepare(void);
void uart_setup_execute(void);

bool uart_periodic_callback(void);
void UARTsettings(void);

#endif /* BP_ENABLE_UART_SUPPORT */

#endif /* !BP_UART_H */