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

/*
 * The software i2c routines were written in c from public domain pseudo code
 * found at http://www.esacademy.com/faq/i2c/general/i2cpseud.htm, bearing the
 * description "I2C Driver V1.1 Written by V.Himpe. Released as Public Domain".
 */

#include "bitbang.h"
#include "base.h"

/* Values are in microseconds. */

#define BB_5KHZSPEED_SETTLE 20
#define BB_5KHZSPEED_CLOCK 100
#define BB_50KHZSPEED_SETTLE 2
#define BB_50KHZSPEED_CLOCK 10
#define BB_100KHZSPEED_SETTLE 1
#define BB_100KHZSPEED_CLOCK 5
#define BB_MAXSPEED_SETTLE 0
#define BB_MAXSPEED_CLOCK 0

#define DELAY_PROFILES_MAX_INDEX 3

extern mode_configuration_t mode_configuration;

/**
 * Delay profile structure.
 */
typedef struct {
  /**
   * How many microseconds to wait after setting a non-clock pin state, to make
   * sure the state change occurs properly.
   */
  const uint8_t settle;

  /**
   * How many microseconds to wait after setting the clock pin state.
   */
  const uint8_t clock;
} __attribute__((packed)) bitbang_delays_t;

/* The delays are in microseconds. */

/**
 * Predefined delay profiles, each associated with a predefined bus speed.
 * 
 * @see bp_bitbang_speed_t
 */
static const bitbang_delays_t BITBANG_DELAYS[] = {
    {.settle = BB_5KHZSPEED_SETTLE, .clock = BB_5KHZSPEED_CLOCK},
    {.settle = BB_50KHZSPEED_SETTLE, .clock = BB_50KHZSPEED_CLOCK},
    {.settle = BB_100KHZSPEED_SETTLE, .clock = BB_100KHZSPEED_CLOCK},
    {.settle = BB_MAXSPEED_SETTLE, .clock = BB_MAXSPEED_CLOCK}};

/**
 * Which pin to use for bus reads.
 */
static uint16_t miso_pin;

/**
 * The delay profile to use when operating on the bus, and therefore what is the
 * maximum bus speed.
 */
static const bitbang_delays_t *delay_profile;

void bitbang_setup(unsigned char bitbang_pins, const bp_bitbang_speed_t speed) {
  miso_pin = (bitbang_pins == 3) ? MISO : MOSI;
  delay_profile = &BITBANG_DELAYS[speed > DELAY_PROFILES_MAX_INDEX
                                      ? DELAY_PROFILES_MAX_INDEX
                                      : speed];
}

bool bitbang_i2c_start(void) {
  bool error;

  error = false;

  /* Set both SDA and CLK high. */
  bitbang_set_pins_high(MOSI + CLK, delay_profile->clock);

  /* Check whether lines are still high. */
  if ((BP_CLK == LOW) || (BP_MOSI == LOW)) {
    error = true;
  } else {
    /* Bring SDA low. */
    bitbang_set_pins_low(MOSI, delay_profile->clock);

    /* Bring CLK low too. */
    bitbang_set_pins_low(CLK, delay_profile->clock);

    /* Bring SDA back high. */
    bitbang_set_pins_high(MOSI, delay_profile->clock);
  }

  return error;
}

bool bitbang_i2c_repeated_start(void) { // sends a restart condition on the line
  bool error;

  error = false;

  /* Set both SDA and CLK low. */
  bitbang_set_pins_low(MOSI + CLK, delay_profile->clock);

  /* Set SDA high. */
  bitbang_set_pins_high(MOSI, delay_profile->clock);

  /* Set CLK high too. */
  bitbang_set_pins_high(CLK, delay_profile->clock);

  /* Check whether lines are still high. */
  if ((BP_CLK == LOW) || (BP_MOSI == LOW)) {
    error = true;
  } else {
    /* Bring SDA low. */
    bitbang_set_pins_low(MOSI, delay_profile->clock);

    /* Bring CLK low too. */
    bitbang_set_pins_low(CLK, delay_profile->clock);

    /* Bring SDA back high. */
    bitbang_set_pins_high(MOSI, delay_profile->clock);
  }
  return error;
}

void bitbang_i2c_stop(void) {
  /* Set both SDA and CLK low. */
  bitbang_set_pins_low(MOSI + CLK, delay_profile->clock);

  /* Set CLK high. */
  bitbang_set_pins_high(CLK, delay_profile->clock);

  /* Set SDA high too. */
  bitbang_set_pins_high(MOSI, delay_profile->clock);
}

uint16_t bitbang_read_with_write(const uint16_t value) {
  size_t count;
  uint16_t temporary;
  uint16_t bit_index;
  uint16_t input;

  bit_index = 1 << (mode_configuration.numbits - 1);
  temporary = value;
  input = 0;
  for (count = 0; count < mode_configuration.numbits; count++) {
    bitbang_set_pins(temporary & bit_index, MOSI, delay_profile->settle);
    bitbang_set_pins_high(CLK, delay_profile->clock);
    input = (input << 1) | bitbang_read_pin(MISO);
    temporary <<= 1;
    bitbang_set_pins_low(CLK, delay_profile->clock);
  }

  return input;
}

void bitbang_write_value(const uint16_t value) {
  uint16_t temporary;
  uint16_t bit_index;
  size_t count;

  bit_index = 1 << (mode_configuration.numbits - 1);
  temporary = value;
  for (count = 0; count < mode_configuration.numbits; count++) {
    bitbang_set_pins(temporary & bit_index, MOSI, delay_profile->settle);
    bitbang_set_pins_high(CLK, delay_profile->clock);
    bitbang_set_pins_low(CLK, delay_profile->clock);
    temporary <<= 1;
  }
}

uint16_t bitbang_read_value(void) {
  size_t count;
  uint16_t value;

  /* Setup for input. */
  bitbang_read_pin(MOSI);
  value = 0;

  for (count = 0; count < mode_configuration.numbits; count++) {
    bitbang_set_pins_high(CLK, delay_profile->clock);
    value = (value << 1) | bitbang_read_pin(MOSI);
    bitbang_set_pins_low(CLK, delay_profile->clock);
  }

  return value;
}

bool bitbang_read_bit(void) {
  bool bit_value;

  /* Set the MISO pin as input. */
  bitbang_read_pin(miso_pin);

  /* Set CLK high. */
  bitbang_set_pins_high(CLK, delay_profile->clock);

  /* Read value. */
  bit_value = bitbang_read_pin(miso_pin);

  /* Set CLK low. */
  bitbang_set_pins_low(CLK, delay_profile->clock);

  return bit_value;
}

void bitbang_write_bit(const bool state) {
  /* Set the output pin to the given state. */
  bitbang_set_pins(state, MOSI, delay_profile->settle);

  /* Clock the bit out. */
  bitbang_set_pins_high(CLK, delay_profile->clock);
  bitbang_set_pins_low(CLK, delay_profile->clock);
}

void bitbang_advance_clock_ticks(const uint16_t ticks) {
  size_t tick;

  for (tick = 0; tick < ticks; tick++) {
    bitbang_set_pins_high(CLK, delay_profile->clock);
    bitbang_set_pins_low(CLK, delay_profile->clock);
  }
}

void bitbang_set_mosi(const bool state) {
  bitbang_set_pins(state, MOSI, delay_profile->settle);
}

void bitbang_set_clk(const bool state) {
  bitbang_set_pins(state, CLK, delay_profile->settle);
}

void bitbang_set_cs(const bool state) {
  bitbang_set_pins(state, CS, delay_profile->settle);
}

bool bitbang_read_miso(void) { return bitbang_read_pin(miso_pin); }

void bitbang_set_pins_high(const uint16_t pins, const uint16_t delay) {
  if (mode_configuration.high_impedance == OFF) {
    /* Mark pins' state as HIGH. */
    IOLAT |= pins;

    /* Mark pins' direction as OUTPUT. */
    IODIR &= ~pins;
  } else {
    /* Open collector output HIGH. */
    IODIR |= pins;
  }

  /* Wait. */
  if (delay > 0) {
    bp_delay_us(delay);
  }
}

void bitbang_set_pins_low(const uint16_t pins, const uint16_t delay) {
  /* Mark pins' state as LOW. */
  IOLAT &= ~pins;

  /* Mark pins' direction as OUTPUT. */
  IODIR &= ~pins;

  /* Wait. */
  if (delay > 0) {
    bp_delay_us(delay);
  }
}

void bitbang_set_pins(const bool state, const uint16_t pins_mask,
                      const uint16_t delay) {
  if (state == LOW) {
    IOLAT &= ~pins_mask; // pins to 0
    IODIR &= ~pins_mask; // direction to output
  } else {
    if (mode_configuration.high_impedance == OFF) {
      IOLAT |= pins_mask;  // normal output high
      IODIR &= ~pins_mask; // direction to output
    } else {
      IODIR |= pins_mask; // open collector output high
    }
  }

  /* Wait. */
  if (delay > 0) {
    bp_delay_us(delay);
  }
}

bool bitbang_read_pin(const uint16_t pin_bit) {
  IODIR |= pin_bit;
  Nop();
  Nop();
  Nop();
  return IOPOR & pin_bit;
}
