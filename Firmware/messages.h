/*
 * This file is part of the Bus Pirate project (http://code.google.com/p/the-bus-pirate/).
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

#ifndef BP_MESSAGES_H
#define BP_MESSAGES_H

#include <stddef.h>
#include <stdbool.h>

#include "configuration.h"

#ifdef BUSPIRATEV3
#include "messages_v3.h"
#elif defined(BUSPIRATEV4)
#include "messages_v4.h"
#else
#error "No messages file selected!"
#endif /* BUSPIRATEV3 || BUSPIRATEV4 */

/**
 * Prints the help screen information to the serial port.
 */
void print_help(void);

/**
 * Prints a given byte array range from the packed string buffer to the serial
 * port.
 * 
 * @param[in] str pointer to the string.
 * @param[in] length how many bytes to print to the serial port.
 */
void bp_message_write_buffer(unsigned long strptr, size_t length);

/**
 * Prints a given byte array range from the packed string buffer to the serial
 * port, appending a CRLF pair at the end.
 * 
 * @param[in] str pointer to the string.
 * @param[in] length how many bytes to print to the serial port.
 */
void bp_message_write_line(unsigned long strptr, size_t length);

/**
 * Prompts the user to agree or not on a particular question.
 * 
 * @return true if the user pressed either 'y' or 'Y', false otherwise.
 */
bool agree(void);

#endif /* !BP_MESSAGES_H */