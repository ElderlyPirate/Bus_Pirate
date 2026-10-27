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
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include "pc_at_keyboard.h"

#ifdef BP_ENABLE_PC_AT_KEYBOARD_SUPPORT

#include "base.h"

#define KBCLK_TRIS BP_CLK_DIR
#define KBCLK BP_CLK

#define KBDIO_TRIS BP_MOSI_DIR
#define KBDIO BP_MOSI

#define KEYBOARD_CLOCK_DID_CHANGE false
#define KEYBOARD_CLOCK_DID_NOT_CHANGE true

#define KEYBOARD_WRITE_SUCCESS false
#define KEYBOARD_WRITE_TIMEOUT true

extern mode_configuration_t mode_configuration;
extern command_t last_command;

typedef enum {
  KEYBOARD_MACRO_MENU = 0,
  KEYBOARD_MACRO_LIVE_INPUT_MONITOR
} keyboard_macro_identifier_t;

typedef enum {
  KEYBOARD_READ_LOW = 0,
  KEYBOARD_READ_HIGH,
  KEYBOARD_READ_TIMEOUT
} keyboard_read_result_t;

typedef enum {
  KEYBOARD_WRITE_BYTE_ACK = 0,
  KEYBOARD_WRITE_BYTE_NACK,
  KEYBOARD_WRITE_BYTE_TIMEOUT
} keyboard_write_byte_result_t;

typedef enum {
  KEYBOARD_SCANCODE_READ_SUCCESS = 0,
  KEYBOARD_SCANCODE_READ_START_BIT_ERROR,
  KEYBOARD_SCANCODE_READ_PARITY_ERROR,
  KEYBOARD_SCANCODE_READ_STOP_BIT_ERROR,
  KEYBOARD_SCANCODE_READ_TIMEOUT_ERROR,
  KEYBOARD_SCANCODE_READ_NO_DATA = 0xFF
} keyboard_scancode_read_result_t;

static keyboard_read_result_t read_bit(void);
static bool keyboard_wait_clock_change(const bool expected);
static keyboard_scancode_read_result_t read_scancode(uint8_t *output);
static keyboard_scancode_read_result_t read_byte(uint8_t *output);
static keyboard_write_byte_result_t write_byte(const uint8_t value);
static bool write_bit(const bool value);
static void handle_scancode(const keyboard_scancode_read_result_t result);

void pc_at_keyboard_prepare(void) { mode_configuration.high_impedance = ON; }

void pc_at_keyboard_execute(void) {
  KBDIO_TRIS = INPUT;
  KBCLK_TRIS = OUTPUT;
  KBCLK = LOW;
  KBDIO = LOW;
}

uint16_t pc_at_keyboard_read(void) {
  uint8_t scancode;
  handle_scancode(read_byte(&scancode));
  return scancode;
}

uint16_t pc_at_keyboard_send(const uint16_t value) {
  switch (write_byte(value)) {
  case 0:
    MSG_ACK;
    break;

  case 1:
    MSG_NACK;
    break;

  default:
    MSG_KEYBOARD_ERROR_TIMEOUT;
    break;
  }

  return 0x100;
}

void pc_at_keyboard_run_macro(const uint16_t macro) {
  switch (macro) {
  case KEYBOARD_MACRO_MENU:
    MSG_KEYBOARD_MACRO_MENU;
    break;

  case KEYBOARD_MACRO_LIVE_INPUT_MONITOR:
    MSG_KEYBOARD_LIVE_INPUT_START;
    MSG_ANY_KEY_TO_EXIT_PROMPT;
    for (;;) {
      uint8_t scancode;

      if (read_byte(&scancode) == KEYBOARD_SCANCODE_READ_SUCCESS) {
        bp_write_formatted_integer(scancode);
        bpSP;
      }

      if (user_serial_ready_to_read()) {
        user_serial_read_byte();
        bpBR;
        break;
      }
    }
    break;

  default:
    MSG_UNKNOWN_MACRO_ERROR;
    break;
  }
}

void handle_scancode(const keyboard_scancode_read_result_t result) {
  switch (result) {
  case KEYBOARD_SCANCODE_READ_SUCCESS:
    break;

  case KEYBOARD_SCANCODE_READ_START_BIT_ERROR:
    MSG_KEYBOARD_ERROR_STARTBIT;
    break;

  case KEYBOARD_SCANCODE_READ_PARITY_ERROR:
    MSG_KEYBOARD_ERROR_PARITY;
    break;

  case KEYBOARD_SCANCODE_READ_STOP_BIT_ERROR:
    MSG_KEYBOARD_ERROR_STOPBIT;
    break;

  case KEYBOARD_SCANCODE_READ_TIMEOUT_ERROR:
    MSG_KEYBOARD_ERROR_TIMEOUT;
    break;

  case KEYBOARD_SCANCODE_READ_NO_DATA:
    MSG_KEYBOARD_ERROR_NODATA;
    break;

  default:
    MSG_KEYBOARD_ERROR_UNKNOWN;
    break;
  }
}

keyboard_read_result_t read_bit(void) {
  keyboard_read_result_t result;

  if (keyboard_wait_clock_change(LOW) == KEYBOARD_CLOCK_DID_NOT_CHANGE) {
    return KEYBOARD_READ_TIMEOUT;
  }

  result = (keyboard_read_result_t)KBDIO;

  if (keyboard_wait_clock_change(HIGH) == KEYBOARD_CLOCK_DID_NOT_CHANGE) {
    return KEYBOARD_READ_TIMEOUT;
  }

  return result;
}

keyboard_scancode_read_result_t read_scancode(uint8_t *output) {
  uint8_t scancode = 0;
  uint8_t parity = 0;
  uint8_t bit_index;
  keyboard_read_result_t read;

  if (KBDIO != LOW) {
    return KEYBOARD_SCANCODE_READ_START_BIT_ERROR;
  }

  if (keyboard_wait_clock_change(HIGH) == KEYBOARD_CLOCK_DID_NOT_CHANGE) {
    return KEYBOARD_SCANCODE_READ_TIMEOUT_ERROR;
  }

  for (bit_index = 0; bit_index < 8; bit_index++) {
    scancode >>= 1;
    switch (read_bit()) {
    case KEYBOARD_READ_HIGH:
      scancode |= 0b10000000;
      parity ^= 1;
      break;

    case KEYBOARD_READ_TIMEOUT:
      return KEYBOARD_SCANCODE_READ_TIMEOUT_ERROR;

    default:
      break;
    }
  }

  read = read_bit();
  if (read == KEYBOARD_READ_TIMEOUT) {
    return KEYBOARD_SCANCODE_READ_TIMEOUT_ERROR;
  }

  if ((uint8_t)read == parity) {
    return KEYBOARD_SCANCODE_READ_PARITY_ERROR;
  }

  read = read_bit();
  if (read == KEYBOARD_READ_TIMEOUT) {
    return KEYBOARD_SCANCODE_READ_TIMEOUT_ERROR;
  }

  KBCLK_TRIS = OUTPUT;
  if (read != KEYBOARD_READ_HIGH) {
    return KEYBOARD_SCANCODE_READ_STOP_BIT_ERROR;
  }

  *output = scancode;
  return KEYBOARD_SCANCODE_READ_SUCCESS;
}

keyboard_scancode_read_result_t read_byte(uint8_t *result) {
  uint8_t counter;
  keyboard_scancode_read_result_t read = KEYBOARD_SCANCODE_READ_NO_DATA;

  KBDIO_TRIS = INPUT;
  KBCLK_TRIS = INPUT;

  for (counter = 0; counter < 255; counter++) {
    if (KBCLK == LOW) {
      read = read_scancode(result);
      if ((read == KEYBOARD_SCANCODE_READ_SUCCESS) ||
          (read == KEYBOARD_SCANCODE_READ_TIMEOUT_ERROR)) {
        break;
      }
    }

    bp_delay_us(5);
  }

  KBCLK_TRIS = OUTPUT;

  return read;
}

bool write_bit(const bool value) {
  if (keyboard_wait_clock_change(HIGH) == KEYBOARD_CLOCK_DID_NOT_CHANGE) {
    return KEYBOARD_WRITE_TIMEOUT;
  }

  KBDIO_TRIS = value;

  if (keyboard_wait_clock_change(LOW) == KEYBOARD_CLOCK_DID_NOT_CHANGE) {
    return KEYBOARD_WRITE_TIMEOUT;
  }

  return KEYBOARD_WRITE_SUCCESS;
}

keyboard_write_byte_result_t write_byte(const uint8_t value) {
  uint8_t bit_index;
  uint8_t parity = 0;
  keyboard_read_result_t result;

  KBCLK_TRIS = OUTPUT;
  KBCLK = LOW;

  bp_delay_us(60);

  KBDIO = LOW;
  KBDIO_TRIS = OUTPUT;

  KBCLK_TRIS = INPUT;
  bp_delay_us(1);

  if (keyboard_wait_clock_change(LOW) == KEYBOARD_CLOCK_DID_NOT_CHANGE) {
    return KEYBOARD_WRITE_BYTE_TIMEOUT;
  }

  for (bit_index = 0; bit_index < 8; bit_index++) {
    if ((value & (1 << bit_index)) != 0) {
      if (write_bit(HIGH) != KEYBOARD_WRITE_SUCCESS) {
        return KEYBOARD_WRITE_BYTE_TIMEOUT;
      }
      parity ^= 1;
    } else {
      if (write_bit(LOW) != KEYBOARD_WRITE_SUCCESS) {
        return KEYBOARD_WRITE_BYTE_TIMEOUT;
      }
    }
  }

  parity ^= 1;

  if (write_bit(MASKBOTTOM8(parity, 1)) != KEYBOARD_WRITE_SUCCESS) {
    return KEYBOARD_WRITE_BYTE_TIMEOUT;
  }

  KBDIO_TRIS = INPUT;
  KBDIO = LOW;

  if (keyboard_wait_clock_change(HIGH) == KEYBOARD_CLOCK_DID_NOT_CHANGE) {
    return KEYBOARD_WRITE_BYTE_TIMEOUT;
  }

  if (read_bit() == KEYBOARD_READ_TIMEOUT) {
    return KEYBOARD_WRITE_BYTE_TIMEOUT;
  }

  result = read_bit();
  if (result == KEYBOARD_READ_TIMEOUT) {
    return KEYBOARD_WRITE_BYTE_TIMEOUT;
  }

  KBCLK_TRIS = OUTPUT;
  KBCLK = LOW;

  return (result == KEYBOARD_READ_LOW) ? KEYBOARD_WRITE_BYTE_ACK
                                       : KEYBOARD_WRITE_BYTE_NACK;
}

bool keyboard_wait_clock_change(const bool expected) {
  volatile uint16_t delay = 0xFFFF;

  while (delay > 0) {
    if (KBCLK == expected) {
      return KEYBOARD_CLOCK_DID_CHANGE;
    }

    --delay;
  }

  return KEYBOARD_CLOCK_DID_NOT_CHANGE;
}

#endif /* BP_ENABLE_PC_AT_KEYBOARD_SUPPORT */
