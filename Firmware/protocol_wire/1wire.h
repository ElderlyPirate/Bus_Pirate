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

/**
 * @file 1wire.h
 *
 * @brief 1-Wire protocol handler definition file.
 *
 * From https://en.wikipedia.org/wiki/1-Wire :
 *
 * 1-Wire is a device communications bus system designed by Dallas
 * Semiconductor Corp. that provides low-speed data, signaling, and power over
 * a single conductor.  1-Wire is similar in concept to I2C, but with lower
 * data rates and longer range. It is typically used to communicate with small
 * inexpensive devices such as digital thermometers and weather instruments. A
 * network of 1-Wire devices with an associated master device is called a
 * MicroLAN.  One distinctive feature of the bus is the possibility of using
 * only two wires: data and ground.  To accomplish this, 1-Wire devices include
 * an 800 pF capacitor to store charge, and to power the device during periods
 * when the data line is active.
 *
 */

#ifndef BP_1WIRE_H
#define BP_1WIRE_H

#include "configuration.h"

#ifdef BP_ENABLE_1WIRE_SUPPORT

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enters binary I/O mode for sending 1-Wire commands directly.
 */
void binary_io_enter_1wire_mode(void);

/**
 * @brief Reads some data from the bus.
 *
 * @return the data being read from the bus.
 */
uint16_t onewire_read(void);

/**
 * @brief Sends the given data to the bus.
 *
 * @param[in] data the data to send to the bus.
 *
 * @return the data being read from the bus if any has to be read, or
 *         0 otherwise.
 */
uint16_t onewire_write(const uint16_t data);

/**
 * @brief Reads one bit from the data bus.
 *
 * @return the bit read from the bus.
 *
 * @see bus_pirate_protocol_t.read_bit
 */
bool onewire_read_bit(void);

/**
 * Pulses the clock line, if one is present.
 *
 * @see bus_pirate_protocol_t.clock_pulse
 */
void onewire_clock_pulse(void);

/**
 * Pulls the bus data line LOW.
 *
 * @see bus_pirate_protocol_t.data_low
 */
void onewire_data_low(void);

/**
 * @brief Pulls the bus data line HIGH.
 *
 * @see bus_pirate_protocol_t.data_high
 */
void onewire_data_high(void);

/**
 * @brief Prepare the the software/hardware to initiate the setup phase.
 *
 * @see bus_pirate_protocol_t.setup_prepare
 */
void onewire_setup_prepare(void);

/**
 * @brief Performs the setup phase to use the protocol.
 *
 * @see bus_pirate_protocol_t.setup_execute
 */
void onewire_setup_execute(void);

/**
 * @brief Runs the macro identified by the given marker.
 *
 * @param[in] macro the macro identifier.
 *
 * @see bus_pirate_protocol_t.run_macro
 */
void onewire_run_macro(const uint16_t macro);

/**
 * Prints the current state of hardware pins when in the current protocol mode.
 *
 * @see bus_pirate_protocol_t.print_pins_state
 */
void onewire_pins_state(void);

/**
 * @brief Reads the state of the data line, if one is present.
 *
 * @return the state on the data line.
 *
 * @see bus_pirate_protocol_t.data_state
 */
uint16_t onewire_data_state(void);

/**
 * @brief Resets the bus.
 */
void onewire_reset(void);

#endif /* BP_ENABLE_1WIRE_SUPPORT */

#endif /* !BP_1WIRE_H */
