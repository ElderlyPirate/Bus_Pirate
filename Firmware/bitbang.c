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

/*
 * The software i2c routines were written in c from public domain pseudo code
 * found at http://www.esacademy.com/faq/i2c/general/i2cpseud.htm, bearing the
 * description "I2C Driver V1.1 Written by V.Himpe. Released as Public Domain".
 */

#include "bitbang.h"
#include "base.h"

/* Values are in microseconds. */

#ifdef BP_USE_HARDWARE_DELAY_TIMER

#define BB_5KHZSPEED_SETTLE 20
#define BB_5KHZSPEED_CLOCK 80
#define BB_50KHZSPEED_SETTLE 1
#define BB_50KHZSPEED_CLOCK 8
#define BB_100KHZSPEED_SETTLE 1
#define BB_100KHZSPEED_CLOCK 3

#else

#define BB_5KHZSPEED_SETTLE 19
#define BB_5KHZSPEED_CLOCK 80
#define BB_50KHZSPEED_SETTLE 1
#define BB_50KHZSPEED_CLOCK 9
#define BB_100KHZSPEED_SETTLE 1
#define BB_100KHZSPEED_CLOCK 4

#endif /* BP_USE_HARDWARE_DELAY_TIMER */

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

/**
 * How many clock waits have timed out since reset.  A timed-out wait means
 * the bus was held low longer than BB_CLK_STRETCH_TIMEOUT: whatever the
 * operation that waited returned is not trustworthy, and the count lets
 * callers and diagnostics tell a real acknowledge from a guessed one (the
 * timeout used to be silent).  Saturating.
 */
static uint16_t bitbang_clk_stretch_timeouts = 0;
static bool i2c_error = false;

uint16_t bitbang_clock_stretch_timeouts(void) {
  return bitbang_clk_stretch_timeouts;
}

static bool bitbang_wait_for_clock_release(void);

void bitbang_i2c_clear_error(void) { i2c_error = false; }

bool bitbang_i2c_error(void) { return i2c_error; }

static void bitbang_i2c_abort(void) {
  i2c_error = true;
  IODIR |= MOSI | CLK;
}

/* I2C high time starts on the wire's rise, not when we release the pin. */
static bool bitbang_i2c_clock_high(void) {
  if (i2c_error) {
    return false;
  }
  bitbang_set_pins_high(CLK, 0);
  if (!bitbang_wait_for_clock_release()) {
    bitbang_i2c_abort();
    return false;
  }
  if (delay_profile->clock) {
    delay_short_inline(delay_profile->clock);
  }
  return true;
}

bool bitbang_i2c_start_wait(bp_bitbang_i2c_start_type_t type) {
  if (i2c_error) {
    return false;
  }
  if (type == BITBANG_I2C_RESTART) {
    bitbang_set_pins_low(CLK, delay_profile->clock);
  }
  /* Release SDA while SCL is still low on a repeated START. */
  bitbang_set_pins_high(MOSI, delay_profile->clock);
  if (!bitbang_i2c_clock_high()) {
    return false;
  }
  if (!bitbang_read_pin(MOSI)) {
    bitbang_i2c_abort();
    return false;
  }
  bitbang_set_pins_low(MOSI, delay_profile->clock);
  bitbang_set_pins_low(CLK, delay_profile->clock);
  bitbang_set_pins_high(MOSI, delay_profile->clock);
  return true;
}

bool bitbang_i2c_stop_wait(void) {
  if (i2c_error) {
    return false;
  }
  bitbang_set_pins_low(CLK, delay_profile->clock);
  bitbang_set_pins_low(MOSI, delay_profile->clock);
  if (!bitbang_i2c_clock_high()) {
    return false;
  }
  /* Never signal STOP by moving SDA before the actual SCL high phase. */
  bitbang_set_pins_high(MOSI, delay_profile->clock);
  if (!bitbang_read_pin(MOSI)) {
    bitbang_i2c_abort();
    return false;
  }
  return true;
}

bool bitbang_i2c_write_bit(bool state) {
  if (i2c_error) {
    return false;
  }
  bitbang_set_pins(state, MOSI, delay_profile->settle);
  if (!bitbang_i2c_clock_high()) {
    return false;
  }
  bitbang_set_pins_low(CLK, delay_profile->clock);
  return true;
}

void bitbang_setup(unsigned char bitbang_pins, const bp_bitbang_speed_t speed) {
  miso_pin = (bitbang_pins == 3) ? MISO : MOSI;
  delay_profile = &BITBANG_DELAYS[speed > DELAY_PROFILES_MAX_INDEX
                                      ? DELAY_PROFILES_MAX_INDEX
                                      : speed];
}

bool bitbang_i2c_start(bp_bitbang_i2c_start_type_t type) {

  /* Set both SDA and CLK high. */
  bitbang_set_pins_high(MOSI + CLK, delay_profile->clock);

  if (type == BITBANG_I2C_RESTART) {
    /* Set SDA high. */
    bitbang_set_pins_high(MOSI, delay_profile->clock);

    /* Set CLK high too. */
    bitbang_set_pins_high(CLK, delay_profile->clock);
  }

  /* Check whether lines are still high. */
  if ((BP_CLK == LOW) || (BP_MOSI == LOW)) {
    return true;
  }

  /* Bring SDA low. */
  bitbang_set_pins_low(MOSI, delay_profile->clock);

  /* Bring CLK low too. */
  bitbang_set_pins_low(CLK, delay_profile->clock);

  /* Bring SDA back high. */
  bitbang_set_pins_high(MOSI, delay_profile->clock);

  return false;
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
  const uint16_t settle = delay_profile->settle;
  const uint16_t clock = delay_profile->clock;
  const uint16_t numbits = mode_configuration.numbits;
  const uint16_t bit_index = 1 << (numbits - 1);
  uint16_t temporary = value;
  uint16_t input = 0;
  size_t count;

  /*
   * This is a per-bit hot path for the 3-wire raw protocols, so the
   * per-bit operations are written out directly, in the style of
   * bitbang_advance_clock_ticks():
   *
   * - the high-impedance branch is hoisted (it cannot change mid-value),
   * - the profile delays are loaded once instead of once per bit,
   * - the pin writes compile to single-cycle BSET/BCLR on LATD/TRISD,
   * - the per-bit bitbang_set_pins*() and bitbang_read_pin() calls are
   *   replaced by their bodies (direction setup, three settling Nops,
   *   port read),
   * - delay_short() is called directly.
   *
   * The delays themselves, and therefore the selected speed profile, are
   * unchanged.
   */
  if (mode_configuration.high_impedance == OFF) {
    for (count = 0; count < numbits; count++) {
      /* Output the next data bit (driven). */
      if (temporary & bit_index) {
        IOLAT |= MOSI;
        IODIR &= ~MOSI;
      } else {
        IOLAT &= ~MOSI;
        IODIR &= ~MOSI;
      }
      if (settle > 0) {
        delay_short_inline(settle);
      }

      /* Clock high. */
      IOLAT |= CLK;
      IODIR &= ~CLK;
      if (clock > 0) {
        delay_short_inline(clock);
      }

      /* Sample the input. */
      IODIR |= MISO;
      Nop();
      Nop();
      Nop();
      input = (input << 1) | ((IOPOR & MISO) ? 1 : 0);
      temporary <<= 1;

      /* Clock low. */
      IOLAT &= ~CLK;
      IODIR &= ~CLK;
      if (clock > 0) {
        delay_short_inline(clock);
      }
    }
  } else {
    for (count = 0; count < numbits; count++) {
      /* Output the next data bit (open collector: high is released). */
      if (temporary & bit_index) {
        IODIR |= MOSI;
      } else {
        IOLAT &= ~MOSI;
        IODIR &= ~MOSI;
      }
      if (settle > 0) {
        delay_short_inline(settle);
      }

      /* Clock high (open collector: released to the pull-up). */
      IODIR |= CLK;
      if (clock > 0) {
        delay_short_inline(clock);
      }

      /* Sample the input. */
      IODIR |= MISO;
      Nop();
      Nop();
      Nop();
      input = (input << 1) | ((IOPOR & MISO) ? 1 : 0);
      temporary <<= 1;

      /* Clock low. */
      IOLAT &= ~CLK;
      IODIR &= ~CLK;
      if (clock > 0) {
        delay_short_inline(clock);
      }
    }
  }

  return input;
}

void bitbang_write_value(const uint16_t value,
                         const bool wait_for_clock_release) {
  const uint16_t settle = delay_profile->settle;
  const uint16_t clock = delay_profile->clock;
  const uint16_t numbits = mode_configuration.numbits;
  const uint16_t bit_index = 1 << (numbits - 1);
  uint16_t temporary = value;
  size_t count;
  if (wait_for_clock_release && i2c_error) {
    return;
  }

  /*
   * This is a per-bit hot path for the 2-wire raw protocols and the
   * software I2C, so the per-bit operations are written out directly, in
   * the style of bitbang_advance_clock_ticks():
   *
   * - the high-impedance branch is hoisted (it cannot change mid-value),
   * - the profile delays are loaded once instead of once per bit,
   * - the pin writes compile to single-cycle BSET/BCLR on LATD/TRISD,
   * - the per-bit bitbang_set_pins*() calls are replaced by their
   *   bodies,
   * - delay_short() is called directly.
   *
   * The delays themselves, and therefore the selected speed profile, are
   * unchanged.
   */
  if (mode_configuration.high_impedance == OFF) {
    for (count = 0; count < numbits; count++) {
      /* Output the next data bit (driven). */
      if (temporary & bit_index) {
        IOLAT |= MOSI;
        IODIR &= ~MOSI;
      } else {
        IOLAT &= ~MOSI;
        IODIR &= ~MOSI;
      }
      if (settle > 0) {
        delay_short_inline(settle);
      }

      if (wait_for_clock_release) {
        if (!bitbang_i2c_clock_high()) {
          return;
        }
      } else {
        IOLAT |= CLK;
        IODIR &= ~CLK;
        if (clock > 0) {
          delay_short_inline(clock);
        }
      }

      /* Clock low. */
      IOLAT &= ~CLK;
      IODIR &= ~CLK;
      if (clock > 0) {
        delay_short_inline(clock);
      }

      temporary <<= 1;
    }
  } else {
    for (count = 0; count < numbits; count++) {
      /* Output the next data bit (open collector: high is released). */
      if (temporary & bit_index) {
        IODIR |= MOSI;
      } else {
        IOLAT &= ~MOSI;
        IODIR &= ~MOSI;
      }
      if (settle > 0) {
        delay_short_inline(settle);
      }

      if (wait_for_clock_release) {
        if (!bitbang_i2c_clock_high()) {
          return;
        }
      } else {
        IODIR |= CLK;
        if (clock > 0) {
          delay_short_inline(clock);
        }
      }

      /* Clock low. */
      IOLAT &= ~CLK;
      IODIR &= ~CLK;
      if (clock > 0) {
        delay_short_inline(clock);
      }

      temporary <<= 1;
    }
  }
}

uint16_t bitbang_read_value(const bool wait_for_clock_release) {
  const uint16_t clock = delay_profile->clock;
  const uint16_t numbits = mode_configuration.numbits;
  size_t count;
  uint16_t value;
  if (wait_for_clock_release && i2c_error) {
    return 0xFFFF;
  }

  /* Setup for input (bitbang_read_pin(MOSI), its read discarded). */
  IODIR |= MOSI;
  Nop();
  Nop();
  Nop();
  value = 0;

  /*
   * This is a per-bit hot path for the 2-wire raw protocols and the
   * software I2C, so the per-bit operations are written out directly, in
   * the style of bitbang_advance_clock_ticks():
   *
   * - the high-impedance branch is hoisted (it cannot change mid-value),
   * - the profile delays are loaded once instead of once per bit,
   * - the pin writes compile to single-cycle BSET/BCLR on LATD/TRISD,
   * - the per-bit bitbang_set_pins*() and bitbang_read_pin() calls are
   *   replaced by their bodies (direction setup, three settling Nops,
   *   port read),
   * - delay_short() is called directly.
   *
   * The delays themselves, and therefore the selected speed profile, are
   * unchanged.
   */
  if (mode_configuration.high_impedance == OFF) {
    for (count = 0; count < numbits; count++) {
      if (wait_for_clock_release) {
        if (!bitbang_i2c_clock_high()) {
          return 0xFFFF;
        }
      } else {
        IOLAT |= CLK;
        IODIR &= ~CLK;
        if (clock > 0) {
          delay_short_inline(clock);
        }
      }

      /* Sample the input. */
      IODIR |= MOSI;
      Nop();
      Nop();
      Nop();
      value = (value << 1) | ((IOPOR & MOSI) ? 1 : 0);

      /* Clock low. */
      IOLAT &= ~CLK;
      IODIR &= ~CLK;
      if (clock > 0) {
        delay_short_inline(clock);
      }
    }
  } else {
    for (count = 0; count < numbits; count++) {
      if (wait_for_clock_release) {
        if (!bitbang_i2c_clock_high()) {
          return 0xFFFF;
        }
      } else {
        IODIR |= CLK;
        if (clock > 0) {
          delay_short_inline(clock);
        }
      }

      /* Sample the input. */
      IODIR |= MOSI;
      Nop();
      Nop();
      Nop();
      value = (value << 1) | ((IOPOR & MOSI) ? 1 : 0);

      /* Clock low. */
      IOLAT &= ~CLK;
      IODIR &= ~CLK;
      if (clock > 0) {
        delay_short_inline(clock);
      }
    }
  }

  return value;
}
/**
 * Time limit, in wait-loop iterations, for a
 * slave to release the I2C clock line it is stretching.  A bus that stays
 * stuck longer than this is abandoned so that a faulty device cannot hang
 * the Bus Pirate; the event is counted (bitbang_clock_stretch_timeouts()).
 *
 * This is not a wall-clock deadline: delay_short_inline(1) waits for a timer
 * transition, and GPIO/counter/branch instructions add to each iteration.
 * With FCY=16 MHz and the 2 MHz Timer1 (DS39897C 11.0), the XC16 -O1 V4 bench
 * observed roughly 25 ms of waiting, not 12.5 ms. This covers the SHT3x's
 * 15 ms measurement stretch on that build; it is not a portable SMBus timing
 * guarantee. The finite iteration count is the implementation's hard bound.
 */
#define BB_CLK_STRETCH_TIMEOUT 25000

/**
 * Wait, bounded, until the clock line is actually released (reads high).
 *
 * Open-drain I2C slaves may hold SCL low (clock stretching) on any bit;
 * after this Bus Pirate has released the line, a line that still reads low
 * means the slave is not ready for the next edge.  The first sample goes
 * through bitbang_read_pin(), settling included, so it cannot be an
 * artefact of the previous pin state; the following polls read the pin
 * directly (it is already an input by then), which keeps the cost of the
 * hot polling path down to a read plus a tick delay.  After
 * BB_CLK_STRETCH_TIMEOUT the bus is abandoned for this bit, the event is
 * counted (see bitbang_clock_stretch_timeouts()) and false is returned.
 */
static bool bitbang_wait_for_clock_release(void) {
  uint32_t waited;

  /* First sample: full helper (input direction + settling). */
  if (bitbang_read_pin(CLK)) {
    return true;
  }

  /* Subsequent polls: direct pin reads, one tick apart. */
  for (waited = 0; waited < BB_CLK_STRETCH_TIMEOUT; waited++) {
    delay_short_inline(1);

    if (BP_CLK == HIGH) {
      return true;
    }
  }

  if (bitbang_clk_stretch_timeouts != 0xFFFF) {
    bitbang_clk_stretch_timeouts++;
  }

  return false;
}

bool bitbang_read_bit(const bool wait_for_clock_release) {
  bool bit_value;

  if (wait_for_clock_release && i2c_error) {
    return true;
  }

  /* Set the MISO pin as input. */
  bitbang_read_pin(miso_pin);

  /* Set CLK high. With an open collector output this releases the line. */
  if (wait_for_clock_release) {
    if (!bitbang_i2c_clock_high()) {
      return true;
    }
  } else {
    bitbang_set_pins_high(CLK, delay_profile->clock);
  }

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
  const uint16_t delay = delay_profile->clock;

  /*
   * This is the hot path for the `^` clock command and for the software
   * clock rates in general, so the per-tick operations are written out
   * directly:
   *
   * - the high-impedance branch is hoisted (it cannot change mid-burst),
   * - the pin writes compile to single-cycle BSET/BCLR on LATD/TRISD,
   * - delay_short() is called directly instead of through bp_delay_us(),
   *   saving one call and one branch per clock phase.
   *
   * The delays themselves, and therefore the selected speed profile, are
   * unchanged.
   */
  if (mode_configuration.high_impedance == OFF) {
    for (tick = 0; tick < ticks; tick++) {
      /* Clock high (driven). */
      IOLAT |= CLK;
      IODIR &= ~CLK;
      if (delay > 0) {
        delay_short_inline(delay);
      }

      /* Clock low. */
      IOLAT &= ~CLK;
      IODIR &= ~CLK;
      if (delay > 0) {
        delay_short_inline(delay);
      }
    }
  } else {
    for (tick = 0; tick < ticks; tick++) {
      /* Clock high (open collector: released to the pull-up). */
      IODIR |= CLK;
      if (delay > 0) {
        delay_short_inline(delay);
      }

      /* Clock low. */
      IOLAT &= ~CLK;
      IODIR &= ~CLK;
      if (delay > 0) {
        delay_short_inline(delay);
      }
    }
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

  /* Wait.  Every caller passes a profile-sized delay (at most a few tens
     of microseconds), so the short-delay helper is used directly and the
     bp_delay_us() range branch is skipped. */
  if (delay > 0) {
    delay_short_inline(delay);
  }
}

void bitbang_set_pins_low(const uint16_t pins, const uint16_t delay) {
  /* Mark pins' state as LOW. */
  IOLAT &= ~pins;

  /* Mark pins' direction as OUTPUT. */
  IODIR &= ~pins;

  /* Wait.  Every caller passes a profile-sized delay (at most a few tens
     of microseconds), so the short-delay helper is used directly and the
     bp_delay_us() range branch is skipped. */
  if (delay > 0) {
    delay_short_inline(delay);
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

  /* Wait.  Every caller passes a profile-sized delay (at most a few tens
     of microseconds), so the short-delay helper is used directly and the
     bp_delay_us() range branch is skipped. */
  if (delay > 0) {
    delay_short_inline(delay);
  }
}

bool bitbang_read_pin(const uint16_t pin_bit) {
  IODIR |= pin_bit;
  Nop();
  Nop();
  Nop();
  return IOPOR & pin_bit;
}
