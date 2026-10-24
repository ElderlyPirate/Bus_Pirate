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

#include "raw2wire.h"

#ifdef BP_ENABLE_RAW_2WIRE_SUPPORT

#include "aux_pin.h"
#include "base.h"
#include "bitbang.h"
#include "proc_menu.h"

#define R2WCLK_TRIS BP_CLK_DIR
#define R2WCLK BP_CLK

#define R2WDIO_TRIS BP_MOSI_DIR
#define R2WDIO BP_MOSI

#define ATR_HEADER_LENGTH 4

typedef enum {
  RAW2WIRE_MACRO_MENU = 0,
  RAW2WIRE_MACRO_ISO_7816_3_ATR,
  RAW2WIRE_MACRO_ISO_7816_3_ATR_PARSE
} raw2wire_macro_identifier_t;

extern mode_configuration_t mode_configuration;
extern command_t last_command;

static const uint32_t DATA_UNIT_LENGTH_TABLE[14] = {
    128,   256,   512,   1024,   2048,   4096,   8192,
    16384, 32768, 65536, 131072, 262144, 524288, 1048576};

static const uint32_t DATA_UNIT_TABLE[8] = {
    1, 2, 4, 8, 16, 32, 64, 128,
};

/**
 * Parses an ATR reply from an ISO 7816-3 compliant card.
 */
static void read_atr_header(void);

/**
 * Sends an ATR signal (attention to reset) to an ISO 7816-3 compliant card.
 */
static void trigger_atr(void);

uint16_t raw2wire_read(void) { return bitbang_read_value(); }

uint16_t raw2wire_write(const uint16_t value) {
  bitbang_write_value(value);
  return 0x100;
}

void raw2wire_start(void) {
  bitbang_i2c_start(BITBANG_I2C_START_ONE_SHOT);
  MSG_RAW2WIRE_I2C_START;
  MSG_I2C_START_BIT;
}

void raw2wire_stop(void) {
  bitbang_i2c_stop();
  MSG_RAW2WIRE_I2C_STOP;
  MSG_I2C_STOP_BIT;
}

void raw2wire_print_settings(void) {
  MSG_RAW2WIRE_MODE_HEADER;
  bp_write_dec_byte(mode_configuration.speed);
  bpSP;
  bp_write_dec_byte(mode_configuration.high_impedance);
  bpSP;
  MSG_MODE_HEADER_END;
}

void raw2wire_setup_prepare(void) {
  consumewhitechars();
  int speed = getint();
  consumewhitechars();
  int output = getint();

  bool user_prompt =
      !(((speed > 0) && (speed <= 4)) && ((output > 0) && (output <= 2)));

  if (user_prompt) {
    MSG_SOFTWARE_MODE_SPEED_PROMPT;
    mode_configuration.speed = getnumber(1, 1, 4, 0) - 1;
    MSG_PIN_OUTPUT_TYPE_PROMPT;
    mode_configuration.high_impedance = ~(getnumber(1, 1, 2, 0) - 1);
    mode_configuration.command_error = NO;
  } else {
    mode_configuration.speed = speed - 1;
    mode_configuration.high_impedance = (output - 1) == 0;
    raw2wire_print_settings();
  }

  /*
   * Apply the timing profile here as well: the interactive flow only runs
   * setup_execute() on the first power-on request (`W`), so a mode that is
   * never powered would otherwise clock with the previous mode's profile.
   * The pins themselves are still configured by setup_execute().
   */
  bitbang_setup(2, mode_configuration.speed);
}

void raw2wire_setup_execute(void) {
  R2WCLK = LOW;
  R2WDIO = LOW;
  R2WDIO_TRIS = INPUT;
  R2WCLK_TRIS = OUTPUT;
  bitbang_setup(2, mode_configuration.speed);
}

void raw2wire_run_macro(const uint16_t macro_id) {
  switch ((raw2wire_macro_identifier_t)macro_id) {
  case RAW2WIRE_MACRO_MENU:
    MSG_RAW2WIRE_MACRO_MENU;
    break;

  case RAW2WIRE_MACRO_ISO_7816_3_ATR:
    trigger_atr();
  /* Intentional pass-through. */

  case RAW2WIRE_MACRO_ISO_7816_3_ATR_PARSE:
    read_atr_header();
    break;

  default:
    MSG_UNKNOWN_MACRO_ERROR;
    break;
  }
}

void raw2wire_print_pins_state(void) { MSG_I2C_PINS_STATE; }

void trigger_atr(void) {
  MSG_RAW2WIRE_ATR_TRIGGER_INFO;

  bitbang_set_cs(LOW);
  bp_delay_us(0xFF);
  bitbang_set_cs(HIGH);
  bitbang_advance_clock_ticks(1);
  bitbang_set_cs(LOW);
}

void read_atr_header(void) {
  uint8_t buffer[ATR_HEADER_LENGTH];

  MSG_RAW2WIRE_ATR_REPLY_HEADER;
  for (size_t index = 0; index < ATR_HEADER_LENGTH; index++) {
    buffer[index] = bitbang_read_value();
    if (mode_configuration.little_endian == YES) {
      buffer[index] = bp_reverse_byte(buffer[index]);
    }
    bp_write_hex_byte(buffer[index]);
    bpSP;
  }
  bpBR;

  MSG_RAW2WIRE_ATR_PROTOCOL_HEADER;
  switch ((buffer[0] >> 4)) {
  case 0x08:
    MSG_RAW2WIRE_ATR_PROTOCOL_SERIAL;
    break;

  case 0x09:
    MSG_RAW2WIRE_ATR_PROTOCOL_3WIRE;
    break;

  case 0x0A:
    MSG_RAW2WIRE_ATR_PROTOCOL_2WIRE;
    break;

  case 0x0F:
    MSG_RAW2WIRE_ATR_RFU;
    break;

  default:
    MSG_RAW2WIRE_ATR_PROTOCOL_UNKNOWN;
    break;
  }
  bpBR;

  MSG_RAW2WIRE_ATR_READ_TYPE_HEADER;
  if ((buffer[1] & 0x80) == 0) {
    MSG_RAW2WIRE_ATR_READ_TYPE_TO_END;
  } else {
    MSG_RAW2WIRE_ATR_READ_TYPE_VARIABLE_LENGTH;
  }
  bpBR;

  MSG_RAW2WIRE_ATR_DATA_UNITS_HEADER;
  uint8_t length = (buffer[1] >> 3) & 0b00001111;
  switch (length) {
  case 0:
    MSG_RAW2WIRE_ATR_DATA_UNITS_NO_INDICATION;
    break;

  case 0b1111:
    MSG_RAW2WIRE_ATR_RFU;
    break;

  default:
    bp_write_dec_dword(DATA_UNIT_TABLE[length]);
    break;
  }

  MSG_RAW2WIRE_ATR_DATA_UNIT_LENGTH;
  bp_write_dec_byte(DATA_UNIT_LENGTH_TABLE[buffer[1] & 0b00000111]);
  bpBR;
}

#endif /* BP_ENABLE_RAW_2WIRE_SUPPORT */
