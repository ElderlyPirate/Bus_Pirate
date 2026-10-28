/*
 * This file is part of the Bus Pirate project (buspirate.com).
 *
 * Originally written by hackaday.com <legal@hackaday.com>
 *
 * To the extent possible under law, hackaday.com <legal@hackaday.com> has
 * waived all copyright and related or neighboring rights to Bus Pirate. This
 * work is published from United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include "jtag.h"

#ifdef BP_ENABLE_JTAG_SUPPORT

#include <stdbool.h>
#include <stdint.h>

#include "base.h"

#ifdef BP_JTAG_XSVF_SUPPORT

#include "jtag/micro.h"
#include "jtag/ports.h"

#define XSVF_ERROR_NONE 0
#define XSVF_ERROR_UNKNOWN 1
#define XSVF_ERROR_TDOMISMATCH 2
#define XSVF_ERROR_MAXRETRIES 3 /* TDO mismatch after max retries */
#define XSVF_ERROR_ILLEGALCMD 4
#define XSVF_ERROR_ILLEGALSTATE 5
#define XSVF_ERROR_DATAOVERFLOW 6 /* Data > lenVal MAX_LEN buffer size*/
/* Insert new errors here */
#define XSVF_ERROR_LAST 7

#endif /* BP_JTAG_XSVF_SUPPORT */

#define JTAGDATASETTLE 20
#define JTAGCLOCK 100

#define RESET 0
#define IDLE 1
#define SHIFTIR 2
#define SHIFTDR 3

static void jtag_setup(void);
static void jtag_set_state(const uint8_t new_state);
static void jtag_leave_state(void);
static void jtag_reset(void);
static uint8_t jtag_read_byte(void);
static uint8_t jtag_write_bit(const bool value);
static uint8_t jtag_read_bit(void);
static void jtag_data_low(void);
static void jtag_data_high(void);
static void jtag_clock_low(void);
static void jtag_clock_high(void);
static void jtag_clock_ticks(size_t ticks);
static void jtag_tms_high(void);
static void jtag_tms_low(void);

typedef struct {
  uint8_t state : 2;
  uint8_t hiz : 1;
  uint8_t bit_pending : 1;
  uint8_t delayed_bit : 1;
} jtag_settings_t;

static jtag_settings_t jtag_settings = {0};

void jtag(void) {
  jtag_setup();
  jtag_settings.hiz = true;
  jtag_settings.bit_pending = false;
  jtag_settings.delayed_bit = false;

  for (;;) {
    uint8_t cmd = user_serial_read_byte();

    switch (cmd) {
      /* Reset */
    case 1:
      jtag_reset();
      jtag_leave_state();
      break;

    case 2: { // read ID, chain length, # devices
      jtag_reset();
      jtag_data_high();
      // how many devices?
      //[0xffx255]{while not 1}
      jtag_leave_state();
      jtag_set_state(SHIFTIR);
      jtag_clock_ticks(0xFF);
      jtag_clock_ticks(0xFF);
      jtag_leave_state();
      jtag_set_state(SHIFTDR);
      uint16_t words = 0;
      while (jtag_read_bit() == LOW) {
        words++;
        if (words > 250) {
          break; // 250 device timout/limit...
        }
      }
      jtag_leave_state();
      jtag_reset();
      // read ID#s (32 bits * devices) {r: (4*devices)}
      jtag_leave_state();
      jtag_set_state(SHIFTDR);
      user_serial_transmit_character(words * 4);
      for (size_t word_counter = 0; word_counter < words; word_counter++) {
        user_serial_transmit_character(jtag_read_byte());
        user_serial_transmit_character(jtag_read_byte());
        user_serial_transmit_character(jtag_read_byte());
        user_serial_transmit_character(jtag_read_byte());
      }
      jtag_leave_state();
      break;
    }

#ifdef BP_JTAG_XSVF_SUPPORT
    case 3: {
      // data MUST be low when we start or we get error 3!
      jtag_data_low();
      jtag_clock_low();
      jtag_tms_low();
      xsvf_setup();
      user_serial_transmit_character(xsvfExecute());
      break;
    }
#endif /* BP_JTAG_XSVF_SUPPORT */

    case 0x0F: {
      /* Exit to the binary mode: the standard return-to-terminal byte the
         other binary modes use.  The player loop previously had no way
         out at all, so a tooling park (or an aborted host) trapped the
         port until a reset - same trap class the bridges had.  Send 0x0F
         once more to reach the terminal. */
      jtag_reset();
      return;
    }

    default:
      break;
    }
  }
}

void jtag_leave_state(void) {
  switch (jtag_settings.state) {
  case IDLE:
    break;

  case RESET:
    jtag_tms_low();
    jtag_clock_ticks(1);
    jtag_settings.state = IDLE;
    break;

  case SHIFTIR:
    if (jtag_settings.bit_pending) {
      if (jtag_settings.delayed_bit) {
        jtag_data_high();
      } else {
        jtag_data_low();
      }
      jtag_settings.bit_pending = false;
    }
    jtag_tms_high();
    jtag_clock_ticks(2);
    jtag_tms_low();
    jtag_clock_ticks(1);
    jtag_settings.state = IDLE;
    break;

  case SHIFTDR:
    jtag_tms_high();
    jtag_clock_ticks(2);
    jtag_tms_low();
    jtag_clock_ticks(1);
    jtag_settings.state = IDLE;
    break;

  default:
    break;
  }
}

void jtag_reset(void) {
  jtag_tms_high();
  jtag_clock_ticks(10);
  jtag_tms_low();
  jtag_settings.state = RESET;
}

void jtag_set_state(const uint8_t new_state) {
  switch (new_state) {
  case IDLE:
    break;

  case RESET:
    jtag_reset();
    break;

  case SHIFTDR:
    jtag_tms_high();
    jtag_clock_ticks(1);
    jtag_tms_low();
    jtag_clock_ticks(2);
    jtag_settings.state = SHIFTDR;
    break;

  case SHIFTIR:
    jtag_tms_high();
    jtag_clock_ticks(2);
    jtag_tms_low();
    jtag_clock_ticks(2);
    jtag_settings.state = SHIFTIR;
    break;

  default:
    break;
  }
}

void jtag_setup(void) {
  JTAGTD0_TRIS = INPUT;
  JTAGTCK_TRIS = OUTPUT;
  JTAGTDI_TRIS = OUTPUT;
  JTAGTMS_TRIS = OUTPUT;
  JTAGTDO = LOW;
  JTAGTCK = LOW;
  JTAGTDI = LOW;
  JTAGTMS = LOW;
}

uint8_t jtag_read_byte(void) {
  jtag_clock_low();

  if (jtag_settings.bit_pending) {
    jtag_write_bit(jtag_settings.delayed_bit);
    jtag_settings.bit_pending = false;
  }

  uint8_t value = 0;
  for (size_t bit_index = 0; bit_index < 8; bit_index++) {
    jtag_clock_high();
    value = (value >> 1) | (JTAGTDO << 7);
    jtag_clock_low();
  }

  return value;
}

uint8_t jtag_write_bit(const bool value) {
  if (value == LOW) {
    jtag_data_low();
  } else {
    jtag_data_high();
  }
  jtag_clock_high();
  uint8_t input_bit = JTAGTDO;
  jtag_clock_low();
  return input_bit;
}

uint8_t jtag_read_bit(void) {
  jtag_clock_high();
  uint8_t input_bit = JTAGTDO;
  jtag_clock_low();
  return input_bit;
}

void jtag_data_high(void) {
  JTAGTDI_TRIS = ~jtag_settings.hiz;
  JTAGTDI = jtag_settings.hiz;
  bp_delay_us(JTAGDATASETTLE);
}

void jtag_data_low(void) {
  JTAGTDI = LOW;
  JTAGTDI_TRIS = OUTPUT;
  bp_delay_us(JTAGDATASETTLE);
}

void jtag_clock_ticks(size_t ticks) {
  for (size_t tick = 0; tick < ticks; tick++) {
    jtag_clock_high();
    jtag_clock_low();
  }
}

void jtag_clock_high(void) {
  JTAGTCK_TRIS = ~jtag_settings.hiz;
  JTAGTCK = jtag_settings.hiz;
  bp_delay_us(JTAGCLOCK);
}

void jtag_clock_low(void) {
  JTAGTCK = LOW;
  JTAGTCK_TRIS = OUTPUT;
  bp_delay_us(JTAGCLOCK);
}

void jtag_tms_high(void) {
  JTAGTMS_TRIS = ~jtag_settings.hiz;
  JTAGTMS = jtag_settings.hiz;
  bp_delay_us(JTAGDATASETTLE);
}

void jtag_tms_low(void) {
  JTAGTMS = LOW;
  JTAGTMS_TRIS = OUTPUT;
  bp_delay_us(JTAGDATASETTLE);
}

#endif /* BP_ENABLE_JTAG_SUPPORT */
