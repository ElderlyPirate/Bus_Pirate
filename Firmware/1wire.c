/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * One wire search code taken from here:
 * http://www.maxim-ic.com/appnotes.cfm/appnote_number/187
 *
 * We claim no copyright on our code, but there may be different licenses for
 * some of the code in this file.
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
 * @file 1wire.c
 *
 * @brief 1-Wire protocol handler implementation file.
 *
 * From https://en.wikipedia.org/wiki/1-Wire :
 *
 * 1-Wire is a device communications bus system designed by Dallas
 * Semiconductor Corp. that provides low-speed data, signaling, and power over
 * a single conductor.  1-Wire is similar in concept to I2C, but with lower
 * data rates and longer range. It is typically used to communicate with small
 * inexpensive devices such as digital thermometers and weather instruments. A
 * network of 1-Wire devices with an associated master device is called a
 * MicroLAN.  One distinctive feature of the bus is the possibility of using
 * only two wires: data and ground.  To accomplish this, 1-Wire devices include
 * an 800 pF capacitor to store charge, and to power the device during periods
 * when the data line is active.
 *
 * @todo
 * * Clean up the search functions, with better documentation.
 * * Have an external table of device names and descriptions.
 * * Expand the device table using the data from
 * http://owfs.org/index.php?page=family-code-list
 * * Implement BP_1WIRE_PRINT_FAMILY_DESCRIPTION on/off changes.
 * * Rename string message identifiers to something more appropriate.
 * * Rig up a circuit to simulate 1-Wire devices for automated tests.
 */

#include "1wire.h"

#ifdef BP_ENABLE_1WIRE_SUPPORT

#include <stdbool.h>
#include <stdint.h>

#include "base.h"
#include "binary_io.h"
#include "proc_menu.h"

extern mode_configuration_t mode_configuration;
extern command_t last_command;

/**
 * @brief The maximum size of the saved devices roster, in entries.
 */
#define MAXIMUM_DEVICES_ROSTER_SIZE 50

/* Check whether the current module settings are valid. */

#if BP_1WIRE_DEVICE_DEV_ROSTER_SLOTS > MAXIMUM_DEVICES_ROSTER_SIZE
#error "BP_1WIRE_DEVICE_DEV_ROSTER_SLOTS too big"
#endif /* BP_1WIRE_DEVICE_DEV_ROSTER_SLOTS > MAXIMUM_DEVICES_ROSTER_SIZE */

/**
 * @brief Size of a 1-Wire ROM number identifier, in bytes.
 */
#define ROM_BYTES_SIZE 8

/**
 * @brief Data line pin assignment.
 */
#define ONEWIRE_DATA_LINE BP_MOSI

/**
 * @brief Data line pin direction assignment.
 */
#define ONEWIRE_DATA_DIRECTION BP_MOSI_DIR

/**
 * @brief Device ID for DS2404 EconoRAM Time Chip.
 */
#define DS2404 0x04

/**
 * @brief Device ID for DS18S20 High-Precision Digital Thermometer.
 */
#define DS18S20 0x10

/**
 * @brief Device ID for DS1822 Econo Digital Thermometer.
 */
#define DS1822 0x22

/**
 * @brief Device ID for DS18B20 Programmable Resolution Digital Thermometer.
 */
#define DS18B20 0x28

/**
 * @brief Device ID for DS2431 1024-bit EEPROM.
 */
#define DS2431 0x2D

/**
 * @brief Binary I/O 1-Wire Action command.
 *
 * This is for further actions dealing with simple operations like bus reset,
 * and so on.
 *
 * Current form is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Set to `0b0000`.</td></tr>
 * <tr><td>`3:0`</td><td>The action identifier:
 * * `0b0000` : BINARY_IO_ONEWIRE_ACTION_EXIT.
 * * `0b0001` : BINARY_IO_ONEWIRE_ACTION_VERSION_STRING.
 * * `0b0010` : BINARY_IO_ONEWIRE_ACTION_BUS_RESET.
 * * `0b0011` : Reserved.
 * * `0b0100` : BINARY_IO_ONEWIRE_ACTION_READ_BYTE.
 * * `0b0101` : Reserved.
 * * `0b0110` : Reserved.
 * * `0b0111` : Reserved.
 * * `0b1000` : BINARY_IO_ONEWIRE_ACTION_ROM_SEARCH_MACRO.
 * * `0b1001` : BINARY_IO_ONEWIRE_ACTION_ALARM_SEARCH_MACRO.
 * * `0b1010` : Reserved.
 * * `0b1011` : Reserved.
 * * `0b1100` : Reserved.
 * * `0b1101` : Reserved.
 * * `0b1110` : Reserved.
 * * `0b1111` : Reserved.
 * </td></tr></table>
 *
 * @see BINARY_IO_ONEWIRE_ACTION_EXIT
 * @see BINARY_IO_ONEWIRE_ACTION_VERSION_STRING
 * @see BINARY_IO_ONEWIRE_ACTION_BUS_RESET
 * @see BINARY_IO_ONEWIRE_ACTION_READ_BYTE
 * @see BINARY_IO_ONEWIRE_ACTION_ROM_SEARCH_MACRO
 * @see BINARY_IO_ONEWIRE_ACTION_ALARM_SEARCH_MACRO
 */
#define BINARY_IO_ONEWIRE_COMMAND_ACTION 0x00

/**
 * @brief Binary I/O 1-Wire Bulk transfer command.
 *
 * Once this command is received by the board, it will read up to the needed
 * amount of bytes from the serial port and write those same bytes to the 1-Wire
 * bus - in the same order as they were received.  After every byte received on
 * the serial port, the Bus Pirate board will output a `SUCCESS` value on the
 * port, signaling that the incoming data has been processed.
 *
 * Current command format is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Set to `0b0001`.</td></tr>
 * <tr><td>`3:0`</td><td>How many bytes to send *minus one*.  The length value
 * range is still from 0 to 15, although passing 0 will trigger a 1 byte
 * transfer, and 15 will trigger a 16 bytes transfer instead.</td></tr></table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>`0b0001xxxx` (where `xxxx` is length - 1).</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr>
 * <tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>Data byte #0.</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr>
 * <tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>Data byte #1.</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr>
 * <tr><td colspan=4><center>...</center></td></tr>
 * <tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>Last data byte.</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr></table>
 */
#define BINARY_IO_ONEWIRE_COMMAND_BULK_TRANSFER 0x01

/**
 * @brief Binary I/O 1-Wire configuration command.
 *
 * Once this command is received by the board, it will configure the 1-Wire
 * parameters to those provided. At present this simply controls whether the
 * bus speed is Standard or Overdrive.
 *
 * Current command format is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Set to `0b0010`.</td></tr>
 * <tr><td>`3:2`</td><td>Reserved, set to `0b00`.</td></tr>
 * <tr><td>`1:0`</td><td>The speed to be configured:
 * * `0b00` : Standard.
 * * `0b01` : Overdrive.
 * * `0b10` : Reserved.
 * * `0b11` : Reserved.</td><tr></table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>`0b001000ss` (where `ss` is the requested speed).</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr></table>
 */
#define BINARY_IO_ONEWIRE_COMMAND_CONFIGURE 0x02

/**
 * @brief Binary I/O 1-Wire Peripherals configuration command.
 *
 * This command will set up the board in a particular fashion to accommodate the
 * needs of the device currently being manipulated.  Once this command is
 * received by the board, it will respond with a SUCCESS value.
 *
 * Current format is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Set to `0b0100`.</td></tr>
 * <tr><td>`3`</td><td>Power ON/OFF.</td></tr>
 * <tr><td>`2`</td><td>Pull-ups ON/OFF.</td></tr>
 * <tr><td>`1`</td><td>AUX line LOW/HIGH.</td></tr>
 * <tr><td>`0`</td><td>CS line LOW/HIGH.</td></tr></table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>`0b0100abcd` (where `a`, `b`, `c`, and `d` are the peripheral
 * configuration flags).</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr></table>
 */
#define BINARY_IO_ONEWIRE_COMMAND_CONFIGURE_PERIPHERALS 0x04

/**
 * @brief Binary I/O 1-Wire Peripherals read command.
 *
 * Available only on Bus Pirate v4.  It currently manipulates the state of the
 * pull-ups on the board.
 *
 * Current format is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Set to `0b0101`.</td></tr>
 * <tr><td>`3:2`</td><td>Reserved, set to `0b00`.</td></tr>
 * <tr><td>`1:0`</td><td>Pull-up voltage:
 * * `0b00` : Pull-ups OFF.
 * * `0b01` : +3V3 pull-up ON.
 * * `0b10` : +5V pull-up ON.
 * * `0b11` : Pull-ups OFF.</td></tr></table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>`0b010100xx` (where `xx` is the desired pull-up voltage).</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr></table>
 */
#define BINARY_IO_ONEWIRE_COMMAND_READ_PERIPHERALS 0x05

/**
 * @brief Binary I/O 1-Wire Action command to exit 1-Wire mode.
 *
 * Current format is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Command type, set to `0b0000` (ACTION).</td></tr>
 * <tr><td>`3:0`</td><td>Action type, set to `0b0000` (EXIT).</td></tr></table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td><center>&rarr;</center></td><td>Bus Pirate</td>
 * <td>`0b00000000`</td></tr>
 * <tr><td>PC</td><td><center>&times;</center></td><td>Bus Pirate</td>
 * <td>The Bus Pirate is now in interactive mode, no response is given.</td>
 * </tr></table>
 */
#define BINARY_IO_ONEWIRE_ACTION_EXIT 0x00

/**
 * @brief Binary I/O 1-Wire Action command to print out the mode version string.
 *
 * Right now the 1-Wire mode command set is identified by the ASCII bytes `1`,
 * `W`, `0`, `1` - standing for 1-Wire protocol version 1.
 *
 * Current format is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Command type, set to `0b0000` (ACTION).</td></tr>
 * <tr><td>`3:0`</td><td>Action type, set to `0b0001` (VERSION).</td></tr>
 * </table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>`0b00000001`</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00110001` (ASCII `1`)<br>
 * `0b00010111` (ASCII `W`)<br>
 * `0b00110000` (ASCII `0`)<br>
 * `0b00110001` (ASCII `1`).</td></tr></table>
 */
#define BINARY_IO_ONEWIRE_ACTION_VERSION_STRING 0x01

/**
 * @brief Binary I/O 1-Wire Action command to perform a 1-Wire bus reset.
 *
 * The board will respond with a SUCCESS value once the bus is reset.
 *
 * Current format is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Command type, set to `0b0000` (ACTION).</td></tr>
 * <tr><td>`3:0`</td><td>Action type, set to `0b0010` (BUS_RESET).</td></tr>
 * </table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>`0b00000010`</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr></table>
 */
#define BINARY_IO_ONEWIRE_ACTION_BUS_RESET 0x02

/**
 * @brief Binary I/O 1-Wire Action command to read a byte from the bus.
 *
 * Current format is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Command type, set to `0b0000` (ACTION).</td></tr>
 * <tr><td>`3:0`</td><td>Action type, set to `0b0100` (READ).</td></tr>
 * </table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>`0b00000010`</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0bxxxxxxxx` (where `xxxxxxxx` is the byte read from the bus).</td></tr>
 * </table>
 */
#define BINARY_IO_ONEWIRE_ACTION_READ_BYTE 0x04

/**
 * @brief Binary I/O 1-Wire Action command to invoke the "ROM search" macro.
 *
 * This action will perform a ROM search action, by enumerating all devices
 * on the bus.  Once a device is found, its information is sent back to the
 * serial port.  A sequence of eight (8) `0xFF` bytes indicates that the search
 * has terminated.  Right after receiving the ROM search action command, the
 * board will return a SUCCESS value indicating that it will start the devices
 * scan.  The search algorithm is described in
 * https://www.maximintegrated.com/en/app-notes/index.mvp/id/187
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Command type, set to `0b0000` (ACTION).</td></tr>
 * <tr><td>`3:0`</td><td>Action type, set to `0b1000` (ROM_SEARCH).</td></tr>
 * </table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>`0b00001000`</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>Device information bytes.</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>Device information end marker:<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`).</td></tr></table>
 */
#define BINARY_IO_ONEWIRE_ACTION_ROM_SEARCH_MACRO 0x08

/**
 * @brief Binary I/O 1-Wire Action command to invoke the "ALARM search" macro.
 *
 * This action will perform an ALARM search action, by enumerating all devices
 * on the bus that are in ALARM state.  Once a device is found, its information
 * is sent back to the serial port.  A sequence of eight (8) `0xFF` bytes
 * indicates that the search has terminated.  Right after receiving the ROM
 * search action command, the board will return a SUCCESS value indicating that
 * it will start the devices scan.  The search algorithm is described in
 * https://www.maximintegrated.com/en/app-notes/index.mvp/id/187
 *
 * Current format is as follows:
 *
 * <table><tr><th>Bits</th><th>Meaning</th></tr>
 * <tr><td>`7:4`</td><td>Command type, set to `0b0000` (ACTION).</td></tr>
 * <tr><td>`3:0`</td><td>Action type, set to `0b1001` (ALARM_SEARCH).</td></tr>
 * </table>
 *
 * Interaction flow is as follows:
 *
 * <table><tr><td>PC</td><td>&rarr;</td><td>Bus Pirate</td>
 * <td>`0b00001001`</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>`0b00000001` (SUCCESS).</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>Device information bytes.</td></tr>
 * <tr><td>PC</td><td>&larr;</td><td>Bus Pirate</td>
 * <td>Device information end marker:<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`)<br>
 * `0b11111111` (`0xFF`).</td></tr></table>
 */
#define BINARY_IO_ONEWIRE_ACTION_ALARM_SEARCH_MACRO 0x09

/**
 * @brief 1-Wire protocol macro identifiers.
 */
typedef enum {
  /** Identifier for the "Dump roster entries" macro entry. */
  MACRO_DUMP_ROSTER = 0x00,

  /** Identifier for the "Read ROM" macro entry. */
  MACRO_READ_ROM = 0x33,

  /** Identifier for the "Match ROM" macro entry. */
  MACRO_MATCH_ROM = 0x55,

  /** Identifier for the "Skip ROM" macro entry. */
  MACRO_SKIP_ROM = 0xCC,

  /** Identifier for the "Search Alarm" macro entry. */
  MACRO_ALARM_SEARCH = 0xEC,

  /** Identifier for the "Search ROM" macro entry. */
  MACRO_SEARCH_ROM = 0xF0
} onewire_macros_t;

/**
 * @brief Sends and receives 1-bit values on/from the bus.
 *
 * This function works both for sending and receiving of a 1-bit value on the
 * bus.  To get a bit value send a logical 1 (or HIGH), this will pulse the
 * line as needed then release the bus and wait a few microseconds before
 * sampling if the 1Wire device has sent any data.
 *
 * @param[in] bit_value the bit to send.
 *
 * @return either the bit that was just sent, or a bit read from the bus.
 */
static bool onewire_internal_bit_io(bool bit_value);

/**
 * @brief Sends and receives 8-bits values on/from the bus.
 *
 * Like onewire_internal_bit_io, this function works for both sending and
 * receiving.  1-Wire time slots are the same for sending and receiving, so only
 * one function is needed to perform both tasks.
 *
 * Sending 0xFF will trigger a read from the bus (as sending HIGH to
 * onewire_internal_bit_io will trigger a 1-bit read).
 *
 * @param[in] byte_value the byte to send to the bus.
 *
 * @return either the byte that was just sent, or a byte read from the bus.
 */
static uint8_t onewire_internal_byte_io(uint8_t byte_value);

/**
 * @brief Internal macro for triggering a byte bus write.
 *
 * @param[in] value the value to write.
 */
#define ONEWIRE_WRITE_BYTE(value)                                              \
  do {                                                                         \
    onewire_internal_byte_io(value);                                           \
  } while (0)

/**
 * @brief Internal macro for triggering a byte bus read.
 *
 * @return the byte read from the bus.
 */
#define ONEWIRE_READ_BYTE() onewire_internal_byte_io(0xFF)

/**
 * @brief Internal macro for triggering a bit bus write.
 *
 * @param[in] value the value to write.
 */
#define ONEWIRE_WRITE_BIT(value)                                               \
  do {                                                                         \
    onewire_internal_bit_io(value);                                            \
  } while (0)

/**
 * @brief Internal macro for triggering a bit bus read.
 *
 * @return the bit read from the bus.
 */
#define ONEWIRE_READ_BIT() onewire_internal_bit_io(ON)

/**
 * @brief Possible results from 1-Wire bus reset.
 */
typedef enum {

  /** Reset was performed successfully. */
  ONEWIRE_BUS_RESET_OK = 0,

  /** A potential short was detected on the bus. */
  ONEWIRE_BUS_RESET_SHORT,

  /** No device was found on the bus. */
  ONEWIRE_BUS_RESET_NO_DEVICE

} onewire_bus_reset_result_t;

/**
 * @brief 1-Wire protocol internal state variables container.
 */
typedef struct {

  /**
   * Current CRC8 value.
   */
  uint8_t crc8;

  /**
   * Last recorded device discrepancy.
   */
  uint8_t last_device_discrepancy;

  /**
   * Last recorded family discrepancy.
   */
  uint8_t last_family_discrepancy;

  /**
   * ROM identifier of the last discovered device.
   */
  uint8_t rom_bytes[ROM_BYTES_SIZE];

  /**
   * How many entries are saved in the device roster.
   */
  uint8_t used_roster_entries;

  /**
   * Device roster slots.
   *
   * Indexed as [roster slot][ROM byte]: each slot holds one full ROM
   * identifier, and there are BP_1WIRE_DEVICE_DEV_ROSTER_SLOTS slots.  The
   * dimensions must be in this order - the roster is filled with a
   * ROM_BYTES_SIZE memcpy per slot (see onewire_run_macro), so declaring it
   * the other way around reads and writes past the end of the array once
   * more than ROM_BYTES_SIZE devices are found (issue #116).
   */
  uint8_t roster_entries[BP_1WIRE_DEVICE_DEV_ROSTER_SLOTS][ROM_BYTES_SIZE];

  /**
   * The command byte to send on the bus.
   */
  uint8_t command_byte;

  /**
   * Current bus data state.
   *
   * Since 1-Wire uses timing for bits transfer, setting the data line high or
   * low has not effect.  Therefore, it is needed to save the desired bus state,
   * and then clock in the proper value during a clock pulse operation.
   */
  uint8_t data_state : 1;

  /**
   * Flag indicating if the last device found was the last one available on
   * the bus.
   */
  uint8_t last_device_flag : 1;

} __attribute__((packed)) onewire_state_t;

/**
 * @brief 1-Wire protocol state.
 */
static onewire_state_t onewire_state = {.command_byte = MACRO_SEARCH_ROM};

/**
 * @brief Performs a 1-Wire bus reset according to the protocol specifications.
 *
 * @return an appropriate state from onewire_bus_reset_result_t describing
 * the operation result.
 */
static onewire_bus_reset_result_t perform_bus_reset(void);

/**
 * @brief 1-wire protocol precalculated CRC table.
 *
 * Taken from https://www.maximintegrated.com/en/app-notes/index.mvp/id/27
 */
static const uint8_t CRC_TABLE[] = {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20,
    0xA3, 0xFD, 0x1F, 0x41, 0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E,
    0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC, 0x23, 0x7D, 0x9F, 0xC1,
    0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
    0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E,
    0x1D, 0x43, 0xA1, 0xFF, 0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5,
    0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07, 0xDB, 0x85, 0x67, 0x39,
    0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
    0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45,
    0xC6, 0x98, 0x7A, 0x24, 0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B,
    0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9, 0x8C, 0xD2, 0x30, 0x6E,
    0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
    0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31,
    0xB2, 0xEC, 0x0E, 0x50, 0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C,
    0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE, 0x32, 0x6C, 0x8E, 0xD0,
    0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
    0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA,
    0x69, 0x37, 0xD5, 0x8B, 0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4,
    0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16, 0xE9, 0xB7, 0x55, 0x0B,
    0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
    0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54,
    0xD7, 0x89, 0x6B, 0x35};

/**
 * @brief Updates the internal CRC8 variable with the given data value.
 *
 * @param[in] value the new byte to update the internal CRC8 variable with.
 * @return the updated internal CRC8 variable value.
 */
static uint8_t update_crc8(const uint8_t value);

/**
 * @brief Looks up the given model identifier.
 *
 * Looks up the given model identifier and checks it against a list of known
 * devices, then prints the model information if a match is found.
 */
static void lookup_device_model(const uint8_t model);

/**
 * @brief Attempts to find the first device on the 1-Wire bus.
 *
 * The returned ROM number is saved inside onewire_state.rom_number if
 * successful.
 *
 * @return true if a device was found, false otherwise.
 */
static bool device_find_first(void);

/**
 * @brief Attempts to find the next device on the 1-Wire bus.
 *
 * The returned ROM number is saved inside onewire_state.rom_number if
 * successful.
 *
 * @return true if a device was found, false otherwise.
 */
static bool device_find_next(void);

/**
 * @brief Discovers the next device in the chain on a 1-Wire bus.
 *
 * The returned ROM number is saved inside onewire_state.rom_number if
 * successful.
 *
 * The search algorithm was derived from
 * https://www.maximintegrated.com/en/app-notes/index.mvp/id/187
 *
 * @return true if a device was found, false otherwise.
 */
static bool perform_device_search(void);

#ifdef BP_1WIRE_LOOKUP_FAMILY_ID

/**
 * @brief Prints the given ROM address information to the screen.
 *
 * @param[in] roster_id the index of the entry in the roster list.
 * @param[in] rom_address the ROM address bytes.
 */
static void print_device_information(const size_t roster_id,
                                     const uint8_t *rom_address);

#endif /* BP_1WIRE_LOOKUP_FAMILY_ID */

/**
 * @brief Sets the data line to the given state.
 *
 * @param[in] state the state to set the data line to.
 */
static inline void onewire_internal_set_data_state(const bool state);

uint16_t onewire_read(void) { return ONEWIRE_READ_BYTE(); }

uint16_t onewire_write(const uint16_t value) {
  ONEWIRE_WRITE_BYTE(value & 0xFF);

  return 0x100;
}

bool onewire_read_bit(void) { return ONEWIRE_READ_BIT(); }

void onewire_clock_pulse(void) { ONEWIRE_WRITE_BIT(onewire_state.data_state); }

uint16_t onewire_data_state(void) { return onewire_state.data_state; }

inline void onewire_data_low(void) { onewire_internal_set_data_state(LOW); }

inline void onewire_data_high(void) { onewire_internal_set_data_state(HIGH); }

void onewire_setup_prepare(void) {
  int speed;

  /* Always start in high-impedance mode. */
  mode_configuration.high_impedance = true;

  /* Assume standard speed. */
  consumewhitechars();
  speed = getint();
  if ((speed > 0) && (speed <= 2)) {
    mode_configuration.speed = speed - 1;
  } else {
    mode_configuration.command_error = NO;
    MSG_1WIRE_SPEED_PROMPT;
    mode_configuration.speed = getnumber(1, 1, 2, 0) - 1;
  }

  /* Clear the saved device roster entries. */
  onewire_state.used_roster_entries = 0;
}

void onewire_setup_execute(void) {
  /* Set up pins. */
  ONEWIRE_DATA_DIRECTION = INPUT;
  ONEWIRE_DATA_LINE = LOW;
}

void print_device_information(const size_t roster_id,
                              const uint8_t *rom_address) {
  size_t index;

  /* Print roster entry counter. */
  bpSP;
  bp_write_dec_byte(roster_id);
  bp_write_string(".");

  /* Print ROM address. */
  for (index = 0; index < ROM_BYTES_SIZE; index++) {
    bp_write_formatted_integer(rom_address[index]);
    bpSP;
  }

#ifdef BP_1WIRE_LOOKUP_FAMILY_ID
  MSG_1WIRE_LOOKUP_ID_HEADER;
  /* Print the device family identifier if known. */
  lookup_device_model(rom_address[0]);
#endif /* BP_1WIRE_LOOKUP_FAMILY_ID */
}

void onewire_run_macro(const uint16_t macro) {
  uint16_t macro_id;

  macro_id = macro;

  /*
   * Check if the macro identifier is indeed a valid device roster index
   * rather than a predefined protocol macro.
   */
  if ((macro_id > 0) && (macro_id < MAXIMUM_DEVICES_ROSTER_SIZE)) {
    size_t rom_index;

    macro_id--;

    if (macro_id >= onewire_state.used_roster_entries) {
      /* Alert the user if the device is not in the roster. */
      MSG_1WIRE_NO_DEVICE;
      return;
    }

    /*
     * Print the device information taken from the roster on the serial port,
     * and send the ROM address of the device over the 1-Wire bus as well.
     */

    MSG_1WIRE_ADDRESS_MACRO_HEADER;
    bp_write_dec_byte(macro + 1);
    bp_write_string(": ");
    for (rom_index = 0; rom_index < ROM_BYTES_SIZE; rom_index++) {
      bp_write_formatted_integer(
          onewire_state.roster_entries[macro_id][rom_index]);
      bpSP;
      ONEWIRE_WRITE_BYTE(onewire_state.roster_entries[macro_id][rom_index]);
    }
    bpBR;
    return;
  }

  /* Run the requested macro. */

  switch (macro_id) {

  case MACRO_DUMP_ROSTER: {
    size_t index;

    MSG_1WIRE_MACRO_MENU_HEADER;
    MSG_1WIRE_MACRO_TABLE_HEADER;

    /*
     * Print roster entries, or let the user know that a search needs to be
     * performed first.
     */

    if (onewire_state.used_roster_entries == 0) {
      MSG_1WIRE_NO_DEVICE;
    } else {
      for (index = 0; index < onewire_state.used_roster_entries; index++) {
        print_device_information(index + 1,
                                 &onewire_state.roster_entries[index][0]);
      }
    }

    MSG_1WIRE_MACRO_LIST;
    break;
  }

  case MACRO_ALARM_SEARCH:
  case MACRO_SEARCH_ROM: {
    bool device_found;
    size_t index;

    onewire_state.command_byte = macro_id;
    if (macro_id == MACRO_ALARM_SEARCH) {
      MSG_1WIRE_ALARM_MACRO_NAME;
    } else {
      MSG_1WIRE_SEARCH_MACRO_NAME;
    }

    MSG_1WIRE_MACRO_TABLE_HEADER;

    /* Find all devices on the bus. */

    index = 0;
    device_found = device_find_first();
    onewire_state.used_roster_entries = 0;

    while (device_found) {

      /* Print device information. */

      print_device_information(index + 1, &onewire_state.rom_bytes[0]);

      /* Save the device if there is enough space. */

      if (index < BP_1WIRE_DEVICE_DEV_ROSTER_SLOTS) {
        memcpy(onewire_state.roster_entries[onewire_state.used_roster_entries],
               onewire_state.rom_bytes, sizeof(onewire_state.rom_bytes));
        onewire_state.used_roster_entries++;
      }

      index++;

      device_found = device_find_next();
    }

    MSG_1WIRE_MACRO_TABLE_TRAILER;
    break;
  }

  case MACRO_READ_ROM: {
    uint8_t device_address[ROM_BYTES_SIZE];
    size_t index;

    onewire_reset();
    MSG_1WIRE_READ_ROM_MACRO_NAME;
    ONEWIRE_WRITE_BYTE(MACRO_READ_ROM);
    for (index = 0; index < ROM_BYTES_SIZE; index++) {
      device_address[index] = ONEWIRE_READ_BYTE();
      bp_write_formatted_integer(device_address[index]);
      bpSP;
    }
    bpBR;
    lookup_device_model(device_address[0]);
    break;
  }

  case MACRO_MATCH_ROM:
    onewire_reset();
    MSG_1WIRE_MATCH_ROM_MACRO_NAME;
    ONEWIRE_WRITE_BYTE(MACRO_MATCH_ROM);
    break;

  case MACRO_SKIP_ROM:
    onewire_reset();
    MSG_1WIRE_SKIP_ROM_MACRO_NAME;
    ONEWIRE_WRITE_BYTE(MACRO_SKIP_ROM);
    break;

  default:
    MSG_UNKNOWN_MACRO_ERROR;
    break;
  }
}

void onewire_pins_state(void) { MSG_1WIRE_PINS_STATE; }

void onewire_reset(void) {
  onewire_bus_reset_result_t reset_result;

  reset_result = perform_bus_reset();
  MSG_1WIRE_BUS_RESET;

  if (reset_result == ONEWIRE_BUS_RESET_OK) {
    BPMSG1185;
    return;
  }

  MSG_WARNING_HEADER;
  if (reset_result == ONEWIRE_BUS_RESET_SHORT) {
    MSG_WARNING_SHORT_OR_NO_PULLUP;
  } else {
    MSG_1WIRE_NO_DEVICE_DETECTED;
  }

  bpBR;
}

onewire_bus_reset_result_t perform_bus_reset(void) {
  onewire_bus_reset_result_t result;

  result = ONEWIRE_BUS_RESET_OK;

  /* Pull the bus line LOW. */

  ONEWIRE_DATA_DIRECTION = INPUT;
  if (mode_configuration.speed == 1) {
    /* AN126: Parameter G */
    bp_delay_us(1);
  }
  ONEWIRE_DATA_LINE = LOW;
  ONEWIRE_DATA_DIRECTION = OUTPUT;

  /*
   * According to specification a minimum of 480us need to be waited before
   * reading the line in standard mode, or no more than 70us for overdrive.
   * AN126: Parameter H
   */
  if (mode_configuration.speed == 0) {
    bp_delay_us(500);
  } else {
    bp_delay_us(70);
  }

  /* Release the bus. */
  ONEWIRE_DATA_DIRECTION = INPUT;
  /* AN126: Parameter I */
  if (mode_configuration.speed == 0) {
    bp_delay_us(65);
  } else {
    bp_delay_us(6);
  }

  /* Read the data line. */
  if (ONEWIRE_DATA_LINE) {
    /* If the data line is still high, no device is available on the bus. */
    result = ONEWIRE_BUS_RESET_NO_DEVICE;
  }

  /* AN126: Parameter J */
  if (mode_configuration.speed == 0) {
    bp_delay_us(500);
  } else {
    bp_delay_us(32);
  }

  /* Read the data line. */
  if (ONEWIRE_DATA_LINE == LOW) {
    /* If the data line was not pulled high now, there is a short on the bus. */
    result = ONEWIRE_BUS_RESET_SHORT;
  }

  return result;
}

#ifdef BP_1WIRE_LOOKUP_FAMILY_ID

void lookup_device_model(const uint8_t model) {
  switch (model) {
  case DS18S20:
    BPMSG1022;
    break;

  case DS18B20:
    BPMSG1023;
    break;

  case DS1822:
    BPMSG1024;
    break;

  case DS2404:
    BPMSG1025;
    break;

  case DS2431:
    BPMSG1026;
    break;

  default:
    BPMSG1027;
  }

  bpBR;
}

#endif /* BP_1WIRE_LOOKUP_FAMILY_ID */

bool device_find_first() {
  onewire_state.last_device_discrepancy = 0;
  onewire_state.last_family_discrepancy = 0;
  onewire_state.last_device_flag = false;

  return perform_device_search();
}

bool device_find_next(void) { return perform_device_search(); }

bool perform_device_search(void) {
  bool id_bit;
  bool cmp_id_bit;

  uint8_t id_bit_number;
  uint8_t last_zero;
  uint8_t rom_byte_offset;
  uint8_t search_result;
  uint8_t rom_byte_mask;
  uint8_t search_direction;

  id_bit_number = 1;
  last_zero = 0;
  rom_byte_offset = 0;
  rom_byte_mask = 1;
  search_result = 0;
  onewire_state.crc8 = 0;

  /* Check if the bus enumeration is still in progress. */

  if (!onewire_state.last_device_flag) {

    /* The last device on the bus was not yet found. */

    if (perform_bus_reset()) {
      onewire_state.last_device_discrepancy = 0;
      onewire_state.last_family_discrepancy = 0;
      onewire_state.last_device_flag = false;

      return false;
    }

    /* Issue search command. */

    ONEWIRE_WRITE_BYTE(onewire_state.command_byte);

    do {

      /* Read a bit and its complement. */

      id_bit = ONEWIRE_READ_BIT();
      cmp_id_bit = ONEWIRE_READ_BIT();

      if ((id_bit == ON) && (cmp_id_bit == ON)) {

        /* No devices on the line, bail out. */

        break;
      }

      if (id_bit != cmp_id_bit) {
        search_direction = id_bit;
      } else {

        /*
         * Determine the search direction from the last recorded discrepancy
         * index.
         */

        if (id_bit_number < onewire_state.last_device_discrepancy) {
          search_direction =
              (onewire_state.rom_bytes[rom_byte_offset] & rom_byte_mask) > 0;
        } else {

          /* Determine search direction. */

          search_direction =
              (id_bit_number == onewire_state.last_device_discrepancy);
        }

        /* If a 0 was read, then record its position. */

        if (search_direction == 0) {
          last_zero = id_bit_number;

          /* Check for the last bit discrepancy in the family byte. */

          if (last_zero < 9) {
            onewire_state.last_family_discrepancy = last_zero;
          }
        }
      }

      /*
       * Set or clear the current ROM address bit according to the search
       * direction.
       */
      if (search_direction == 1) {
        onewire_state.rom_bytes[rom_byte_offset] |= rom_byte_mask;
      } else {
        onewire_state.rom_bytes[rom_byte_offset] &= ~rom_byte_mask;
      }

      /* Write the serial number search direction bit. */

      ONEWIRE_WRITE_BIT(search_direction);

      /* Advance by one bit. */

      id_bit_number++;
      rom_byte_mask <<= 1;

      /* If 8 bits have been read, update CRC and move to the next byte. */

      if (rom_byte_mask == 0) {

        /* Calculate CRC. */

        update_crc8(onewire_state.rom_bytes[rom_byte_offset]);
        rom_byte_offset++;
        rom_byte_mask = 1;
      }

      /* Get all 8 bytes. */

    } while (rom_byte_offset < ROM_BYTES_SIZE);

    /* Checks the result of the search. */

    if (!((id_bit_number < 65) || (onewire_state.crc8 != 0))) {

      /* Update search state values. */

      onewire_state.last_device_discrepancy = last_zero;

      if (onewire_state.last_device_discrepancy == 0) {
        onewire_state.last_device_flag = true;
      }

      /* Found a device. */

      search_result = true;
    }
  }

  /* No device was found, so reset search state to a clean slate. */

  if (!search_result || !onewire_state.rom_bytes[0]) {
    onewire_state.last_device_discrepancy = 0;
    onewire_state.last_family_discrepancy = 0;
    onewire_state.last_device_flag = false;
    search_result = false;
  }

  return search_result;
}

uint8_t update_crc8(const uint8_t value) {
  onewire_state.crc8 = CRC_TABLE[onewire_state.crc8 ^ value];
  return onewire_state.crc8;
}

void binary_io_enter_1wire_mode(void) {
  uint8_t input_byte;
  uint8_t command;

  ONEWIRE_DATA_DIRECTION = INPUT;
  ONEWIRE_DATA_LINE = LOW;
  BP_CS_DIR = OUTPUT;

  /* Set CS to open drain. */

  mode_configuration.high_impedance = true;

  /* Just in case. */

  mode_configuration.little_endian = NO;

  /* Send version string. */

  MSG_1WIRE_MODE_IDENTIFIER;

  for (;;) {
    input_byte = user_serial_read_byte();
    command = input_byte >> 4;

    switch (command) {
    case BINARY_IO_ONEWIRE_COMMAND_ACTION:
      switch (input_byte) {
      case BINARY_IO_ONEWIRE_ACTION_EXIT:
        return;

      case BINARY_IO_ONEWIRE_ACTION_VERSION_STRING:
        MSG_1WIRE_MODE_IDENTIFIER;
        break;

      case BINARY_IO_ONEWIRE_ACTION_BUS_RESET:
        perform_bus_reset();
        REPORT_IO_SUCCESS();
        break;

      case BINARY_IO_ONEWIRE_ACTION_READ_BYTE:
        user_serial_transmit_character(ONEWIRE_READ_BYTE());
        break;

      case BINARY_IO_ONEWIRE_ACTION_ROM_SEARCH_MACRO:
      case BINARY_IO_ONEWIRE_ACTION_ALARM_SEARCH_MACRO: {
        bool next;
        size_t index;

        REPORT_IO_SUCCESS();

        onewire_state.command_byte =
            (input_byte == BINARY_IO_ONEWIRE_ACTION_ALARM_SEARCH_MACRO)
                ? MACRO_ALARM_SEARCH
                : MACRO_SEARCH_ROM;

        /* Find all devices. */

        next = device_find_first();
        while (next) {
          /* Print address. */
          bp_write_buffer(&onewire_state.rom_bytes[0],
                          sizeof(onewire_state.rom_bytes));
          next = device_find_next();
        }

        /* Send 8x 0xFF bytes. */

        for (index = 0; index < 8; index++) {
          user_serial_transmit_character(0xFF);
        }

        break;
      }

      default:
        REPORT_IO_FAILURE();
        break;
      }
      break;

    case BINARY_IO_ONEWIRE_COMMAND_BULK_TRANSFER: {
      size_t index;

      input_byte = (input_byte & 0x0F) + 1;
      REPORT_IO_SUCCESS();

      for (index = 0; index < input_byte; index++) {
        ONEWIRE_WRITE_BYTE(user_serial_read_byte());
        REPORT_IO_SUCCESS();
      }

      break;
    }

    case BINARY_IO_ONEWIRE_COMMAND_CONFIGURE:
      mode_configuration.speed = (input_byte & 1);
      user_serial_transmit_character(BP_BINARY_IO_RESULT_SUCCESS);
      break;

    case BINARY_IO_ONEWIRE_COMMAND_CONFIGURE_PERIPHERALS:
      bp_binary_io_peripherals_set(input_byte);
      REPORT_IO_SUCCESS();
      break;

#ifdef BUSPIRATEV4
    case BINARY_IO_ONEWIRE_COMMAND_READ_PERIPHERALS:
      user_serial_transmit_character(bp_binary_io_pullup_control(input_byte));
      break;
#endif /* BUSPIRATEV4 */

    default:
      REPORT_IO_FAILURE();
      break;
    }
  }
}

bool onewire_internal_bit_io(bool bit_value) {
  ONEWIRE_DATA_DIRECTION = INPUT;
  ONEWIRE_DATA_LINE = LOW;
  ONEWIRE_DATA_DIRECTION = OUTPUT;

  /* AN126: Parameter A */
  if (mode_configuration.speed == 0) {
    bp_delay_us(4);
  } else {
    bp_delay_us(1);
  }
  if (bit_value) {
    ONEWIRE_DATA_DIRECTION = INPUT;
  }
  /* AN126: Parameter E */
  if (mode_configuration.speed == 0) {
    bp_delay_us(8);
  }

  /*
   * This is where the magic happens. If a bit_value value of 1 is sent to this
   * function, well thats the same timing needed to get a value (not just send
   * it) so why not perform both?  So sending 1 will not only send one bit;
   * it will also read one bit.  It all depends on what the iDevice is in the
   * mood to do.  If it is in send mode then it will sends its data, if it is in
   * receive mode, then we will send ours.  Magical, I know. :)
   */

  if (bit_value) {
    bit_value = ONEWIRE_DATA_LINE;
    /* AN126: Parameter F */
    if (mode_configuration.speed == 0) {
      bp_delay_us(32);
    } else {
      bp_delay_us(6);
    }
  } else {
    /* AN126: Parameter C */
    if (mode_configuration.speed == 0) {
      bp_delay_us(25);
    } else {
      bp_delay_us(4);
    }
    ONEWIRE_DATA_DIRECTION = INPUT;
    /* AN126: Parameter D */
    if (mode_configuration.speed == 0) {
      bp_delay_us(7);
    } else {
      bp_delay_us(2);
    }
  }

  /* Adjust timing to take 70us per bit for standard mode. */

  if (mode_configuration.speed == 0) {
    bp_delay_us(5);
  }

  return bit_value;
}

uint8_t onewire_internal_byte_io(uint8_t byte_value) {
  uint8_t bit_index;
  uint8_t current_bit;

  current_bit = 0;

  /*
   * Nothing much to say about this function, it is pretty standard; do this 8
   * times and collect results.  Except that sends and GETS data.  Sending a
   * value of 0xFF will have the onewire_internal_bit_io function return bits;
   * this will collect those returns and spit them out.  Same with send.  It all
   * depends on what the iDevice is looking for at the time this command is
   * sent.
   */

  for (bit_index = 0; bit_index < 8; bit_index++) {
    current_bit = onewire_internal_bit_io(byte_value & 0x01);
    byte_value >>= 1;
    if (current_bit) {
      byte_value |= 0x80;
    }
  }

  if (mode_configuration.speed == 0) {
    bp_delay_us(8);
  }

  return byte_value;
}

void onewire_internal_set_data_state(const bool state) {
  onewire_state.data_state = state;
  MSG_1WIRE_NEXT_CLOCK_ALERT;
}

#endif /* BP_ENABLE_1WIRE_SUPPORT */
