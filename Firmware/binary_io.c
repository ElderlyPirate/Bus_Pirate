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

#include <stdbool.h>

/* Binary access modes for Bus Pirate scripting */

#include "aux_pin.h"
#include "base.h"
#include "binary_io.h"
#include "bitbang.h"
#include "configuration.h"
#include "core.h"
#include "selftest.h"

#ifdef BP_ENABLE_SPI_SUPPORT
#include "spi.h"
#endif /* BP_ENABLE_SPI_SUPPORT */

#ifdef BP_ENABLE_I2C_SUPPORT
#include "i2c.h"
#endif /* BP_ENABLE_I2C_SUPPORT */

#ifdef BP_ENABLE_UART_SUPPORT
#include "uart.h"
#endif /* BP_ENABLE_UART_SUPPORT */

#ifdef BP_ENABLE_1WIRE_SUPPORT
#include "1wire.h"
#endif /* BP_ENABLE_1WIRE_SUPPORT */

#ifdef BP_ENABLE_PIC_SUPPORT
#include "pic.h"
#endif /* BP_ENABLE_PIC_SUPPORT */

#ifdef BP_ENABLE_JTAG_SUPPORT
#include "jtag.h"
#ifdef BP_JTAG_OPENOCD_SUPPORT
#include "openocd.h"
#endif /* BP_JTAG_OPENOCD_SUPPORT */
#endif /* BP_ENABLE_JTAG_SUPPORT */

#ifdef BP_ENABLE_SMPS_SUPPORT
#include "smps.h"
#endif /* BP_ENABLE_SMPS_SUPPORT */

extern mode_configuration_t mode_configuration;
extern bus_pirate_configuration_t bus_pirate_configuration;

/**
 * Sends the Binary I/O mode identifier over to the serial channel.
 *
 * Unlike other instances of mode identifier reporting, this is called several
 * times in the same function and therefore it makes sense to encapsulate it in
 * a single function to save on code size.
 */
static void send_binary_io_mode_identifier(void);

typedef enum {
  IO_COMMAND_GROUP_GENERIC = 0b0000,
  IO_COMMAND_GROUP_BULK_BYTE_TRANSFER = 0b0001,
  IO_COMMAND_GROUP_BULK_CLOCK_TICKS_ADVANCE = 0b0010,
  IO_COMMAND_GROUP_BULK_BIT_TRANSFER = 0b0011,
  IO_COMMAND_GROUP_CONFIGURE_PERIPHERALS = 0b0100,
  IO_COMMAND_GROUP_SET_PULLUP = 0b0101,
  IO_COMMAND_GROUP_SET_SPEED = 0b0110,
  IO_COMMAND_GROUP_CONFIGURATION = 0b1000,
  IO_COMMAND_GROUP_PIC = 0b1010,
  IO_COMMAND_GROUP_SMPS = 0b1111
} io_command_group;

typedef enum {
  IO_COMMAND_EXIT = 0,
  IO_COMMAND_REPORT_ID_STRING,
  IO_COMMAND_SEND_I2C_START_BIT,
  IO_COMMAND_SEND_I2C_STOP_BIT,
  IO_COMMAND_CS_LOW,
  IO_COMMAND_CS_HIGH,
  IO_COMMAND_BITBANG_READ_BYTE,
  IO_COMMAND_BITBANG_READ_BIT,
  IO_COMMAND_PEEK_INPUT_BIT,
  IO_COMMAND_CLOCK_TICK,
  IO_COMMAND_CLOCK_LOW,
  IO_COMMAND_CLOCK_HIGH,
  IO_COMMAND_DATA_LOW,
  IO_COMMAND_DATA_HIGH
} wire_generic_command;

typedef enum {
  PIC_MODE_INVALID = 0,
  PIC_MODE_416,
  PIC_MODE_424,
  PIC_MODE_614,
  PIC_MODE_COUNT
} pic_mode;

typedef enum {
  PIC_USER_COMMAND_WRITE = 1,
  PIC_USER_COMMAND_READ
} pic_command_type;

typedef enum {
  PIC_COMMAND_SET_MODE = 0xA0,
  PIC_COMMAND_WRITE = 0xA4,
  PIC_COMMAND_WRITE_AND_READ_BITS = 0xA5,
  PIC_COMMAND_WRITE_AND_READ_COMMANDS = 0xA7
} pic_command;

typedef enum {
  SMPS_COMMAND_GET_OUTPUT_VOLTAGE = 0xF0,
  SMPS_COMMAND_STOP = 0xF1,
  SMPS_COMMAND_START = 0xF2
} smps_command;

typedef enum {
  BITBANG_COMMAND_RESET = 0x00,
  BITBANG_COMMAND_SPI,
  BITBANG_COMMAND_I2C,
  BITBANG_COMMAND_UART,
  BITBANG_COMMAND_1WIRE,
  BITBANG_COMMAND_RAW_WIRE,
  BITBANG_COMMAND_OPENOCD,
  BITBANG_COMMAND_PIC,
  BITBANG_COMMAND_RETURN_TO_TERMINAL = 0x0F,
  BITBANG_COMMAND_SHORT_SELF_TEST,
  BITBANG_COMMAND_FULL_SELF_TEST,
  BITBANG_COMMAND_SETUP_PWM,
  BITBANG_COMMAND_CLEAR_PWM,
  BITBANG_COMMAND_ADC_ONE_SHOT,
  BITBANG_COMMAND_ADC_CONTINUOUS,
  BITBANG_COMMAND_FREQUENCY_COUNT,
  BITBANG_COMMAND_JTAG_XSVF = 0x18
} bitbang_command;

/**
 * Write and read bits payload for PIC24 SIX commands.
 *
 * Original data is 0xBA0B96, 0xBADB86, 0xBAD3D6, 0xBA0B86.
 *
 * This version is the preprocessed version of those words (from LSB to MSB and
 * bit-reversed).
 */
static const uint8_t PIC24_WRITE_AND_READ_PAYLOAD[3 * 4] = {
    0x69, 0xD0, 0x5D, 0x6D, 0xDB, 0x5D, 0x6B, 0xCB, 0x5D, 0x69, 0xD0, 0x5D};

static const uint8_t PIC24_NOP_PAYLOAD[3] = {0};

/**
 * @brief Raw wire command handler.
 *
 * Commands are split in two halves, the first four bits from MSB being the
 * command category and the last four bits being the command parameter (or,
 * [7:4] and [3:0], respectively).
 *
 * Currently available commands are as follows:
 *
 * <table>
 * <thead>
 * <tr><th>Binary</th><th>Hexadecimal</th><th>Description</th></tr>
 * </thead>
 * <tbody>
 * <tr><td><tt>0b0000xxxx</tt></td><td><tt>0x0x</tt></td><td>Generic</td></tr>
 * <tr><td><tt>0b0001xxxx</tt></td><td><tt>0x1x</tt></td><td>Bulk byte
 * transfer</td></tr>
 * <tr><td><tt>0b0010xxxx</tt></td><td><tt>0x2x</tt></td><td>Bulk clock ticks
 * advance</td></tr>
 * <tr><td><tt>0b0011xxxx</tt></td><td><tt>0x3x</tt></td><td>Bulk bit
 * transfer</td></tr>
 * <tr><td><tt>0b0100xxxx</tt></td><td><tt>0x4x</tt></td><td>Peripherals
 * configuration</td></tr>
 * <tr><td><tt>0b0101xxxx</tt></td><td><tt>0x5x</tt></td><td>Pullup
 * configuration</td></tr>
 * <tr><td><tt>0b0110xxxx</tt></td><td><tt>0x6x</tt></td><td>Set speed</td></tr>
 * <tr><td><tt>0b1000xxxx</tt></td><td><tt>0x8x</tt></td><td>Mode
 * configuration</td></tr>
 * <tr><td><tt>0b1010xxxx</tt></td><td><tt>0xAx</tt></td><td>PIC
 * programming</td></tr>
 * <tr><td><tt>0b1111xxxx</tt></td><td><tt>0xFx</tt></td><td>SMPS</td></tr>
 * </tbody>
 * </table>
 *
 * @see handle_wire_generic_command
 * @see handle_bulk_byte_transfer
 * @see handle_bulk_clock_ticks_advance
 * @see handle_bulk_bit_transfer
 * @see handle_set_pullup
 * @see handle_configure_peripherals
 * @see handle_set_speed
 * @see handle_configuration
 * @see handle_pic_command
 * @see handle_smps_command
 * @see io_command_group
 */
static inline void binary_io_raw_wire_mode_handler(void);

/**
 * @brief Processes a generic category command obtained by
 * binary_io_raw_wire_mode_handler.
 *
 * Currently available commands are as follows:
 *
 * <table>
 * <thead>
 * <tr><th>Binary</th><th>Hexadecimal</th><th>Description</th></tr>
 * </thead>
 * <tbody>
 * <tr><td><tt>0b00000000</tt></td><td><tt>0x00</tt></td><td>Exit raw wire
 * mode.</td></tr>
 * <tr><td><tt>0b00000001</tt></td><td><tt>0x01</tt></td><td>Report mode
 * identifier.</td></tr>
 * <tr><td><tt>0b00000010</tt></td><td><tt>0x02</tt></td><td>Send I2C start
 * bit.</td></tr> <tr><td><tt>0b00000011</tt></td><td><tt>0x03</tt></td><td>Send
 * I2C stop bit.</td></tr>
 * <tr><td><tt>0b00000100</tt></td><td><tt>0x04</tt></td><td>Pull CS
 * low.</td></tr> <tr><td><tt>0b00000101</tt></td><td><tt>0x05</tt></td><td>Set
 * CS high.</td></tr>
 * <tr><td><tt>0b00000110</tt></td><td><tt>0x06</tt></td><td>Read a byte in raw
 * wire mode.</td></tr>
 * <tr><td><tt>0b00000111</tt></td><td><tt>0x07</tt></td><td>Read a bit in raw
 * wire mode.</td></tr>
 * <tr><td><tt>0b00001000</tt></td><td><tt>0x08</tt></td><td>Peek at the bit
 * state.</td></tr>
 * <tr><td><tt>0b00001001</tt></td><td><tt>0x09</tt></td><td>Advance clock
 * tick.</td></tr>
 * <tr><td><tt>0b00001010</tt></td><td><tt>0x0A</tt></td><td>Pull CLK
 * low.</td></tr> <tr><td><tt>0b00001011</tt></td><td><tt>0x0B</tt></td><td>Set
 * CLK high.</td></tr>
 * <tr><td><tt>0b00001100</tt></td><td><tt>0x03</tt></td><td>Pull DATA
 * low.</td></tr> <tr><td><tt>0b00001101</tt></td><td><tt>0x03</tt></td><td>Set
 * DATA high.</td></tr>
 * </tbody>
 * </table>
 *
 * Any commands that are not in the above list will trigger a failure result
 * code being sent over the serial port.
 *
 * @param[in] command the command to process.
 * @return true if binary_io_raw_wire_mode_handler should accept more commands,
 * false if binary raw wire mode handling should end.
 *
 * @see binary_io_raw_wire_mode_handler
 * @see wire_generic_command
 */
static inline bool
handle_wire_generic_command(const wire_generic_command command);

static inline void handle_bulk_byte_transfer(const uint8_t command);
static inline void handle_bulk_clock_ticks_advance(const uint8_t command);
static inline void handle_bulk_bit_transfer(const uint8_t command);
static inline void handle_set_pullup(const uint8_t command);
static inline void handle_configure_peripherals(const uint8_t command);
static inline void handle_set_speed(const uint8_t command);
static inline void handle_configuration(const uint8_t command);
static inline void handle_pic_command_write_and_read_commands(void);
static inline void handle_pic_command_write_and_read_bits(void);
static inline void handle_pic_command_write(void);
static inline void handle_pic_command(const pic_command command);
static inline void handle_smps_command(const smps_command command);

static inline void handle_setup_pwm(void);
static inline void handle_clear_pwm(void);
static inline void handle_read_adc_one_shot(void);
static inline void handle_read_adc_continuously(void);
static inline void handle_frequency_measurement(void);
static inline int handle_bitbang_command(const bitbang_command command);

static void read_and_transmit_adc_measurement(void);

static void send_bits(const uint8_t value, const size_t count);

static void pic24_send_six_payload(const uint8_t *payload);
static inline void pic24_send_nop_opcode(void);
static inline void pic614_read(const uint8_t value);
static inline void pic614_write(const uint8_t command, const uint8_t data_low,
                                const uint8_t data_high);
static inline void pic416_read(const uint8_t value);
static void pic416_write(const uint8_t command, const uint8_t data_low,
                         const uint8_t data_high);
static void pic424_read(void);
static void pic424_write(const uint8_t *payload, const size_t nops);

#define R3WMOSI_TRIS BP_MOSI_DIR
#define R3WCLK_TRIS BP_CLK_DIR
#define R3WMISO_TRIS BP_MISO_DIR
#define R3WCS_TRIS BP_CS_DIR

#define R3WMOSI BP_MOSI
#define R3WCLK BP_CLK
#define R3WMISO BP_MISO
#define R3WCS BP_CS

static void binary_io_self_test(bool jumper_test);
static void reset_state(void);

#define BINARY_IO_2_WIRES 0
#define BINARY_IO_3_WIRES 1

typedef struct {
  uint8_t wires : 1;
  uint8_t pic_mode : 2;
  uint8_t reserved : 5;
} binary_io_state_t;

static binary_io_state_t io_state = {0};

/*
Bitbang is like a player piano or bitmap. The 1 and 0 represent the pins.
So for the four Bus Pirate pins we use the the bits as follows:
COMMAND|POWER|PULLUP|AUX|CS|MISO|CLK|MOSI.

The Bus pirate also responds to each write with a read byte showing the current
state of the pins.

The bits control the state of each of those pins when COMMAND=1.
When COMMAND=0 then up to 127 command codes can be entered on the lower bits.
0x00 resets the Bus Pirate to bitbang mode.

Data:
1xxxxxxx //COMMAND|POWER|PULLUP|AUX|MOSI|CLK|MISO|CS

Commands:
00000000 //Reset to raw BB mode, get raw BB version string
00000001 //enter rawSPI mode
00000010 //enter raw I2C mode
00000011 //enter raw UART mode
00000100 // enter raw 1-wire
00000101 //enter raw wire mode
00000110 // enter openOCD
00000111 // pic programming mode
00001111 //reset, return to user terminal
00010000 //short self test
00010001 //full self test with jumpers
00010010 // setup PWM
00010011 // clear PWM
00010100 // ADC measurement

// Added JM  Only with BP4
00010101 // ADC ....
00010110 // ADC Stop
00011000 // XSVF Player
// End added JM
//
010xxxxx //set input(1)/output(0) pin state (returns pin read)
 */

void send_binary_io_mode_identifier(void) { MSG_BBIO_MODE_IDENTIFIER; }

void enter_binary_bitbang_mode(void) {
  bp_enable_mode_led();
  reset_state();
  send_binary_io_mode_identifier();

  for (;;) {
    uint8_t input_byte = user_serial_read_byte();

    if ((input_byte & 0b10000000) == 0) {
      if (handle_bitbang_command((bitbang_command)input_byte))
          break;
    } else {
      user_serial_transmit_character(bitbang_pin_state_set(input_byte));
    }
  }
}

int handle_bitbang_command(const bitbang_command command) {
  switch (command) {
  case BITBANG_COMMAND_RESET:
    send_binary_io_mode_identifier();
    break;

  case BITBANG_COMMAND_SPI:
#if defined(BP_ENABLE_SPI_SUPPORT)
    reset_state();
    spi_enter_binary_io();
#endif /* BP_ENABLE_SPI_SUPPORT */
    reset_state();
    send_binary_io_mode_identifier();
    break;

  case BITBANG_COMMAND_I2C:
#if defined(BP_ENABLE_I2C_SUPPORT)
    reset_state();
    binary_io_enter_i2c_mode();
#endif /* BP_ENABLE_I2C_SUPPORT */
    reset_state();
    send_binary_io_mode_identifier();
    break;

  case BITBANG_COMMAND_UART:
#if defined(BP_ENABLE_UART_SUPPORT)
    reset_state();
    binUART();
#endif /* BP_ENABLE_UART_SUPPORT */
    reset_state();
    send_binary_io_mode_identifier();
    break;

  case BITBANG_COMMAND_1WIRE:
#if defined(BP_ENABLE_1WIRE_SUPPORT)
    reset_state();
    binary_io_enter_1wire_mode();
#endif /* BP_ENABLE_1WIRE_SUPPORT */
    reset_state();
    send_binary_io_mode_identifier();
    break;

  case BITBANG_COMMAND_RAW_WIRE:
    reset_state();
    binary_io_raw_wire_mode_handler();
    reset_state();
    send_binary_io_mode_identifier();
    break;

  case BITBANG_COMMAND_OPENOCD:
#if defined(BP_JTAG_OPENOCD_SUPPORT)
    reset_state();
    binOpenOCD();
#endif /* BP_JTAG_OPENOCD_SUPPORT */
    reset_state();
    send_binary_io_mode_identifier();
    break;

  case BITBANG_COMMAND_PIC:
#if defined(BP_ENABLE_PIC_SUPPORT)
    reset_state();
    binary_io_enter_pic_mode();
#endif /* BP_ENABLE_PIC_SUPPORT */
    reset_state();
    send_binary_io_mode_identifier();
    break;

  case BITBANG_COMMAND_RETURN_TO_TERMINAL:
    REPORT_IO_SUCCESS();
    bp_disable_mode_led();
    user_serial_wait_transmission_done();
#if defined(BUSPIRATEV4)
    reset_state();
    return -1;
#else
    __asm volatile("RESET");
#endif /* BUSPIRATEV4 */
    break;

  case BITBANG_COMMAND_SHORT_SELF_TEST:
    binary_io_self_test(false);
    break;

  case BITBANG_COMMAND_FULL_SELF_TEST:
    binary_io_self_test(true);
    break;

  case BITBANG_COMMAND_SETUP_PWM:
    handle_setup_pwm();
    break;

  case BITBANG_COMMAND_CLEAR_PWM:
    handle_clear_pwm();
    break;

  case BITBANG_COMMAND_ADC_ONE_SHOT:
    handle_read_adc_one_shot();
    break;

  case BITBANG_COMMAND_ADC_CONTINUOUS:
    handle_read_adc_continuously();
    break;

  case BITBANG_COMMAND_FREQUENCY_COUNT:
    handle_frequency_measurement();
    break;

  case BITBANG_COMMAND_JTAG_XSVF:
#ifdef BUSPIRATEV4
    bp_enable_voltage_regulator();
    MSG_XSV1_MODE_IDENTIFIER;
    jtag();
#else
    REPORT_IO_FAILURE();
#endif /* BUSPIRATEV4 */
    break;

  default:
    if ((command & 0b11100000) == 0b01000000) {
      user_serial_transmit_character(bitbang_pin_direction_set(command));
    } else {
      REPORT_IO_FAILURE();
    }
    break;
  }
  return 0;
}

void reset_state(void) {
  bp_disable_3v3_pullup();
  bitbang_pin_direction_set(0xFF);
  bitbang_pin_state_set(0x00);
}

uint8_t bitbang_pin_direction_set(const uint8_t direction_mask) {

  /* Set directions. */

  BP_AUX0_DIR = (direction_mask & 0b00010000) ? INPUT : OUTPUT;
  BP_MOSI_DIR = (direction_mask & 0b00001000) ? INPUT : OUTPUT;
  BP_CLK_DIR = (direction_mask & 0b00000100) ? INPUT : OUTPUT;
  BP_MISO_DIR = (direction_mask & 0b00000010) ? INPUT : OUTPUT;
  BP_CS_DIR = (direction_mask & 0b00000001) ? INPUT : OUTPUT;

  /* Wait a bit. */

  bp_delay_us(5);

  /* Return current state. */

  return ((BP_AUX0 == HIGH) ? 0b00010000 : 0b00000000) |
         ((BP_MOSI == HIGH) ? 0b00001000 : 0b00000000) |
         ((BP_CLK == HIGH) ? 0b00000100 : 0b00000000) |
         ((BP_MISO == HIGH) ? 0b00000010 : 0b00000000) |
         ((BP_CS == HIGH) ? 0b00000001 : 0b00000000);
}

uint8_t bitbang_pin_state_set(const uint8_t state_mask) {

  /* Set state. */

  bp_set_voltage_regulator_state((state_mask & 0b01000000) ? ON : OFF);
  bp_set_pullup_state((state_mask & 0b00100000) ? ON : OFF);

  BP_AUX0 = (state_mask & 0b00010000) ? HIGH : LOW;
  BP_MOSI = (state_mask & 0b00001000) ? HIGH : LOW;
  BP_CLK = (state_mask & 0b00000100) ? HIGH : LOW;
  BP_MISO = (state_mask & 0b00000010) ? HIGH : LOW;
  BP_CS = (state_mask & 0b00000001) ? HIGH : LOW;

  /* Wait a bit. */

  bp_delay_us(5);

  /* Return current state. */

  return ((BP_AUX0 == HIGH) ? 0b00010000 : 0b00000000) |
         ((BP_MOSI == HIGH) ? 0b00001000 : 0b00000000) |
         ((BP_CLK == HIGH) ? 0b00000100 : 0b00000000) |
         ((BP_MISO == HIGH) ? 0b00000010 : 0b00000000) |
         ((BP_CS == HIGH) ? 0b00000001 : 0b00000000);
}

void binary_io_self_test(const bool jumper_test) {
  volatile uint16_t tick = 0;

  uint8_t errors = perform_selftest(false, jumper_test);
  bp_set_mode_led_state(errors > 0);
  user_serial_transmit_character(errors);

  for (;;) {
    if (user_serial_ready_to_read()) {
      uint8_t input_byte = user_serial_read_byte();
      if (input_byte != 0xFF) {
        user_serial_transmit_character(input_byte + errors);
      } else {
        user_serial_transmit_character(0x01);
        return;
      }
    }

    if (errors == 0) {
      if (tick == 0) {
        tick = 0xFFFF;
        bp_toggle_mode_led();
      }

      tick--;
    }
  }
}

void bp_binary_io_peripherals_set(unsigned char inByte) {
  bp_set_voltage_regulator_state((inByte & 0b00001000) == 0b00001000);
  bp_set_pullup_state((inByte & 0b00000100) == 0b00000100);

  BP_AUX0_DIR = OUTPUT;
  BP_AUX0 = (inByte & 0b00000010) ? ON : OFF;

  // CS pin, follows HiZ setting
  if (inByte & 0b1) {
    if (mode_configuration.high_impedance == YES) {
      IODIR |= CS; // CS input in open drain mode
    } else {
      IOLAT |= CS;    // CS high
      IODIR &= (~CS); // CS output
    }
  } else {
    IOLAT &= (~CS); // BP_CS=0;
    IODIR &= (~CS); // CS output
  }
}

#if defined(BUSPIRATEV4)

// checks if voltage is present on VUEXTERN
bool bp_binary_io_pullup_control(uint8_t control_byte) {

  if (mode_configuration.high_impedance == NO) {
    return false;
  }

  /* Disable both pull-ups. */
  bp_disable_3v3_pullup();
  bp_delay_ms(2);

  /* Turn on the ADC. */
  bp_enable_adc();

  /* Is there already an external voltage? */
  bool has_voltage = bp_read_adc(BP_ADC_VPU) > 0x0100;

  /* Turn off the ADC. */
  bp_disable_adc();

  if (has_voltage) {
    return false;
  }

  switch (control_byte) {
  case 0x51:
    /* Turn on the +3.3v pull-up. */
    bp_enable_3v3_pullup();
    break;

  case 0x52:
    /* Turn on the +5v pull-up. */
    bp_enable_5v0_pullup();
    break;

  default:
    /* Turn off both pull-ups. */
    bp_disable_3v3_pullup();
    break;
  }

  return true;
}

#endif /* BUSPIRATEV4 */

void binary_io_raw_wire_mode_handler(void) {
  mode_configuration.high_impedance = YES;
  mode_configuration.little_endian = NO;
  mode_configuration.speed = 1;
  mode_configuration.numbits = 8;

  io_state.wires = BINARY_IO_2_WIRES;
  io_state.pic_mode = PIC_MODE_614;

  bitbang_setup(2, BITBANG_SPEED_MAXIMUM);
  R3WMOSI_TRIS = OUTPUT;
  R3WCLK_TRIS = OUTPUT;
  R3WMISO_TRIS = INPUT;
  bitbang_set_cs(HIGH);

  MSG_RAW_MODE_IDENTIFIER;

  bool keep_looping = true;

  while (keep_looping) {
    uint8_t input_byte = user_serial_read_byte();

    switch ((io_command_group)(input_byte >> 4)) {
    case IO_COMMAND_GROUP_GENERIC:
      keep_looping =
          handle_wire_generic_command((wire_generic_command)input_byte);
      continue;

    case IO_COMMAND_GROUP_BULK_BYTE_TRANSFER:
      handle_bulk_byte_transfer(input_byte);
      break;

    case IO_COMMAND_GROUP_BULK_CLOCK_TICKS_ADVANCE:
      handle_bulk_clock_ticks_advance(input_byte);
      break;

    case IO_COMMAND_GROUP_BULK_BIT_TRANSFER:
      handle_bulk_bit_transfer(input_byte);
      break;

    case IO_COMMAND_GROUP_PIC:
      handle_pic_command((pic_command)input_byte);
      break;

    case IO_COMMAND_GROUP_SET_PULLUP:
      handle_set_pullup(input_byte);
      break;

    case IO_COMMAND_GROUP_CONFIGURE_PERIPHERALS:
      handle_configure_peripherals(input_byte);
      break;

    case IO_COMMAND_GROUP_SET_SPEED:
      handle_set_speed(input_byte);
      break;

    case IO_COMMAND_GROUP_CONFIGURATION:
      handle_configuration(input_byte);
      break;

    case IO_COMMAND_GROUP_SMPS:
      handle_smps_command((smps_command)input_byte);
      break;

    default:
      REPORT_IO_FAILURE();
      break;
    }
  }
}

void send_bits(const uint8_t value, const size_t count) {
  uint8_t bits = value;

  for (size_t index = 0; index < count; index++) {
    bitbang_write_bit((bits & 0x01) ? HIGH : LOW);
    bits >>= 1;
  }
}

void pic614_read(const uint8_t value) {
  send_bits(value, 6);

  user_serial_transmit_character(bitbang_read_value());
  user_serial_transmit_character(bitbang_read_value());
}

void pic614_write(const uint8_t command, const uint8_t data_low,
                  const uint8_t data_high) {
  send_bits(command, 6);

  if ((command & 0x80) == 0x00) {
    bitbang_write_value(data_low);
    bitbang_write_value(data_high);
  }
}

void pic416_read(const uint8_t value) {
  send_bits(value, 4);

  bitbang_read_value();
  user_serial_transmit_character(bitbang_read_value());
}

void pic416_write(const uint8_t command, const uint8_t data_low,
                  const uint8_t data_high) {
  uint8_t delay = (command >> 6) & 0b00000011;
  uint8_t id = command;

  for (size_t index = 0; index < 4; index++) {
    if (index == 3 && (delay > 0)) {
      bitbang_set_clk(HIGH);
      bp_delay_ms(delay);
      bitbang_set_clk(LOW);
      continue;
    }

    bitbang_write_bit((id & 0x01) ? HIGH : LOW);
    id >>= 1;
  }

  bitbang_write_value(data_low);
  bitbang_write_value(data_high);
}

void pic24_send_six_payload(const uint8_t *payload) {
  /* Send SIX command. */
  bitbang_write_bit(LOW);
  bitbang_write_bit(LOW);
  bitbang_write_bit(LOW);
  bitbang_write_bit(LOW);

  /* Send payload. */
  bitbang_write_value(*payload++);
  bitbang_write_value(*payload++);
  bitbang_write_value(*payload++);
}

void pic24_send_nop_opcode(void) {
  pic24_send_six_payload(&PIC24_NOP_PAYLOAD[0]);
}

void pic424_write(const uint8_t *payload, const size_t nops) {
  pic24_send_six_payload(payload);

  /* Insert NOPs. */
  for (size_t counter = 0; counter < nops; counter++) {
    pic24_send_nop_opcode();
  }
}

void pic424_read(void) {
  /* Send REGOUT command. */
  bitbang_write_bit(HIGH);
  bitbang_write_bit(LOW);
  bitbang_write_bit(LOW);
  bitbang_write_bit(LOW);

  bitbang_write_value(0x00);

  uint8_t value = bitbang_read_value();
  user_serial_transmit_character(bitbang_read_value());
  user_serial_transmit_character(value);

  /* Send two NOPs after a read. */
  pic24_send_nop_opcode();
  pic24_send_nop_opcode();
}

bool handle_wire_generic_command(const wire_generic_command command) {
  switch (command) {
  case IO_COMMAND_EXIT:
    /* @todo: Cleanup? */
    return false;

  case IO_COMMAND_REPORT_ID_STRING:
    MSG_RAW_MODE_IDENTIFIER;
    break;

  case IO_COMMAND_SEND_I2C_START_BIT:
    bitbang_i2c_start(BITBANG_I2C_START_ONE_SHOT);
    REPORT_IO_SUCCESS();
    break;

  case IO_COMMAND_SEND_I2C_STOP_BIT:
    bitbang_i2c_stop();
    REPORT_IO_SUCCESS();
    break;

  case IO_COMMAND_CS_LOW:
    bitbang_set_cs(LOW);
    REPORT_IO_SUCCESS();
    break;

  case IO_COMMAND_CS_HIGH:
    bitbang_set_cs(HIGH);
    REPORT_IO_SUCCESS();
    break;

  case IO_COMMAND_BITBANG_READ_BYTE: {
    uint16_t value = (io_state.wires == BINARY_IO_2_WIRES)
                         ? bitbang_read_value()
                         : bitbang_read_with_write(0xFF);
    if (mode_configuration.little_endian == YES) {
      value = bp_reverse_integer(value, mode_configuration.numbits);
    }
    user_serial_transmit_character(value & 0xFF);
    break;
  }

  case IO_COMMAND_BITBANG_READ_BIT:
    user_serial_transmit_character(bitbang_read_bit());
    break;

  case IO_COMMAND_PEEK_INPUT_BIT:
    user_serial_transmit_character(bitbang_read_miso());
    break;

  case IO_COMMAND_CLOCK_TICK:
    bitbang_advance_clock_ticks(1);
    REPORT_IO_SUCCESS();
    break;

  case IO_COMMAND_CLOCK_LOW:
    bitbang_set_clk(LOW);
    REPORT_IO_SUCCESS();
    break;

  case IO_COMMAND_CLOCK_HIGH:
    bitbang_set_clk(HIGH);
    REPORT_IO_SUCCESS();
    break;

  case IO_COMMAND_DATA_LOW:
    bitbang_set_mosi(LOW);
    REPORT_IO_SUCCESS();
    break;

  case IO_COMMAND_DATA_HIGH:
    bitbang_set_mosi(HIGH);
    REPORT_IO_SUCCESS();
    break;

  default:
    REPORT_IO_FAILURE();
    break;
  }

  return true;
}

void handle_bulk_byte_transfer(const uint8_t command) {
  size_t bytes = (command & 0x0F) + 1;
  REPORT_IO_SUCCESS();

  for (size_t counter = 0; counter < bytes; counter++) {
    uint16_t value = user_serial_read_byte();
    if (mode_configuration.little_endian == YES) {
      value = bp_reverse_integer(value, mode_configuration.numbits);
    }

    if (io_state.wires == BINARY_IO_2_WIRES) {
      bitbang_write_value(value & 0xFF);
      REPORT_IO_SUCCESS();
    } else {
      value = bitbang_read_with_write(value & 0xFF);
      if (mode_configuration.little_endian == YES) {
        value = bp_reverse_integer(value, mode_configuration.numbits);
      }
      bitbang_write_value(value & 0xFF);
    }
  }
}

void handle_bulk_clock_ticks_advance(const uint8_t command) {
  bitbang_advance_clock_ticks((command & 0x0F) + 1);
  REPORT_IO_SUCCESS();
}

void handle_bulk_bit_transfer(const uint8_t command) {
  size_t bits = (command & 0x0F) + 1;
  REPORT_IO_SUCCESS();

  uint8_t byte = user_serial_read_byte();
  for (size_t counter = 0; counter < bits; counter++) {
    bitbang_write_bit((command & 0x80) ? HIGH : LOW);
    byte <<= 1;
  }

  REPORT_IO_SUCCESS();
}

void handle_set_pullup(const uint8_t command) {
#if defined(BUSPIRATEV4)
  user_serial_transmit_character(bp_binary_io_pullup_control(command));
#else
  REPORT_IO_FAILURE();
#endif /* BUSPIRATEV4 */
}

void handle_configure_peripherals(const uint8_t command) {
  bp_binary_io_peripherals_set(command);
  REPORT_IO_SUCCESS();
}

void handle_set_speed(const uint8_t command) {
  mode_configuration.speed = command & 0b00000011;
  bitbang_setup(io_state.wires == BINARY_IO_2_WIRES ? 2 : 3,
                mode_configuration.speed);
  bitbang_set_cs(HIGH);
  REPORT_IO_SUCCESS();
}

void handle_configuration(const uint8_t command) {
  mode_configuration.high_impedance = (command & 0b00001000) ? YES : NO;
  io_state.wires =
      (command & 0b00000100) ? BINARY_IO_3_WIRES : BINARY_IO_2_WIRES;
  mode_configuration.little_endian = (command & 0b00000010) ? YES : NO;
  bitbang_setup(io_state.wires == BINARY_IO_2_WIRES ? 2 : 3,
                mode_configuration.speed);
  bitbang_set_cs(HIGH);
  REPORT_IO_SUCCESS();
}

void handle_pic_command(const pic_command command) {
  switch (command) {

  case PIC_COMMAND_SET_MODE: {
    uint8_t value = user_serial_read_byte();
    if (((pic_mode)value >= PIC_MODE_COUNT) ||
        ((pic_mode)value == PIC_MODE_INVALID)) {
      REPORT_IO_FAILURE();
    } else {
      io_state.pic_mode = value;
      REPORT_IO_SUCCESS();
    }
    break;
  }

  case PIC_COMMAND_WRITE:
    handle_pic_command_write();
    break;

  case PIC_COMMAND_WRITE_AND_READ_BITS:
    handle_pic_command_write_and_read_bits();
    break;

  case PIC_COMMAND_WRITE_AND_READ_COMMANDS:
    handle_pic_command_write_and_read_commands();
    break;

  default:
    REPORT_IO_FAILURE();
    break;
  }
}

void handle_pic_command_write(void) {
  if ((io_state.pic_mode != PIC_MODE_416) &&
      (io_state.pic_mode != PIC_MODE_424)) {
    REPORT_IO_FAILURE();
    return;
  }

  size_t commands_count =
      (user_serial_read_byte() * (io_state.pic_mode == PIC_MODE_416 ? 3 : 4));
  for (size_t offset = 0; offset < commands_count; offset++) {
    bus_pirate_configuration.terminal_input[offset] = user_serial_read_byte();
  }

  if (io_state.pic_mode == PIC_MODE_416) {
    for (size_t offset = 0; offset < commands_count; offset += 3) {
      pic416_write(bus_pirate_configuration.terminal_input[offset],
                   bus_pirate_configuration.terminal_input[offset + 1],
                   bus_pirate_configuration.terminal_input[offset + 2]);
    }

    REPORT_IO_SUCCESS();
    return;
  }

  for (size_t offset = 0; offset < commands_count; offset += 4) {
    pic424_write(&bus_pirate_configuration.terminal_input[offset],
                 bus_pirate_configuration.terminal_input[offset + 3]);
  }

  REPORT_IO_SUCCESS();
}

void handle_pic_command_write_and_read_bits(void) {
  if ((io_state.pic_mode != PIC_MODE_416) &&
      (io_state.pic_mode != PIC_MODE_424)) {
    REPORT_IO_FAILURE();
    return;
  }

  uint8_t commands_count = user_serial_read_byte();

  if (io_state.pic_mode == PIC_MODE_416) {
    uint8_t command = user_serial_read_byte();

    for (size_t counter = 0; counter < commands_count; counter++) {
      send_bits(command, 4);

      bitbang_read_value();
      user_serial_transmit_character(bitbang_read_value());
    }

    return;
  }

  for (size_t counter = 0; counter < commands_count; counter++) {
    const uint8_t *payload_pointer = &PIC24_WRITE_AND_READ_PAYLOAD[0];

    pic424_write(payload_pointer, 2);
    pic424_read();
    pic424_write(payload_pointer + 3, 2);

    pic424_write(payload_pointer + 6, 2);
    pic424_read();
    pic424_write(payload_pointer + 9, 2);
    pic424_read();
  }
}

void handle_pic_command_write_and_read_commands(void) {
  uint8_t write_commands = user_serial_read_byte();
  uint8_t read_commands = user_serial_read_byte();
  size_t commands = 0;

  switch (io_state.pic_mode) {
  case PIC_MODE_424:
    commands = (write_commands * 5) + read_commands;
    break;

  case PIC_MODE_416:
  case PIC_MODE_614:
    commands = (write_commands * 4) + (read_commands * 2);
    break;

  default:
    REPORT_IO_FAILURE();
    return;
  }

  for (size_t counter = 0; counter < commands; counter++) {
    bus_pirate_configuration.terminal_input[counter] = user_serial_read_byte();
  }

  if (read_commands != 0) {
    REPORT_IO_SUCCESS();
  }

  size_t index = 0;
  while (index < commands) {
    switch (bus_pirate_configuration.terminal_input[index]) {
    case PIC_USER_COMMAND_WRITE:
      switch (io_state.pic_mode) {
      case PIC_MODE_614:
        pic614_write(bus_pirate_configuration.terminal_input[index + 1],
                     bus_pirate_configuration.terminal_input[index + 2],
                     bus_pirate_configuration.terminal_input[index + 3]);
        index += 4;
        break;

      case PIC_MODE_416:
        pic416_write(bus_pirate_configuration.terminal_input[index + 1],
                     bus_pirate_configuration.terminal_input[index + 2],
                     bus_pirate_configuration.terminal_input[index + 3]);
        index += 4;
        break;

      case PIC_MODE_424:
        pic424_write(&bus_pirate_configuration.terminal_input[index + 1],
                     bus_pirate_configuration.terminal_input[index + 4] & 0x0F);
        index += 5;
        break;

      default:
        /* Should be already covered by an earlier check. */
        REPORT_IO_FAILURE();
        return;
      }
      break;

    case PIC_USER_COMMAND_READ:
      switch (io_state.pic_mode) {
      case PIC_MODE_614:
        pic614_read(bus_pirate_configuration.terminal_input[index + 1]);
        index += 2;
        break;

      case PIC_MODE_416:
        pic416_read(bus_pirate_configuration.terminal_input[index + 1]);
        index += 2;
        break;

      case PIC_MODE_424:
        pic424_read();
        index++;
        break;

      default:
        /* Should be already covered by an earlier check. */
        REPORT_IO_FAILURE();
        return;
      }
      break;

    default:
      index++;
      break;
    }
  }

  if (read_commands == 0) {
    REPORT_IO_SUCCESS();
  }
}

void handle_smps_command(const smps_command command) {
#if defined(BP_ENABLE_SMPS_SUPPORT)
  switch (command) {
  case SMPS_COMMAND_GET_OUTPUT_VOLTAGE:
    smps_adc();
    break;

  case SMPS_COMMAND_STOP:
    smps_stop();
    REPORT_IO_SUCCESS();
    break;

    /*
     * SMPS_COMMAND_START is used here as an alias to the first value that can
     * trigger a switch on event for the SMPS board.
     */
  case SMPS_COMMAND_START:
  default: {
    uint16_t output_voltage = ((((uint16_t)((uint8_t)command) & 0x0F)) << 8) |
                              ((uint16_t)user_serial_read_byte());
    smps_start(output_voltage);
    REPORT_IO_SUCCESS();
    break;
  }
  }
#else
  (void)command;
  REPORT_IO_FAILURE();
#endif /* BP_ENABLE_SMPS_SUPPORT */
}

void handle_clear_pwm(void) {
  /*
   * T2CON - TIMER 2 CONTROL REGISTER
   *
   * MSB
   * 0-0------000-0-
   * | |      ||| |
   * | |      ||| +---- TCS:   External clock from pin.
   * | |      ||+------ T32:   TIMER2 is not bound with TIMER3 for 32 bit mode.
   * | |      ++------- TCKPS: 1:1 Prescaler.
   * | +--------------- TSIDL: Continue module operation in idle mode.
   * +----------------- TON:   Timer OFF.
   */
  T2CON = 0x0000;

#if defined(BUSPIRATEV4)
  /*
   * OC5CON1 - OUTPUT COMPARE 5 CONTROL REGISTER 1
   *
   * MSB
   * --0000--0--00000
   *   ||||  |  |||||
   *   ||||  |  ||+++--- OCM:      Output compare is disabled.
   *   ||||  |  |+------ TRIGMODE: TRIGSTAT is only cleared by software.
   *   ||||  |  +------- OCFLT0:   No PWM fault condition.
   *   ||||  +---------- ENFLT0:   Fault 0 input disabled.
   *   |+++------------- OCTSEL:   Output compare with Timer 2.
   *   +---------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#else
  /*
   * OC5CON - OUTPUT COMPARE 5 CONTROL REGISTER
   *
   * MSB
   * --0--------00000
   *   |        |||||
   *   |        ||+++--- OCM:    Output compare is disabled.
   *   |        |+------ OCTSEL: Output compare with Timer 2.
   *   |        +------- OCFLT:  No PWM fault condition.
   *   +---------------- OCSIDL: Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#endif /* BUSPIRATEV4 */

  /* Remove output from AUX pin. */
  BP_AUX_RPOUT = 0x00;

  REPORT_IO_SUCCESS();
}

void read_and_transmit_adc_measurement(void) {
  uint16_t value = bp_read_adc(BP_ADC_PROBE);
  user_serial_transmit_character((value >> 8) & 0xFF);
  user_serial_transmit_character(value & 0xFF);
}

void handle_read_adc_one_shot(void) {
  bp_enable_adc();
  read_and_transmit_adc_measurement();
  bp_disable_adc();
}

void handle_read_adc_continuously(void) {
  bool should_loop = true;

  bp_enable_adc();
  do {
    read_and_transmit_adc_measurement();
    if (user_serial_ready_to_read()) {
      user_serial_read_byte();
      should_loop = false;
    }
  } while (should_loop);
  bp_disable_adc();
}

void handle_frequency_measurement(void) {
  uint32_t frequency = bp_measure_frequency();

  user_serial_transmit_character((frequency >> 24) & 0xFF);
  user_serial_transmit_character((frequency >> 16) & 0xFF);
  user_serial_transmit_character((frequency >> 8) & 0xFF);
  user_serial_transmit_character(frequency & 0xFF);
}

void handle_setup_pwm(void) {
  /*
   * T2CON - TIMER 2 CONTROL REGISTER
   *
   * MSB
   * 0-0------000-0-
   * | |      ||| |
   * | |      ||| +---- TCS:   External clock from pin.
   * | |      ||+------ T32:   TIMER2 is not bound with TIMER3 for 32 bit mode.
   * | |      ++------- TCKPS: 1:1 Prescaler.
   * | +--------------- TSIDL: Continue module operation in idle mode.
   * +----------------- TON:   Timer OFF.
   */
  T2CON = 0x0000;

  /*
   * T4CON - TIMER 4 CONTROL REGISTER
   *
   * MSB
   * 0-0------000-0-
   * | |      ||| |
   * | |      ||| +---- TCS:   External clock from pin.
   * | |      ||+------ T32:   TIMER4 is not bound with TIMER5 for 32 bit mode.
   * | |      ++------- TCKPS: 1:1 Prescaler.
   * | +--------------- TSIDL: Continue module operation in idle mode.
   * +----------------- TON:   Timer OFF.
   */
  T4CON = 0x0000;

#if defined(BUSPIRATEV4)
  /*
   * OC5CON1 - OUTPUT COMPARE 5 CONTROL REGISTER 1
   *
   * MSB
   * --0000--0--00000
   *   ||||  |  |||||
   *   ||||  |  ||+++--- OCM:      Output compare is disabled.
   *   ||||  |  |+------ TRIGMODE: TRIGSTAT is only cleared by software.
   *   ||||  |  +------- OCFLT0:   No PWM fault condition.
   *   ||||  +---------- ENFLT0:   Fault 0 input disabled.
   *   |+++------------- OCTSEL:   Output compare with Timer 2.
   *   +---------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#else
  /*
   * OC5CON - OUTPUT COMPARE 5 CONTROL REGISTER
   *
   * MSB
   * --0--------00000
   *   |        |||||
   *   |        ||+++--- OCM:    Output compare is disabled.
   *   |        |+------ OCTSEL: Output compare with Timer 2.
   *   |        +------- OCFLT:  No PWM fault condition.
   *   +---------------- OCSIDL: Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#endif /* BUSPIRATEV4 */

  BP_AUX_RPOUT = OC5_IO;
  T2CONbits.TCKPS = user_serial_read_byte() & 0b00000011;

  uint16_t duty_cycle = user_serial_read_byte();
  duty_cycle = (duty_cycle << 8) | user_serial_read_byte();
  OC5R = duty_cycle;
  OC5RS = duty_cycle;

#if defined(BUSPIRATEV4)
  /*
   * OC5CON1 - OUTPUT COMPARE 5 CONTROL REGISTER 1
   *
   * MSB
   * --0000--0--00110
   *   ||||  |  |||||
   *   ||||  |  ||+++--- OCM:      PWM ON, Fault pin, OCF5 disabled.
   *   ||||  |  |+------ TRIGMODE: TRIGSTAT is only cleared by software.
   *   ||||  |  +------- OCFLT0:   No PWM fault condition.
   *   ||||  +---------- ENFLT0:   Fault 0 input disabled.
   *   |+++------------- OCTSEL:   Output compare with Timer 2.
   *   +---------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0006;
#else
  /*
   * OC5CON - OUTPUT COMPARE 5 CONTROL REGISTER
   *
   * MSB
   * --0--------00110
   *   |        |||||
   *   |        ||+++--- OCM:    PWM ON, Fault pin, OCF5 disabled.
   *   |        |+------ OCTSEL: Output compare with Timer 2.
   *   |        +------- OCFLT:  No PWM fault condition.
   *   +---------------- OCSIDL: Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0006;
#endif /* BUSPIRATEV4 */

  uint16_t period = user_serial_read_byte();
  period = (period << 8) | user_serial_read_byte();
  PR2 = period;

  T2CONbits.TON = ON;
  REPORT_IO_SUCCESS();
}
