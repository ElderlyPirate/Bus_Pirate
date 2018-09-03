/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
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

/**
 * @file base.h
 *
 * @brief Base functions definition file.
 */

#ifndef BP_BASE_H
#define BP_BASE_H

/**
 * @brief MCU Operating Frequency, in Hz.
 */
#define FCY 16000000UL

#include <libpic30.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xc.h>

#include "configuration.h"
#include "messages.h"

/**
 * @brief Value indicating a bit to be OFF.
 */
#define OFF 0

/**
 * @brief Value indicating a bit to be ON.
 */
#define ON 1

/**
 * @brief Value indicating a pin direction to be OUTPUT.
 */
#define OUTPUT 0

/**
 * @brief Value indicating a pin direction to be INPUT.
 */
#define INPUT 1

/**
 * @brief Value indicating a pin state to be LOW.
 */
#define LOW 0

/**
 * @brief Value indicating a pin state to be HIGH.
 */
#define HIGH 1

/**
 * @brief Value indicating a negative statement.
 */
#define NO 0

/**
 * @brief Value indicating a positive statement.
 */
#define YES 1

/**
 * @brief Returns the most significant 16 bits of the given 32-bits integer.
 *
 * @param[in] value a 32-bits integer whose MSW is required.
 */
#define HI16(value) (((uint32_t)(value) >> 16) & 0xFFFF)

/**
 * @brief Returns the least significant 16 bits of the given 32-bits integer.
 *
 * @param[in] value a 32-bits integer whose LSW is required.
 */
#define LO16(value) ((uint32_t)(value)&0xFFFF)

/**
 * @brief Returns the most significant 8 bits of the given 16-bits integer.
 *
 * @param[in] value a 16-bits integer whose MSB is required.
 */
#define HI8(value) (((uint16_t)(value) >> 8) & 0xFF)

/**
 * @brief Returns the least significant 8 bits of the given 16-bits integer.
 *
 * @param[in] value a 16-bits integer whose LSB is required.
 */
#define LO8(value) ((uint16_t)(value)&0xFF)

/**
 * @brief Returns the given value with only the requested bottom bits masked in.
 *
 * @param[in] value a 8-bits integer to mask.
 * @param[in] bits how many bits to leave, starting from LSB.
 */
#define MASKBOTTOM8(value, bits) (((uint8_t)value) & ((1 << (uint8_t)bits) - 1))

/**
 * @brief Returns the given value with only the requested bottom bits masked in.
 *
 * @param[in] value a 16-bits integer to mask.
 * @param[in] bits how many bits to leave, starting from LSB.
 */
#define MASKBOTTOM16(value, bits)                                              \
  (((uint16_t)value) & ((1 << (uint16_t)bits) - 1))

/**
 * @brief Firmware version string, used at startup and for the 'i' command.
 */
#define BP_FIRMWARE_STRING "Community Firmware v7.1 - goo.gl/gCzQnW "

#ifdef BUSPIRATEV3
#include "hardwarev3.h"
#elif defined(BUSPIRATEV4)
#include "dp_usb/usb_stack_globals.h"
#include "hardwarev4a.h"
#include "onboard_eeprom.h"
#else
#error "No hardware defined in base.h."
#endif /* BUSPIRATEV3 || BUSPIRATEV4 */

/**
 * @brief Current mode configuration settings structure.
 *
 * This is used to let protocol implementations to interact with the Bus Pirate
 * event loop.  Every time a mode is changed (a.k.a. a new protocol gets
 * activated) this structure is cleared.
 */
typedef struct {
  uint8_t buf[16];
  uint8_t alternate_aux : 2;
  uint8_t periodicService : 1;

  /**
   * Values that can be either big or little endian are forced to read as
   * little endian.
   */
  uint8_t little_endian : 1;

  uint8_t high_impedance : 1;

  /**
   * Values are 16-bits wide.
   */
  uint8_t int16 : 1;

  /**
   * Each I/O write from the protocol must be followed by a read operation.
   */
  uint8_t write_with_read : 1;

  /**
   * Reserved bit, keep it set to zero.
   */
  uint8_t reserved : 1;

  uint8_t speed;
  uint8_t numbits;
} __attribute__((packed)) mode_configuration_t;

typedef struct {
  uint16_t num;
  uint16_t repeat;
  uint8_t cmd;
} __attribute__((packed)) command_t;

/**
 * @brief Reverses the bits in the given value and returns it.
 *
 * @param[in] value the value whose bits should be reversed.
 * @param[in] bits width of the value to reverse, in bits.
 *
 * @return the value with reversed bits.
 */
uint16_t bp_reverse_integer(const uint16_t value, const uint8_t bits);

/**
 * @brief Reverses the bits in the given byte and returns it.
 *
 * @param[in] value the byte whose bits should be reversed.
 *
 * @return the value with reversed bits.
 */
uint8_t bp_reverse_byte(const uint8_t value);

/**
 * @brief Reverses the bits in the given word and returns it.
 *
 * @param[in] value the word whose bits should be reversed.
 *
 * @return the value with reversed bits.
 */
uint16_t bp_reverse_word(const uint16_t value);

/**
 * @brief Bring the board to a clean slate shortly before switching to a new
 * operational mode.
 */
void bp_reset_board_state(void);

/**
 * @brief Reads a value from the ADC on the given channel.
 *
 * @warning this function assumes the ADC is already enabled, and will not turn
 *          it on or off.
 *
 * @param[in] channel the channel to read data from.
 * @return the value read from the channel.
 */
uint16_t bp_read_adc(const uint16_t channel);

/**
 * @brief Takes one single ADC measurement and prints it to the serial port.
 */
void bp_adc_probe(void);

/**
 * @brief Takes ADC measurements and prints them to the serial port until a byte
 * is sent to the serial port.
 */
void bp_adc_continuous_probe(void);

/**
 * @brief Prints the given value to the user terminal according to the format
 * settings specified by bus_pirate_configuration_t.display_mode.
 *
 * @param[in] value the value to print to the serial port.
 */
void bp_write_formatted_integer(const uint16_t value);

/**
 * @brief Pauses execution for the given amount of milliseconds.
 *
 * @param[in] milliseconds the amount of milliseconds to wait.
 */
#define bp_delay_ms(milliseconds) __delay_ms(milliseconds)

/**
 * @brief Pauses execution for the given amount of microseconds.
 *
 * @param[in] microseconds the amount of microseconds to wait.
 */
#define bp_delay_us(microseconds) __delay_us(microseconds)

/**
 * @brief Writes the given buffer to the serial port.
 *
 * @param[in] buffer the data to write.
 * @param[in] length how many bytes to write.
 */
void bp_write_buffer(const uint8_t *buffer, const size_t length);

/**
 * Writes the given NULL-terminated string to the serial port.
 *
 * @param[in] string the string to write.
 */
void bp_write_string(const char *string);

/**
 * @brief Writes the given NULL-terminated string to the serial port, followed
 * by a line break.
 *
 * @param[in] string the string to write.
 */
void bp_write_line(const char *string);

/**
 * @brief Writes the given 8-bits value to the serial port in hexadecimal form.
 *
 * @param[in] value the value to write.
 */
void bp_write_hex_byte(const uint8_t value);

/**
 * @brief Writes the given 16-bits value to the serial port in hexadecimal form.
 *
 * @param[in] value the value to write.
 */
void bp_write_hex_word(const uint16_t value);

/**
 * @brief Writes the given 8-bits value to the serial port in binary form.
 *
 * @param[in] value the value to write.
 */
void bp_write_bin_byte(const uint8_t value);

/**
 * @brief Writes the given 8-bits value to the serial port in decimal form.
 *
 * @param[in] value the value to write.
 */
void bp_write_dec_byte(const uint8_t value);

/**
 * @brief Writes the given 16-bits value to the serial port in decimal form.
 *
 * @param[in] value the value to write.
 */
void bp_write_dec_word(const uint16_t value);

/**
 * @brief Writes the given 32-bits value to the serial port in decimal form.
 *
 * @param[in] value the value to write.
 */
void bp_write_dec_dword(const uint32_t value);

/**
 * @brief Writes the given 32-bits value to the serial port in decimal form,
 * using thousand separators.
 *
 * @param[in] value the value to write.
 */
void bp_write_dec_dword_friendly(const uint32_t value);

/**
 * @brief Writes the given ADC reading to the serial port in human-readable
 * form.
 *
 * @param[in] adc the ADC reading to print.
 */
void bp_write_voltage(const uint16_t adc);

/**
 * @brief Reads the lower 16 bits of the given flash memory address.
 *
 * @param[in] page the memory page index.
 * @param[in] address the address inside the memory page.
 *
 * @return the lower 16 bits of the 24 bits word at the given address.
 */
uint16_t bp_read_from_flash(const uint16_t page, const uint16_t address);

/**
 * @brief Writes the given value in hexadecimal format into the user-facing
 * serial port ringbuffer.
 *
 * @param[in] value the value to write into the ringbuffer.
 */
void bp_write_hex_byte_to_ringbuffer(const uint8_t value);

/**
 * @brief Shortcut for writing an empty line to the user-facing serial port.
 */
#define bpBR bp_write_line("")

/**
 * @brief Shortcut for writing a space to the user-facing serial port.
 */
#define bpSP user_serial_transmit_character(' ')

#if defined(BP_JTAG_OPENOCD_SUPPORT)

#define UART_NORMAL_SPEED 34
#define UART_FAST_SPEED 3

#endif /* BP_JTAG_OPENOCD_SUPPORT */

#if defined(BUSPIRATEV3)

extern uint8_t *UART1RXBuf;
extern uint16_t UART1RXToRecv;
extern uint16_t UART1RXRecvd;
extern uint8_t *UART1TXBuf;
extern uint16_t UART1TXSent;
extern uint16_t UART1TXAvailable;

#endif /* BUSPIRATEV3 */

/**
 * @defgroup user_serial User-facing serial port functions.
 * @{
 */

/**
 * @brief Initialise the user-facing serial port.
 */
void user_serial_initialise(void);

/**
 * @brief Checks whether the transmission queue is empty or not.
 *
 * @return YES if the transmission queue is empty, NO otherwise.
 */
bool user_serial_transmit_done(void);

/**
 * @brief Checks whether the reception queue is full or not.
 *
 * @return YES if the reception queue is not full, NO otherwise.
 */
bool user_serial_ready_to_read(void);

/**
 * @brief Clears the user-facing serial port overflow error flag.
 */
void user_serial_clear_overflow(void);

/**
 * @brief Returns the user-facing serial port overflow error flag.
 *
 * @return YES if an overflow error was detected, NO otherwise.
 */
bool user_serial_check_overflow(void);

/**
 * @brief Set the baud rate for the user-facing serial port.
 *
 * @param[in] rate the rate for the serial port, as a frequency counter for the
 * baud rate generator circuitry.
 */
void user_serial_set_baud_rate(const uint16_t rate);

/**
 * @brief Blocks execution until the user-facing serial port transmission queue
 * is empty.
 */
void user_serial_wait_transmission_done(void);

/**
 * @brief Blocks execution until a byte arrives on the user-facing serial port
 * and returns said value.
 *
 * @return the byte read from the serial port.
 */
uint8_t user_serial_read_byte(void);

/**
 * @brief Writes the first available byte from the transmission queue into the
 * serial port transmission buffer register.
 */
void user_serial_process_transmission_interrupt(void);

/**
 * @brief Writes the given character to the user-facing serial port, blocking
 * until the transmission completes.
 *
 * @param[in] character the character to write.
 */
void user_serial_transmit_character(const char character);

/**
 * @}
 */

/**
 * @defgroup user_serial_ringbuffer User-facing serial port ringbuffer
 * functions.
 * @{
 */

/**
 * @brief Sets up the user-facing serial port ringbuffer.
 */
void user_serial_ringbuffer_setup(void);

/**
 * @brief Flushes the user-facing serial port ringbuffer.
 */
void user_serial_ringbuffer_flush(void);

/**
 * @brief Appends the given character to the user-facing serial port ringbuffer,
 * blocking until there is enough space.
 *
 * @param[in] character the character to append.
 */
void user_serial_ringbuffer_append(const char character);

/**
 * @brief Transmits the first available character from the ringbuffer.
 */
void user_serial_ringbuffer_process(void);

/**
 * @}
 */

#endif /* !BP_BASE_H */
