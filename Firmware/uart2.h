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

#ifndef BP_UART2_H
#define BP_UART2_H

#include "configuration.h"

#if defined(BP_ENABLE_UART_SUPPORT)

#include <stdbool.h>
#include <stdint.h>

/**
 * @def UART2_POLARITY_INVERT_YES
 *
 * Flag indicating that the UART2 idle states must be the opposite of what RS232
 * mandates.
 */
#define UART2_POLARITY_INVERT_YES ON

/**
 * @def UART2_POLARITY_INVERT_NO
 *
 * Flag indicating that the UART2 idle states must follow the RS232
 * specifications.
 */
#define UART2_POLARITY_INVERT_NO OFF

/**
 * @def UART2_9_N
 *
 * Flag indicating that the port will send 9 bits per character, with no parity
 * bit.
 */
#define UART2_9_N 0b11

/**
 * @def UART2_8_N
 *
 * Flag indicating that the port will send 8 bits per character, using
 * odd-numbered bits for calculating the parity bit.
 */
#define UART2_8_O 0b10

/**
 * @def UART2_8_N
 *
 * Flag indicating that the port will send 8 bits per character, using
 * even-numbered bits for calculating the parity bit.
 */
#define UART2_8_E 0b01

/**
 * @def UART2_8_N
 *
 * Flag indicating that the port will send 8 bits per character, with no parity
 * bit.
 */
#define UART2_8_N 0b00

/**
 * @def UART2_1_S
 *
 * Flag indicating that the port needs to send one stop bit after every
 * character.
 */
#define UART2_1_S OFF

/**
 * @def UART2_2_S
 *
 * Flag indicating that the port needs to send two stop bits after every
 * character.
 */
#define UART2_2_S ON

/**
 * Sets up UART #2.
 *
 * @param[in] baud_rate_generator_prescaler the prescaler value for UART #2's
 * baud rate generator.
 * @param[in] open_drain_output ON if the output should be open-drain, OFF if it
 * should be active driver.
 * @param[in] invert_polarity UART2_POLARITY_INVERT_YES if RX and TX idle states
 * are going to be inverted from what the RS232 standard mandates,
 * UART2_POLARITY_INVERT_NO if the port must follow RS232 specifications.
 * @param[in] databits_and_parity how many bits are going to be sent/received
 * per character and how parity bits should be handled - must be one of
 * UART2_9_N, UART2_8_O, UART2_8_E, or UART2_8_N.
 * @param[in] stop_bits how many stop bits need to be sent after each character,
 * UART2_1_S for one stop bit, UART2_2_S for two stop bits.
 *
 * @see UART2_POLARITY_INVERT_YES
 * @see UART2_POLARITY_INVERT_NO
 * @see UART2_9_N
 * @see UART2_8_O
 * @see UART2_8_E
 * @see UART2_8_N
 * @see UART2_1_S
 * @see UART2_2_S
 */
void uart2_setup(const uint16_t baud_rate_generator_prescaler,
                 const bool open_drain_output, const bool invert_polarity,
                 const uint8_t databits_and_parity, const bool stop_bits);

/**
 * Enables the UART #2 port for normal RX/TX operations.
 */
void uart2_enable(void);

/**
 * Disables the UART #2 port, stopping any ongoing RX/TX operations.
 */
void uart2_disable(void);

/**
 * Sends the given character on UART #2.
 *
 * @warning this function will bring the CPU into a busy loop until UART #2's TX
 * buffer empties.
 *
 * @param[in] character the character to send.
 */
void uart2_tx(const uint8_t character);

/**
 * Checks if the UART #2 RX buffer is not empty.
 *
 * @return true if there are characters to read, false otherwise.
 */
bool uart2_rx_ready(void);

/**
 * Reads a character from UART #2.
 *
 * @warning this function will bring the CPU into a busy loop until UART #2's RX
 * buffer fills up.
 *
 * @return the character being read from UART #2.
 */
uint8_t uart2_rx(void);

#endif /* BP_ENABLE_UART_SUPPORT */

#endif /* !BP_UART2_H */