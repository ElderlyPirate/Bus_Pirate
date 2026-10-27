/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Initially written by Chris van Dongen, 2010.
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

#include "pic.h"

#ifdef BP_ENABLE_PIC_SUPPORT

#include "aux_pin.h"
#include "base.h"
#include "binary_io.h"
#include "bitbang.h"
#include "core.h"
#include "proc_menu.h"

/**
 * PIC opcode width in bits.
 *
 * @see pic_mode_t.opcode_bits
 */
typedef enum {
  /**
   * Opcodes are six bits wide.
   */
  PIC_MODE_6BITS = 0,

  /**
   * Opcodes are four bits wide.
   */
  PIC_MODE_4BITS
} pic_mode_bits;

/**
 * Flag indicating that the Bus Pirate is not sending commands to the PIC chip.
 *
 * @see PIC_MODE_COMMAND
 */
#define PIC_MODE_DATA 0

/**
 * Flag indicating that the Bus Pirate is sending commands to the PIC chip.
 *
 * @see PIC_MODE_DATA
 */
#define PIC_MODE_COMMAND 1

/**
 * Structure that handles all needed information for the bus mode operations.
 */
typedef struct {
  /**
   * Flag indicating opcode bits width/
   *
   * @see PIC_MODE_6BITS
   * @see PIC_MODE_4BITS
   */
  uint8_t opcode_bits : 1;

  /**
   * Delay to apply after each write, in milliseconds.
   */
  uint8_t delay : 2;

  /**
   * Operation mode, either command or data.
   *
   * @see PIC_MODE_COMMAND
   * @see PIC_MODE_DATA
   */
  uint8_t mode : 1;

  /**
   * Reserved, should always be set to 0.
   */
  uint8_t reserved : 4;
} pic_mode_t;

/**
 * Delay to apply after each pin state change, in microseconds.
 */
#define PIC_PIN_DELAY 100

extern bus_pirate_configuration_t bus_pirate_configuration;
extern mode_configuration_t mode_configuration;

/**
 * Mode operation information container.
 */
static pic_mode_t mode_info = {0};

/**
 * Writes a null byte to the bus.
 */
static void write_zero(void);

/**
 * Writes the given bits to the bus.
 * 
 * Bits are sent starting from LSB all the way to MSB.
 * 
 * @param data[in] the bits to send.
 * @param bits[in] how many bits to send.
 */
static void write_bits(const uint16_t data, const uint8_t bits);

/**
 * Reads the given amount of bits from the bus.
 * 
 * Read bits are right-aligned.
 * 
 * @param bits[in] how many bits to read.
 * 
 * @return the bits read from the bus.
 */
static uint16_t read_bits(const uint8_t bits);

void pic_setup_prepare(void) {
  consumewhitechars();
  int mode = getint();
  consumewhitechars();
  int delay = getint();
  bool interactive =
      !(((mode > 0) && (mode <= 2)) && ((delay >= 0) && (delay <= 3)));

  if (interactive) {
    mode_configuration.command_error = NO;
    MSG_PIC_MODE_PROMPT;
    mode_info.opcode_bits = getnumber(1, 1, 2, 0) - 1;
    MSG_PIC_DELAY_PROMPT;
    mode_info.delay = getnumber(1, 0, 3, 0);
  } else {
    MSG_PIC_MODE_HEADER;
    bp_write_dec_byte(mode_info.mode == PIC_MODE_6BITS ? 6 : 4);
    bpSP;
    bp_write_dec_byte(mode_info.delay);
    bp_write_line(")");
  }

  /* Allow for different Vcc. */
  mode_configuration.high_impedance = YES;
  mode_configuration.int16 = YES;

  /* Pull both pins low before applying Vcc and Vpp. */
  bitbang_set_pins_low(MOSI | CLK, PIC_PIN_DELAY);
}

void pic_setup_execute(void) { mode_configuration.int16 = YES; }

void pic_cleanup(void) { mode_configuration.int16 = NO; }

void pic_start(void) {
  mode_info.mode = PIC_MODE_COMMAND;
  MSG_PIC_MODE_COMMAND;
  user_serial_transmit_character(mode_info.mode == PIC_MODE_6BITS ? '6' : '4');
  mode_configuration.int16 = NO;
  bpBR;
}

void pic_stop(void) {
  mode_info.mode = PIC_MODE_DATA;
  mode_configuration.int16 = YES;
  MSG_PIC_MODE_DATA;
}

uint16_t read_bits(const uint8_t bits) {
  uint16_t data = 0;

  for (size_t index = 0; index < bits; index++) {
    bitbang_set_pins_high(CLK, PIC_PIN_DELAY / 2);
    data = (data >> 1) |
           ((bitbang_read_pin(MOSI) == HIGH ? 1 : 0) << (bits - 1));
    bitbang_set_pins_low(CLK, PIC_PIN_DELAY / 2);
  }

  bitbang_set_pins_high(CLK, PIC_PIN_DELAY / 2);
  bitbang_set_pins_low(CLK, PIC_PIN_DELAY / 2);
  bitbang_set_pins_low(MOSI, PIC_PIN_DELAY / 5);

  return data;
}

unsigned int pic_read(void) {
  if (mode_info.mode == PIC_MODE_COMMAND) {
    MSG_PIC_NO_READ;
    return 0;
  }

  switch (mode_info.opcode_bits) {
  case PIC_MODE_6BITS:
    bitbang_read_pin(MOSI);
    bitbang_set_pins_high(CLK, PIC_PIN_DELAY / 2);
    bitbang_set_pins_low(CLK, PIC_PIN_DELAY / 2);
    return read_bits(14);

  case PIC_MODE_4BITS:
    bitbang_read_pin(MOSI);
    return read_bits(16);

  default:
    MSG_PIC_UNKNOWN_MODE;
    return 0;
  }
}

void write_bits(const uint16_t data, const uint8_t bits) {
  uint16_t mask = 1;

  for (size_t index = 0; index < bits; index++) {
    bitbang_set_pins_high(CLK, PIC_PIN_DELAY / 4);
    bitbang_set_pins((data & mask) == mask, MOSI, PIC_PIN_DELAY / 4);
    bitbang_set_pins_low(CLK, PIC_PIN_DELAY / 4);
    bitbang_set_pins_low(MOSI, PIC_PIN_DELAY / 4);
    mask <<= 1;
  }
}

void write_zero(void) {
  bitbang_set_pins_high(CLK, PIC_PIN_DELAY / 4);
  bitbang_set_pins_low(MOSI, PIC_PIN_DELAY / 4);
  bitbang_set_pins_low(CLK, PIC_PIN_DELAY / 2);
}

uint16_t pic_write(const uint16_t value) {
  if (mode_info.mode == PIC_MODE_COMMAND) {
    switch (mode_info.opcode_bits) {
    case PIC_MODE_6BITS:
      write_bits(value, 6);
      break;

    case PIC_MODE_4BITS:
      write_bits(value, 4);
      break;

    default:
      MSG_PIC_UNKNOWN_MODE;
      return 0;
    }

    bp_delay_ms(mode_info.delay);
  } else {
    switch (mode_info.opcode_bits) {
    case PIC_MODE_6BITS:
      write_zero();
      write_bits(value, 14);
      write_zero();
      break;

    case PIC_MODE_4BITS:
      write_bits(value, 16);
      break;

    default:
      MSG_PIC_UNKNOWN_MODE;
      return 0;
    }
  }

  /* No data to show. */
  return 0x100;
}

void pic_run_macro(const uint16_t macro) {
  switch (macro) {
  case 0:
    MSG_PIC_MACRO_MENU;
    break;

  case 1:
    switch (mode_info.opcode_bits) {
    case PIC_MODE_6BITS: {
      bus_pirate_configuration.quiet = YES;
      pic_start();
      pic_write(0);
      pic_stop();
      /* Skip to device id. */
      pic_write(0);
      pic_start();
      for (size_t index = 0; index < 6; index++) {
        pic_write(6);
      }
      pic_write(4);
      pic_stop();
      uint16_t temp = pic_read();
      bus_pirate_configuration.quiet = NO;
      MSG_PIC_DEVICE_ID;
      bp_write_hex_word(temp >> 5);
      MSG_PIC_REVISION_ID;
      bp_write_hex_byte(temp & 0x1F);
      bpBR;
      break;
    }

    case PIC_MODE_4BITS:
    default:
      MSG_PIC_MACRO_NOT_IMPLEMENTED;
    }

    MSG_PIC_EXIT_MODE;
    break;

  default:
    MSG_UNKNOWN_MACRO_ERROR;
    break;
  }
}

void pic_print_pins_state(void) { MSG_PIC_PINS_STATE; }

/*
0000 0000	return to main
0000 0001	id=PIC1
0000 0010	6b cmd
0000 0011	4b cmd
0000 01xx	xx ms delay

0000 1xxx	unimplemented

0001 0xyz	PWM|VREG|PULLUP
0001 1xyz   AUX|MISO|CS

01xx xxxx	just send cmd xxxxxxx
10xx xxxx	send cmd xxxxxxx and next two bytes (14/16 bits)
11xx xxxx	send cmd xxxxxxx and read two bytes

*/

void binary_io_enter_pic_mode(void) {
  MSG_PIC_MODE_IDENTIFIER;
  
  /* Allow for different Vcc. */
  mode_configuration.high_impedance = YES;

  /* Pull both pins low before applying voltages. */
  bitbang_set_pins_low(MOSI | CLK, PIC_PIN_DELAY);
  mode_info.opcode_bits = PIC_MODE_6BITS;
  mode_info.mode = PIC_MODE_DATA;
  mode_info.delay = 2;

  for (;;) {
    uint8_t command = user_serial_read_byte();
    bool ok = false;

    switch (command & 0xC0) {
    case 0x00:
      ok = true;
      switch (command & 0xF0) {
      case 0x00:
        switch (command) {
        case 0x00:
          return;

        case 0x01:
          MSG_PIC_MODE_IDENTIFIER;
          break;

        case 0x02:
          mode_info.opcode_bits = PIC_MODE_6BITS;
          break;

        case 0x03:
          mode_info.opcode_bits = PIC_MODE_4BITS;
          break;

        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
          mode_info.delay = (command - 4);
          break;

        default:
          ok = false;
        }
        break;

      case 0x10:
        if (command & 0x08) {
          bitbang_set_pins((command & 0x04) == 0x04, AUX, 5);
          bitbang_set_pins((command & 0x02) == 0x02, MISO, 5);
          bitbang_set_pins((command & 0x01) == 0x01, CS, 5);
        } else {
          if (command & 0x04) {
            bp_update_pwm(100, 50);
          } else {
            bp_update_pwm(PWM_OFF, PWM_OFF);
          }
          bp_set_voltage_regulator_state((command & 0x02) == 0x02);
          bp_set_pullup_state((command & 0x01) == 0x01);
        }
        break;

      default:
        ok = false;
        break;
      }

      user_serial_transmit_character(ok != 0 ? BP_BINARY_IO_RESULT_SUCCESS
                                             : BP_BINARY_IO_RESULT_FAILURE);
      break;

    case 0x40:
      mode_info.mode = PIC_MODE_COMMAND;
      pic_write(command & 0x3F);
      mode_info.mode = PIC_MODE_DATA;
      REPORT_IO_SUCCESS();
      break;

    case 0x80: {
      mode_info.mode = PIC_MODE_COMMAND;
      pic_write(command & 0x3F);
      mode_info.mode = PIC_MODE_DATA;
      uint16_t temp = user_serial_read_byte();
      temp <<= 8;
      temp |= user_serial_read_byte();
      pic_write(temp);
      REPORT_IO_SUCCESS();
      break;
    }

    case 0xC0: {
      mode_info.mode = PIC_MODE_COMMAND;
      pic_write(command & 0x3F);
      mode_info.mode = PIC_MODE_DATA;
      REPORT_IO_SUCCESS();
      uint16_t temp = pic_read();
      user_serial_transmit_character(temp >> 8);
      user_serial_transmit_character(temp & 0x0FF);
      break;
    }
    }
  }
}

#endif /* BP_ENABLE_PIC_SUPPORT */