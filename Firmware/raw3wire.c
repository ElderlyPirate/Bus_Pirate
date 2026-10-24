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

#include "raw3wire.h"

#ifdef BP_ENABLE_RAW_3WIRE_SUPPORT

#include "base.h"
#include "bitbang.h"
#include "proc_menu.h"

/* Pin aliases. */

#define R3WMOSI_TRIS BP_MOSI_DIR
#define R3WCLK_TRIS BP_CLK_DIR
#define R3WMISO_TRIS BP_MISO_DIR

extern mode_configuration_t mode_configuration;
extern command_t last_command;

/**
 * Raw 3 Wire mode chip select initial line state.
 */
static bool cs_line = LOW;

/**
 * Sets up the board for operating in Raw 3 Mode state.
 *
 * @param[in] write_with_read flag indicating whether write operations imply a
 *                            read operation afterwards.
 * @param[in] cs_line_state the CS line state.
 */
static void setup_raw3wire(const bool write_with_read,
                           const bool cs_line_state);

uint16_t raw3wire_read(void) { return bitbang_read_with_write(0xFF); }

uint16_t raw3wire_write(const uint16_t value) {
  uint16_t read;

  read = bitbang_read_with_write(value);
  return mode_configuration.write_with_read ? read : 0;
}

void raw3wire_start_with_read(void) {
  setup_raw3wire(YES, !cs_line);
  MSG_SPI_CS_ENABLED;
}
void raw3wire_start(void) {
  setup_raw3wire(NO, !cs_line);
  MSG_SPI_CS_ENABLED;
}

void raw3wire_stop(void) {
  setup_raw3wire(NO, cs_line);
  MSG_SPI_CS_DISABLED;
}

void raw3wire_print_settings(void) {
  MSG_RAW3WIRE_MODE_HEADER;
  bp_write_dec_byte(mode_configuration.speed);
  bpSP;
  bp_write_dec_byte(cs_line);
  bpSP;
  bp_write_dec_byte(mode_configuration.high_impedance);
  MSG_MODE_HEADER_END;
}

void raw3wire_setup_prepare(void) {
  bool user_prompt;
  int speed;
  int output;
  int cs_line_low;

  consumewhitechars();
  speed = getint();
  consumewhitechars();
  cs_line_low = getint();
  consumewhitechars();
  output = getint();

  user_prompt = !(((speed > 0) && (speed <= 4)) &&
                  ((cs_line_low > 0) && (cs_line_low <= 2)) &&
                  ((output > 0) && (output <= 2)));

  if (user_prompt) {
    MSG_SOFTWARE_MODE_SPEED_PROMPT;
    mode_configuration.speed = getnumber(1, 1, 4, 0) - 1;
    MSG_SPI_CS_MODE_PROMPT;
    cs_line = getnumber(2, 1, 2, 0) - 1;
    MSG_PIN_OUTPUT_TYPE_PROMPT;
    mode_configuration.high_impedance = (getnumber(1, 1, 2, 0) - 1) == 0;
    mode_configuration.command_error = NO;
  } else {
    mode_configuration.speed = speed - 1;
    cs_line = (cs_line_low - 1) != 0;
    mode_configuration.high_impedance = (output - 1) == 0;
    raw3wire_print_settings();
  }

  mode_configuration.write_with_read = NO;
  mode_configuration.int16 = NO;

  /*
   * Apply the timing profile here as well: the interactive flow only runs
   * setup_execute() on the first power-on request (`W`), so a mode that is
   * never powered would otherwise clock with the previous mode's profile.
   * The pins themselves are still configured by setup_execute().
   */
  bitbang_setup(3, mode_configuration.speed);
}

void raw3wire_setup_execute(void) {
  bitbang_setup(3, mode_configuration.speed);
  R3WMOSI_TRIS = OUTPUT;
  R3WCLK_TRIS = OUTPUT;
  R3WMISO_TRIS = INPUT;
  bitbang_set_cs(cs_line);
}

void raw3wire_print_pins_state(void) { MSG_SPI_PINS_STATE; }

void setup_raw3wire(const bool write_with_read, const bool cs_line_state) {
  mode_configuration.write_with_read = write_with_read;
  bitbang_set_cs(!cs_line_state);
  if (cs_line_state) {
    user_serial_transmit_character('/');
  }
}

#endif /* BP_ENABLE_RAW_3WIRE_SUPPORT */
