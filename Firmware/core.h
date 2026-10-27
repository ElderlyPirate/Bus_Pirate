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

#ifndef BP_CORE_H
#define BP_CORE_H

#include "configuration.h"

#include <stdint.h>

/**
 * Protocol definition.
 *
 * This structure provides a series of callbacks that need to be filled in order
 * to implement a new protocol (say, CAN-Bus).
 */
typedef struct {

  /**
   * Start the protocol operations.
   */
  void (*start)(void);

  /**
   * Start the protocol operations and immediately read data on the bus.
   */
  void (*start_with_read)(void);

  /**
   * Stop the protocol operations.
   */
  void (*stop)(void);

  /**
   * Stop the protocol operations if they have been started with an immediate
   * data read on the bus.
   *
   * @todo Find a better name for this callback.
   */
  void (*stop_from_read)(void);

  /**
   * Send the given data to the bus.
   *
   * According to the mode the device is set in, the given data might be
   * 8 or 16 bits wide.  Same thing for the return value, as some protocols
   * only operate on 2 bytes at a time rather than only on just one.
   *
   * Some protocols require a read operation after writing, and in that case
   * the callback will return the data being read from the bus.
   *
   * @param[in] data the data to be sent.
   *
   * @return the data being read from the bus if any has to be read, or
   *         0 otherwise.
   */
  uint16_t (*send)(uint16_t data);

  /**
   * Read some data from the bus.
   *
   * According to the mode the device is set in, the returned data might be
   * 8 or 16 bits wide.
   *
   * @return the data being read from the bus.
   */
  uint16_t (*read)(void);

  /**
   * Pull the clock line high, if one is present.
   */
  void (*clock_high)(void);

  /**
   * Bring the clock line low, if one is present.
   */
  void (*clock_low)(void);

  /**
   * Pull the data line high, if one is present.
   */
  void (*data_high)(void);

  /**
   * Bring the data line low, if one is present.
   */
  void (*data_low)(void);

  /**
   * Read the state of the data line, if one is present.
   *
   * According to the mode the device is set in, the returned data might be
   * 8 or 16 bits wide.
   *
   * @return the state of the data line.
   */
  uint16_t (*data_state)(void);

  /**
   * Pulse the clock line, if one is present.
   */
  void (*clock_pulse)(void);

  /**
   * Advance the clock by the given number of ticks, when the protocol can
   * do it in a single call.
   *
   * Optional: NULL when the clock can only be pulsed one tick at a time
   * through clock_pulse().  The `^` command uses this to batch its ticks.
   */
  void (*clock_pulses)(uint16_t ticks);

  /**
   * Read a bit from the bus.
   *
   * @return the bit read from the bus.
   */
  bool (*read_bit)(void);

  /**
   * Updates the state of the bus and signals that a data I/O operation must
   * be performed.
   *
   * This is used for UART bridges, as data may arrive at any time with and
   * without any user intervention.
   *
   * @return true if there is data to be read, false otherwise.
   */
  bool (*periodic_update)(void);

  /**
   * Run the macro defined by the given numeric identifier.
   *
   * @param[in] macro the numeric identifier for the macro.
   */
  void (*run_macro)(uint16_t macro);

  /**
   * Perform the final steps of the setup stage, shortly before giving power
   * to the pull-ups and start doing I/O.
   */
  void (*setup_prepare)(void);

  /**
   * Perform the setup steps for the protocol to be able to interact with the
   * outside world.
   */
  void (*setup_execute)(void);

  /**
   * Perform the cleanup steps for the protocol before shutting down.
   */
  void (*cleanup)(void);

  /**
   * Print on the serial port the state of the pins the protocol is currently
   * using.
   */
  void (*print_pins_state)(void);

  /**
   * Print on the serial port the current settings in use by the protocol.
   */
  void (*print_settings)(void);

  /**
   * The protocol name, as displayed in the user interface.
   */
  const char name[8];
} bus_pirate_protocol_t;

/**
 * All the available protocols.
 */
typedef enum {

  /**
   * High-impedance mode.
   *
   * All pins are set to input and internally wired to ground, no operations
   * on pins can be performed in this mode.  This is also the default mode
   * the Bus Pirate starts in, and it is mandatory to be available in the
   * enumeration.
   */
  BP_HIZ = 0,

#ifdef BP_ENABLE_1WIRE_SUPPORT
  BP_1WIRE,
#endif /* BP_ENABLE_1WIRE_SUPPORT */

#ifdef BP_ENABLE_UART_SUPPORT
  BP_UART,
#endif /* BP_ENABLE_UART_SUPPORT */

#ifdef BP_ENABLE_I2C_SUPPORT
  BP_I2C,
#endif /* BP_ENABLE_I2C_SUPPORT */

#ifdef BP_ENABLE_SPI_SUPPORT
  BP_SPI,
#endif /* BP_ENABLE_SPI_SUPPORT */

#ifdef BP_ENABLE_RAW_2WIRE_SUPPORT
  BP_RAW2WIRE,
#endif /* BP_ENABLE_RAW_2WIRE_SUPPORT */

#ifdef BP_ENABLE_RAW_3WIRE_SUPPORT
  BP_RAW3WIRE,
#endif /* BP_ENABLE_RAW_3WIRE_SUPPORT */

#ifdef BP_ENABLE_PC_AT_KEYBOARD_SUPPORT
  BP_PCATKBD,
#endif /* BP_ENABLE_PC_AT_KEYBOARD_SUPPORT */

#ifdef BP_ENABLE_HD44780_SUPPORT
  BP_HD44780,
#endif /* BP_ENABLE_HD44780_SUPPORT */

#ifdef BP_ENABLE_PIC_SUPPORT
  BP_PICPROG,
#endif /* BP_ENABLE_PIC_SUPPORT */

#ifdef BP_ENABLE_DIO_SUPPORT
  BP_DIO,
#endif /* BP_ENABLE_DIO_SUPPORT */

  ENABLED_PROTOCOLS_COUNT
} __attribute__((packed)) bus_pirate_available_protocols_t;

/**
 * The numeric base in which to display numbers to the user.
 */
typedef enum {
  /** Display numbers in base-16. */
  HEX = 0,
  /** Display numbers in base-10. */
  DEC,
  /** Display numbers in base-2. */
  BIN,
  /** Display numbers as raw bytes. */
  RAW
} __attribute__((packed)) bus_pirate_display_mode_t;

typedef struct {
  uint8_t *terminal_input;
  uint16_t device_type;
  uint16_t device_revision;
  uint8_t hardware_version;
  uint8_t terminal_speed;
  bus_pirate_display_mode_t display_mode;
  bus_pirate_available_protocols_t bus_mode;
  uint8_t quiet : 1;
  uint8_t overflow : 1;
#ifdef BP_ENABLE_BASIC_SUPPORT
  uint8_t basic : 1;
#endif /* BP_ENABLE_BASIC_SUPPORT */
} bus_pirate_configuration_t;

#endif /* !BP_CORE_H */
