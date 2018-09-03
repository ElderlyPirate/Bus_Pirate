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

#ifndef BP_BITBANG_H
#define BP_BITBANG_H

#include <stdbool.h>
#include <stdint.h>

/**
 * The possible bus speeds for bit-banging operations.
 */
typedef enum {
  /**
   * Selects a bus speed of 5kHz.
   */
  BITBANG_SPEED_5KHZ = 0,

  /**
   * Selects a bus speed of 50kHz.
   */
  BITBANG_SPEED_50KHZ,

  /**
   * Selects a bus speed of 100kHz.
   */
  BITBANG_SPEED_100KHZ,

  /**
   * Selects the maximum bus speed, which is around 400kHz.
   */
  BITBANG_SPEED_MAXIMUM
} bp_bitbang_speed_t;

/**
 * The possible I2C start signalling types.
 */
typedef enum {
  /**
   * Signal normal I2C start.
   */
  BITBANG_I2C_START_ONE_SHOT = 0,

  /**
   * Signal repeated I2C start.
   */
  BITBANG_I2C_RESTART
} bp_bitbang_i2c_start_type_t;

/**
 * Sets up the bit-bang module.
 *
 * This function sets the bit-banging module operation type, whether to use
 * three pins (and then operate in a SPI-like fashion), or two pins (operating
 * in an I2C-like fashion instead).
 *
 * @param[in] pins the number of pins to use, 2 or 3.
 * @param[in] speed the bit-banging bus speed to use.
 *
 * @see BITBANG_SPEED_5KHZ
 * @see BITBANG_SPEED_50KHZ
 * @see BITBANG_SPEED_100KHZ
 * @see BITBANG_SPEED_MAXIMUM
 */
void bitbang_setup(unsigned char pins, const bp_bitbang_speed_t speed);

/**
 * Writes a value to the bus then reads a value back, used by 3-wire protocols.
 *
 * @warning The amount of bits to read and write is set in the numbits field in
 * the mode configuration structure (up to 16).
 *
 * @param[in] value the value to write on the bus.
 *
 * @return the value read from the bus.
 */
uint16_t bitbang_read_with_write(const uint16_t value);

/**
 * Writes a value to the bus, used by 2-wire protocols.
 *
 * @warning The amount of bits to write is set in the numbits field in the mode
configuration structure (up to 16).
 *
 * @param[in] value the value to write on the bus.
 */
void bitbang_write_value(const uint16_t value);

/**
 * Reads a value from the bus, used by 2-wire protocols.
 *
 * @warning The output size is dependent on the numbits field in the mode
 * configuration structure, with said number being the amount of bits to read
 * from the bus (up to 16 at a time).
 *
 * @return the value read from the bus.
 */
uint16_t bitbang_read_value(void);

/**
 * Reads a single bit from the MISO pin.
 *
 * @return true if the MISO pin is HIGH, false otherwise.
 */
bool bitbang_read_bit(void);

/**
 * Writes the given bit to the bus via the MOSI pin.
 *
 * @param[in] state the bit to write, either HIGH or LOW.
 */
void bitbang_write_bit(const bool state);

/**
 * Pulses the CLK line for the given amount of ticks, leaving the MOSI pin
 * alone.
 *
 * @param[in] ticks the number of pulses to send.
 */
void bitbang_advance_clock_ticks(const uint16_t ticks);

/**
 * Sets the state of the MOSI pin.
 *
 * @param[in] state the state to set.
 */
void bitbang_set_mosi(const bool state);

/**
 * Sets the state of the CLK pin.
 *
 * @param[in] state the state to set.
 */
void bitbang_set_clk(const bool state);

/**
 * Sets the state of the CS pin.
 *
 * @param[in] state the state to set.
 */
void bitbang_set_cs(const bool state);

/**
 * Reads the state of the MISO pin.
 *
 * @return HIGH or LOW, depending on the state of the MISO pin.
 */
bool bitbang_read_miso(void);

/**
 * Sets the pins indicated by the given bitmask to HIGH state and OUTPUT
 * direction, then spins for the given amount of microseconds.
 *
 * @param[in] pins the bitmask of the pins to set HIGH.
 * @param[in] delay the amount of microseconds to wait after setting the pins
 * HIGH.
 */
void bitbang_set_pins_high(const uint16_t pins_bitmask, const uint16_t delay);

/**
 * Sets the pins indicated by the given bitmask to LOW state and OUTPUT
 * direction, then spins for the given amount of microseconds.
 *
 * @param[in] pins the bitmask of the pins to set LOW.
 * @param[in] delay the amount of microseconds to wait after setting the pins
 * LOW.
 */
void bitbang_set_pins_low(const uint16_t pins_bitmask, const uint16_t delay);

/**
 * Sets the pins indicated by the given bitmask to the given state and OUTPUT
 * direction, then spins for the given amount of microseconds.
 *
 * @param[in] state the state to set the selected pins to.
 * @param[in] pins the bitmask of the pins to set.
 * @param[in] delay the amount of microseconds to wait after setting the pins
 * LOW.
 */
void bitbang_set_pins(const bool state, const uint16_t pins_mask,
                      const uint16_t delay);

/**
 * Reads the state from the given pin.
 *
 * The pin number should be passed in the form (1 &lt;&lt; pin).
 *
 * @warning passing a pin number with more than a single bit set to one will
 * trigger an undefined behaviour.
 *
 * @param[in] pin_bit the pin to read from.
 *
 * @return HIGH or LOW, depending on the pin state.
 */
bool bitbang_read_pin(const uint16_t pin_bit);

/**
 * Sends the appropriate signals to indicate an I2C transmission start frame.
 *
 * Sending BITBANG_I2C_RESTART effectively resets the I2C transmission state
 * without releasing the I2C bus.  This is used whenever consecutive atomic
 * operations are needed, and the normal start code would not only reset the
 * protocol state but also the connection state.
 *
 * @param[in] type the start signal type.
 *
 * @return true if an error occurred, false otherwise.
 *
 * @see bp_bitbang_i2c_start_type_t
 */
bool bitbang_i2c_start(bp_bitbang_i2c_start_type_t type);

/**
 * Sends the appropriate signals to indicate an I2C transmission stop frame.
 */
void bitbang_i2c_stop(void);

#endif /* !BP_BITBANG_H */
