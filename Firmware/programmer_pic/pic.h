/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * Initially written by Chris van Dongen, 2010.
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

#ifndef BP_PIC_H
#define BP_PIC_H

#include "configuration.h"

#ifdef BP_ENABLE_PIC_SUPPORT

#include <stdint.h>

/**
 * @brief Starts to accept mode commands using the binary I/O protocol rather
 * than from the user CLI.
 */
void binary_io_enter_pic_mode(void);

/**
 * @brief Prepare the the software/hardware to initiate the setup phase.
 *
 * This function will eventually prompt the user to obtain operational
 * parameters or other values that affect the mode operation.
 */
void pic_setup_prepare(void);

/**
 * @brief Set up the the software/hardware to be ready to begin.
 *
 * This function will use the information gathered in `pic_setup_prepare` to set
 * the hardware up for the mode to actually do something.
 */
void pic_setup_execute(void);

/**
 * @brief Starts the mode operations.
 */
void pic_start(void);

/**
 * @brief Stops the mode operations.
 */
void pic_stop(void);

/**
 * @brief Reads a value from the bus.
 *
 * The length of the value (whether it fits in 8 or 16 bits) is usually
 * controlled by the `int16` field in the mode configuration structure.  That
 * said, each protocol may read less than 8 bits at a time if needed, so even
 * though there is a length specifier that must be taken as the maximum number
 * of bits that can be read at a given time by a single call to this function.
 *
 * @return the data being read from the bus.
 */
uint16_t pic_read(void);

/**
 * @brief Writes a value to the bus.
 *
 * The length of the return value (whether it fits in 8 or 16 bits) is usually
 * controlled by the `int16` field in the mode configuration structure.  That
 * said, each protocol may read less than 8 bits at a time if needed, so even
 * though there is a length specifier that must be taken as the maximum number
 * of bits that can be read at a given time by a single call to this function.
 *
 * @param[in] data the data to send to the bus.
 *
 * @return the data being read from the bus if any has to be read, or 0
 * otherwise.
 */
uint16_t pic_write(const uint16_t value);

/**
 * @brief Restores the hardware to a neutral state.
 */
void pic_cleanup(void);

/**
 * @brief Executes the mode-specific macro identified by the given number.
 *
 * If an invalid or unknown macro identifier is passed, no errors are given but
 * no action should ever take place either.
 *
 * @param[in] macro the identifier for the macro to execute.
 */
void pic_run_macro(const uint16_t macro);

/**
 * @brief Prints the current state of the I/O pins used by the mode to the
 * user-facing serial port.
 */
void pic_print_pins_state(void);

#endif /* BP_ENABLE_PIC_SUPPORT */

#endif /* !BP_PIC_H */