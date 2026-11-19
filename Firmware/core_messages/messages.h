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

#ifndef BP_MESSAGES_H
#define BP_MESSAGES_H

#include <stdbool.h>
#include <stddef.h>

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
 * Prints one NUL-terminated .pasciz message from program memory.
 * The address is an even PIC24 table address, not a RAM byte pointer. Three
 * payload bytes occupy each 24-bit program word; the reader advances by two
 * table-address units per word (DS70000157G, TBLRDL/TBLRDH). TBLPAG is restored.
 *
 * There is no length parameter: callers must supply a valid, terminated
 * message. Generated catalogs enforce non-NUL payload bytes before the
 * assembler appends the terminator; this is not an arbitrary flash-range API.
 *
 * @param[in] strptr packed-message table address, normally __builtin_tbladdress.
 */
void bp_message_write_buffer(unsigned long strptr);

/**
 * Prints a valid packed message under the same preconditions as the buffer
 * writer, then appends exactly one CRLF pair.
 *
 * @param[in] strptr packed-message table address.
 */
void bp_message_write_line(unsigned long strptr);

/**
 * Prompts the user to agree or not on a particular question.
 *
 * @return true if the user pressed either 'y' or 'Y', false otherwise.
 */
bool agree(void);

#endif /* !BP_MESSAGES_H */
