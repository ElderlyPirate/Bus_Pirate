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
 * Call bitbang_setup() before any profile-dependent operation, including pin
 * setters, and again when changing pin count or speed. There is no implicit
 * default profile: setup is state-only and does not drive any pins. Callers
 * also own valid numbits (1..16) before value transfers. The terminal parser
 * validates widths; binary raw entry fixes the width at eight bits.
 */

/**
 * Waiting-I2C contract (all calls with wait_for_clock_release=true and the
 * _wait / i2c_write_bit signals): high_impedance must be ON, with external
 * pull-ups. Releasing SCL is intentional; push-pull drive is not supported.
 * On failure both lines are released and the sticky i2c_error is set. Clear
 * that status before a new operation, not between signals of one operation.
 * Raw/non-waiting primitives neither wait for SCL nor consume that status.
 * See DS39897C 10.1/10.1.1 for input/released versus driven pin behavior.
 */

/**
 * Timing profiles for bit-banging operations. Enum names are legacy nominal
 * rates, not frequency guarantees. At FCY=16 MHz the raw tick path is roughly
 * 6/60/150 kHz for the first three profiles; data setup, sampling, open-drain
 * rise time and protocol/transport gaps reduce the effective transfer rate.
 */
typedef enum {
  /**
   * Slow profile (raw ticks approximately 6 kHz).
   */
  BITBANG_SPEED_5KHZ = 0,

  /**
   * Medium profile (raw ticks approximately 60 kHz).
   */
  BITBANG_SPEED_50KHZ,

  /**
   * Fast profile (raw ticks approximately 150 kHz).
   */
  BITBANG_SPEED_100KHZ,

  /**
   * No deliberate phase delay; the maximum rate depends on the primitive.
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
 * Writes MOSI and samples the fixed MISO pin, used by 3-wire protocols.
 * This primitive requires three-wire setup; it does not use the selected
 * single-bit input pin.
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
 * Writes a value on MOSI, used by 2-wire protocols.
 *
 * @warning The amount of bits to write is set in the numbits field in the mode
 * configuration structure (up to 16).
 *
 * @param[in] value the value to write on the bus.
 * @param[in] wait_for_clock_release whether to wait for a slave to release the
 *            clock line before ending each clock pulse, which I2C devices may
 *            do to stretch it; protocols that do not share the clock line pass
 *            false.
 */
void bitbang_write_value(const uint16_t value,
                         const bool wait_for_clock_release);

/**
 * Reads a value from the fixed MOSI/SDA pin, used by 2-wire protocols.
 *
 * @warning The output size is dependent on the numbits field in the mode
 * configuration structure, with said number being the amount of bits to read
 * from the bus (up to 16 at a time).
 *
 * @param[in] wait_for_clock_release whether to wait for a slave to release the
 *            clock line before sampling each bit, which I2C devices may do to
 *            stretch it; protocols that do not share the clock line pass
 *            false.
 *
 * @return the value read from the bus.
 */
uint16_t bitbang_read_value(const bool wait_for_clock_release);

/**
 * Reads a single bit from the setup-selected input (2 pins: MOSI; 3: MISO).
 *
 * @param[in] wait_for_clock_release whether to wait for a slave to release the
 *            clock line before sampling, which I2C devices may do to stretch
 *            it; protocols that do not share the clock line pass false.
 *
 * @return true if the MISO pin is HIGH, false otherwise.
 */
bool bitbang_read_bit(const bool wait_for_clock_release);

/**
 * Saturating number of clock-stretch waits that have timed out since reset. A
 * non-zero count means some operation gave up waiting for a slave to
 * release the clock line, so its result may be invalid; the count lets
 * callers and diagnostics distinguish that from a valid transfer. Setup and
 * clearing the sticky operation status do not clear this diagnostic counter.
 */
uint16_t bitbang_clock_stretch_timeouts(void);

/* Sticky status for an I2C operation; raw/non-waiting operations ignore it. */
void bitbang_i2c_clear_error(void);
bool bitbang_i2c_error(void);
/* Waiting I2C signals return true on success; legacy raw signals below do not wait. */
bool bitbang_i2c_start_wait(bp_bitbang_i2c_start_type_t type);
bool bitbang_i2c_stop_wait(void);
bool bitbang_i2c_write_bit(bool state);

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
 * Sets the pins HIGH (driven output, or released input in high-impedance
 * mode), then waits using the short-delay helper.
 * Delay must be within the helper's 0..16384 microsecond input range.
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
 * LOW (short-delay helper input range 0..16384 microseconds).
 */
void bitbang_set_pins_low(const uint16_t pins_bitmask, const uint16_t delay);

/**
 * Sets the selected pins to the given state (HIGH is released when
 * high_impedance is ON), then waits using the short-delay helper.
 *
 * @param[in] state the state to set the selected pins to.
 * @param[in] pins the bitmask of the pins to set.
 * @param[in] delay the amount of microseconds to wait after setting the pins
 * to the requested state (helper input range 0..16384 microseconds).
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
 * This legacy raw primitive never waits for SCL; its return convention is
 * the opposite of bitbang_i2c_start_wait(), which returns true on success.
 *
 * @see bp_bitbang_i2c_start_type_t
 */
bool bitbang_i2c_start(bp_bitbang_i2c_start_type_t type);

/**
 * Sends a legacy raw STOP without waiting for SCL or checking sticky status.
 */
void bitbang_i2c_stop(void);

#endif /* !BP_BITBANG_H */
