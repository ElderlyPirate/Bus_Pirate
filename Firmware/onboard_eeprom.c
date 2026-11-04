/*
 * This file is part of the Bus Pirate project
 * (https://github.com/BusPirate/Bus_Pirate/).
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
#include <stddef.h>

#include "onboard_eeprom.h"

#ifdef BUSPIRATEV4

/**
 * How many milliseconds to wait after a I2C flash write.
 */
#define I2C_WRITE_DELAY_TIME 10

#include "base.h"

/* Dedicated v4 EEPROM pins; PORT remains readback, LAT owns output state. */
#define EEPROM_SDA_LAT LATDbits.LATD9
#define EEPROM_SCL_LAT LATDbits.LATD10
#define EEPROM_WP_LAT LATDbits.LATD11

/**
 * On-board EEPROM I2C address for write operations.
 *
 * 0xA0 = 10100000
 *        ||||||||
 *        |||||||+--- R/W (0: Write, 1: Read) = 0
 *        ||||||+---- A0 = 0
 *        |||||+----- A1 = 0
 *        ||||+------ A2 = 0
 *        ++++------- A7-A3 = 1010
 */
#define EEPROM_I2C_WRITE_ADDRESS 0xA0

/**
 * On-board EEPROM I2C address for read operations.
 *
 * 0xA1 = 10100001
 *        ||||||||
 *        |||||||+--- R/W (0: Write, 1: Read) = 1
 *        ||||||+---- A0 = 0
 *        |||||+----- A1 = 0
 *        ||||+------ A2 = 0
 *        ++++------- A7-A3 = 1010
 */
#define EEPROM_I2C_READ_ADDRESS 0xA1

/**
 * Send an ACK response during I2C acknowledge.
 */
#define EEPROM_I2C_ACK false

/**
 * Send a NACK response during I2C acknowledge.
 */
#define EEPROM_I2C_NACK true

/**
 * The I2C clock rate.
 *
 * Calculated on a 400kHz clock rate at 16MHz Fcy.
 */
#define EEPROM_I2C_CLOCK_RATE 37

/**
 * Sets up the I2C communication channel with the on-board EEPROM.
 */
static void eeprom_i2c_setup(void);

/**
 * Reads a byte from the I2C bus.
 *
 * @warning The operation will be performed synchronously.
 *
 * @return a byte read from the I2C bus.
 */
static uint8_t eeprom_i2c_read(void);

/**
 * Writes a byte to the I2C bus.
 *
 * @warning The operation will be performed synchronously.
 *
 * @param[in] value the byte to write to the bus.
 */
static void eeprom_i2c_write(uint8_t value);

/**
 * Sends either an ACK or a NACK on the I2C bus on data acknowledgement.
 *
 * @param[in] ack true if a NACK needs to be sent, false if an ACK ought to be
 *                sent otherwise.
 *
 * @see EEPROM_I2C_ACK
 * @see EEPROM_I2C_NACK
 */
static void eeprom_i2c_send_ack(bool ack);

/**
 * Gets the last seen acknowledgement status bit from the I2C bus.
 *
 * @return true if a NACK was detected last, false if an ACK was detected
 *              instead.
 *
 * @see EEPROM_I2C_ACK
 * @see EEPROM_I2C_NACK
 */
static bool eeprom_i2c_get_ack(void);

/**
 * Sets up a start condition on the I2C bus, preparing the bus for data
 * transfer.
 *
 * @warning The operation will be performed synchronously.
 */
static void eeprom_start(void);

/**
 * Sets up a stop condition on the I2C bus, preparing the bus for shutdown.
 *
 * @warning The operation will be performed synchronously.
 */
static void eeprom_stop(void);

/**
 * Reads a byte from the on-board EEPROM at the given address.
 *
 * @warning This function can only read bytes located in the first 64 kilobytes
 *          of the chip in use - keep in mind that the Bus Pirate v4 ships with
 *          a 8 kilobytes I2C flash chip.  Reading past the 8 kilobyte mark will
 *          yield an undefined condition.
 * @warning The operation will be performed synchronously.
 *
 * @param[in] address the address to read the byte from.
 * @return the byte read from the EEPROM.
 */
static uint8_t eeprom_read_byte(uint16_t address);

/**
 * Writes a byte on the on-board EEPROM at the given address.
 *
 * @warning This function can only write bytes located in the first 64 kilobytes
 *          of the chip in use - keep in mind that the Bus Pirate v4 ships with
 *          a 8 kilobytes I2C flash chip.  Writing past the 8 kilobyte mark will
 *          yield an undefined condition.
 * @warning The operation will be performed synchronously.
 *
 * @param[in] value the byte to write.
 * @param[in] address the address to write the byte to.
 * @return the byte read from the EEPROM.
 */
static bool eeprom_write_byte(uint8_t value, uint16_t address);

void eeprom_initialize(void) {

  /* I2C data line set as INPUT. */
  BP_EE_SDA_DIR = INPUT;

  /* I2C clock line set as INPUT. */
  BP_EE_SCL_DIR = INPUT;

  /* I2C clock line LOW. */
  EEPROM_SCL_LAT = LOW;

  /* I2C data line LOW. */
  EEPROM_SDA_LAT = LOW;

  /* EEPROM Write Protect line HIGH. */
  EEPROM_WP_LAT = HIGH;
  BP_EE_WP_DIR = OUTPUT;
}

bool eeprom_test(void) {

  /* Set up I2C access to the EEPROM. */
  eeprom_i2c_setup();

  /* Get the current write protection flag state. */
  bool write_protection = EEPROM_WP_LAT;

  /* Enable writing to the EEPROM. */
  EEPROM_WP_LAT = LOW;

  /* Write 0xFF (the default value) to offset 0. */
  eeprom_write_byte(0xFF, 0);
  bp_delay_ms(I2C_WRITE_DELAY_TIME);

  /* Write 0x10 to offset 0. */
  eeprom_write_byte(0x10, 0);
  bp_delay_ms(I2C_WRITE_DELAY_TIME);

  /* Checks if writing 0x10 actually succeeded. */
  bool result = (eeprom_read_byte(0) == 0x10);

  /* Set offset 0 to the appropriate value. */
  eeprom_write_byte(0xFF, 0);

  /* Restore the old write protection flag state. */
  EEPROM_WP_LAT = write_protection;

  return result;
}

void eeprom_start(void) {

  /* Initiate a start condition on the I2C bus. */
  I2C1CONbits.SEN = ON;

  /* Wait until start condition has been set. */
  while (I2C1CONbits.SEN == ON) {
  }
}

void eeprom_stop(void) {

  /* Initiate a stop condition on the I2C bus. */
  I2C1CONbits.PEN = ON;

  /* Wait until stop condition has been set. */
  while (I2C1CONbits.PEN == ON) {
  }
}

inline bool eeprom_i2c_get_ack(void) {

  /* Return last seen I2C acknowledge bit. */
  return I2C1STATbits.ACKSTAT;
}

void eeprom_i2c_send_ack(bool ack) {

  /* Set I2C acknowledge data bit. */
  I2C1CONbits.ACKDT = ack;

  /* Set I2C acknowledge sequence data bit. */
  I2C1CONbits.ACKEN = ON;

  /* Wait for acknowledgement. */
  while (I2C1CONbits.ACKEN == ON) {
  }
}

void eeprom_i2c_write(uint8_t value) {

  /* Shifts in the value to send on the bus. */
  I2C1TRN = value;

  /* Waits for the transmission to be over. */
  while (I2C1STATbits.TRSTAT == ON) {
  }
}

uint8_t eeprom_i2c_read(void) {

  /* Starts receiving data from the I2C bus. */
  I2C1CONbits.RCEN = ON;

  /* Waits until a full byte has been received. */
  while (I2C1CONbits.RCEN == ON) {
  }

  /* Returns the byte that has just been read. */
  return LO8(I2C1RCV);
}

void eeprom_i2c_setup(void) {

  /*
   * I2C1CON: I2C1 CONTROL REGISTER
   *
   * MSB
   * x-x0x0x0xxxxxxxx
   *    | | |
   *    | | +------------ SMEN:  Disable SMBus support.
   *    | +-------------- A10M:  Use 7-bit slave addresses.
   *    +---------------- SCREL: Enable clock stretching.
   *
   */
  I2C1CON &= ~(_I2C1CON_A10M_MASK | _I2C1CON_SCLREL_MASK | _I2C1CON_SMEN_MASK);

  /* Clear I2C address register. */
  I2C1ADD = 0x0000;

  /* Clear I2C address mask register. */
  I2C1MSK = 0x0000;

  /* Set I2C baud rate (400kHz). */
  I2C1BRG = EEPROM_I2C_CLOCK_RATE;

  /* Enable I2C. */
  I2C1CONbits.I2CEN = ON;
}

uint8_t eeprom_read_byte(const uint16_t address) {

  /* Start data transmission on the bus. */
  eeprom_start();

  /* Send a dummy write to the bus with the requested read address. */
  eeprom_i2c_write(EEPROM_I2C_WRITE_ADDRESS);
  eeprom_i2c_write(address >> 8);
  eeprom_i2c_write(address);

  /* Start data transmission on the bus. */
  eeprom_start();

  /* Send to the bus a read request. */
  eeprom_i2c_write(EEPROM_I2C_READ_ADDRESS);

  /* Read the byte from the EEPROM. */
  uint8_t byte = eeprom_i2c_read();

  /* Return a NACK for acknowledgement. */
  eeprom_i2c_send_ack(EEPROM_I2C_NACK);

  /* Stop data transmission on the bus. */
  eeprom_stop();

  return byte;
}

bool eeprom_write_byte(const uint8_t value, const uint16_t address) {

  /* Start data transmission on the bus. */
  eeprom_start();

  /* Send a write request containing the target address to the bus. */
  eeprom_i2c_write(EEPROM_I2C_WRITE_ADDRESS);
  eeprom_i2c_write(address >> 8);
  eeprom_i2c_write(address);

  /* Send the byte to write to the bus. */
  eeprom_i2c_write(value);

  /* Read the acknowledgement result from the operation. */
  bool result = eeprom_i2c_get_ack();

  /* Stop data transmission on the bus. */
  eeprom_stop();

  return result;
}

#endif /* BUSPIRATEV4 */
