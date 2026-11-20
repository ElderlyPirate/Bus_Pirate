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
#include <stdint.h>
#include "onboard_eeprom.h"

#ifdef BUSPIRATEV4
#include "base.h"

#define EEPROM_WRITE_TIMEOUT_US 10000UL
#define EEPROM_READY_POLLS 1024u
#define EEPROM_I2C_WRITE_ADDRESS 0xA0
#define EEPROM_I2C_READ_ADDRESS 0xA1
#define EEPROM_I2C_CLOCK_RATE 37
#define EEPROM_POLL_LIMIT 25000u
#define EEPROM_SEQUENCES 0x001Fu
#define EEPROM_ERRORS (_I2C1STAT_BCL_MASK | _I2C1STAT_IWCOL_MASK | _I2C1STAT_I2COV_MASK)
#define EEPROM_TX_BUSY (_I2C1STAT_TRSTAT_MASK | _I2C1STAT_TBF_MASK)
#define EEPROM_SDA (1u << 9)
#define EEPROM_SCL (1u << 10)
#define EEPROM_SDA_LAT LATDbits.LATD9
#define EEPROM_SCL_LAT LATDbits.LATD10
#define EEPROM_WP_LAT LATDbits.LATD11

static bool eeprom_error;

static bool eeprom_i2c_setup(void);
static bool eeprom_read_byte(uint16_t address, uint8_t *value);
static bool eeprom_write_byte(uint8_t value, uint16_t address);
static bool eeprom_ready(void);

static void eeprom_release(void) {
  I2C1CON = 0;
  (void)I2C1RCV;
  I2C1STAT = 0;
  BP_EE_SDA_DIR = INPUT;
  BP_EE_SCL_DIR = INPUT;
}

static void eeprom_abort(void) {
  eeprom_error = true;
  EEPROM_WP_LAT = HIGH;
  eeprom_release();
}

/* DS39897C 16-1/16-2: completion precedes ACKSTAT or receive-buffer use.
 * The poll count is finite; interrupts remain enabled during every wait. */
static bool eeprom_wait(uint16_t con_busy, uint16_t stat_busy,
                        uint16_t stat_required) {
  uint16_t waited;
  if (eeprom_error) {
    return false;
  }
  for (waited = 0; waited < EEPROM_POLL_LIMIT; waited++) {
    uint16_t status = I2C1STAT;
    uint16_t control = I2C1CON;
    if ((status & EEPROM_ERRORS) || !(control & _I2C1CON_I2CEN_MASK)) {
      break;
    }
    if (!(control & con_busy) && !(status & stat_busy) &&
        (status & stat_required) == stat_required) {
      return true;
    }
    delay_short_inline(1);
  }
  eeprom_abort();
  return false;
}

static bool eeprom_gpio_high(uint16_t pins) {
  uint16_t waited;
  /* DS39897C 10.2.1: separate a caller's TRIS change from PORT readback. */
  Nop();
  for (waited = 0; waited < EEPROM_POLL_LIMIT; waited++) {
    if ((IOPOR & pins) == pins) {
      return true;
    }
    delay_short_inline(1);
  }
  eeprom_abort();
  return false;
}

/* Recover only this single-master internal bus. WP is asserted by the caller
 * so the recovery STOP cannot commit an interrupted, unverified write. */
static bool eeprom_bus_clear(void) {
  uint8_t pulse;
  BP_EE_SDA_DIR = INPUT;
  BP_EE_SCL_DIR = INPUT;
  EEPROM_SDA_LAT = LOW;
  EEPROM_SCL_LAT = LOW;
  if (!eeprom_gpio_high(EEPROM_SCL)) {
    return false;
  }
  if (IOPOR & EEPROM_SDA) {
    return true;
  }
  for (pulse = 0; pulse < 9; pulse++) {
    BP_EE_SCL_DIR = OUTPUT;
    delay_short_inline(5);
    BP_EE_SCL_DIR = INPUT;
    if (!eeprom_gpio_high(EEPROM_SCL)) {
      return false;
    }
    delay_short_inline(5);
  }
  if (!(IOPOR & EEPROM_SDA)) {
    eeprom_abort();
    return false;
  }
  BP_EE_SCL_DIR = OUTPUT;
  BP_EE_SDA_DIR = OUTPUT;
  delay_short_inline(5);
  BP_EE_SCL_DIR = INPUT;
  if (!eeprom_gpio_high(EEPROM_SCL)) {
    return false;
  }
  delay_short_inline(5);
  BP_EE_SDA_DIR = INPUT;
  delay_short_inline(5);
  return eeprom_gpio_high(EEPROM_SCL | EEPROM_SDA);
}

void eeprom_initialize(void) {
  /* Protect before releasing a previously active peripheral or output. */
  EEPROM_WP_LAT = HIGH;
  BP_EE_WP_DIR = OUTPUT;
  eeprom_release();
  EEPROM_SDA_LAT = LOW;
  EEPROM_SCL_LAT = LOW;
}

static bool eeprom_i2c_setup(void) {
  eeprom_error = false;
  eeprom_release();
  if (!eeprom_bus_clear()) {
    return false;
  }
  /* SCLREL is slave-only. DISSLW=0 enables the 400 kHz slew control;
   * SMEN/A10M and all stale sequencing controls are deliberately clear. */
  I2C1CON = _I2C1CON_SCLREL_MASK;
  I2C1ADD = 0;
  I2C1MSK = 0;
  I2C1BRG = EEPROM_I2C_CLOCK_RATE;
  I2C1CON |= _I2C1CON_I2CEN_MASK;
  return true;
}

static bool eeprom_start(bool repeated) {
  if (!eeprom_wait(EEPROM_SEQUENCES, EEPROM_TX_BUSY, 0)) {
    return false;
  }
  if (!repeated && !eeprom_gpio_high(EEPROM_SCL | EEPROM_SDA)) {
    return false;
  }
  I2C1CON |= repeated ? _I2C1CON_RSEN_MASK : _I2C1CON_SEN_MASK;
  return eeprom_wait(EEPROM_SEQUENCES, 0, 0);
}

static bool eeprom_stop(void) {
  if (!eeprom_wait(EEPROM_SEQUENCES, EEPROM_TX_BUSY, 0)) {
    return false;
  }
  I2C1CON |= _I2C1CON_PEN_MASK;
  return eeprom_wait(_I2C1CON_PEN_MASK, 0, 0);
}

static bool eeprom_i2c_get_ack(void) {
  return (I2C1STAT & _I2C1STAT_ACKSTAT_MASK) != 0;
}

static bool eeprom_i2c_write(uint8_t value) {
  if (!eeprom_wait(EEPROM_SEQUENCES, EEPROM_TX_BUSY, 0)) {
    return false;
  }
  I2C1TRN = value;
  return eeprom_wait(0, EEPROM_TX_BUSY, 0);
}

static bool eeprom_checked_write(uint8_t value) {
  return eeprom_i2c_write(value) && !eeprom_i2c_get_ack();
}

/* DS20001189 7.0: address-only ACK polling after STOP, never replay data.
 * The shared timestamp wraps at 31 bits; the finite poll budget is also a
 * bound if a test clock does not advance. No fixed write-cycle sleep. */
static bool eeprom_ready(void) {
  uint32_t started = bp_timer_microseconds();
  uint16_t poll;
  for (poll = 0; poll < EEPROM_READY_POLLS; poll++) {
    bool acknowledged;
    if (!eeprom_start(false) || !eeprom_i2c_write(EEPROM_I2C_WRITE_ADDRESS)) {
      return false;
    }
    acknowledged = !eeprom_i2c_get_ack();
    if (!eeprom_stop()) {
      return false;
    }
    if (acknowledged) {
      return true;
    }
    if (((bp_timer_microseconds() - started) & 0x7FFFFFFFUL) >=
        EEPROM_WRITE_TIMEOUT_US) {
      break;
    }
  }
  eeprom_abort();
  return false;
}

static bool eeprom_i2c_read(uint8_t *value) {
  if (!eeprom_wait(EEPROM_SEQUENCES, EEPROM_TX_BUSY, 0)) {
    return false;
  }
  I2C1CON |= _I2C1CON_RCEN_MASK;
  if (!eeprom_wait(_I2C1CON_RCEN_MASK, 0, _I2C1STAT_RBF_MASK)) {
    return false;
  }
  *value = LO8(I2C1RCV);
  return true;
}

static bool eeprom_i2c_send_ack(bool nack) {
  if (!eeprom_wait(EEPROM_SEQUENCES, EEPROM_TX_BUSY, 0)) {
    return false;
  }
  I2C1CON = (I2C1CON & ~_I2C1CON_ACKDT_MASK) |
            (nack ? _I2C1CON_ACKDT_MASK : 0) | _I2C1CON_ACKEN_MASK;
  return eeprom_wait(_I2C1CON_ACKEN_MASK, 0, 0);
}

static bool eeprom_read_byte(uint16_t address, uint8_t *value) {
  uint8_t byte;
  if (address >= ONBOARD_EEPROM_CAPACITY || value == NULL) {
    return false;
  }
  if (!eeprom_start(false) || !eeprom_checked_write(EEPROM_I2C_WRITE_ADDRESS) ||
      !eeprom_checked_write(address >> 8) || !eeprom_checked_write(address) ||
      !eeprom_start(true) || !eeprom_checked_write(EEPROM_I2C_READ_ADDRESS) ||
      !eeprom_i2c_read(&byte) || !eeprom_i2c_send_ack(true) || !eeprom_stop()) {
    (void)eeprom_stop();
    return false;
  }
  *value = byte;
  return true;
}

static bool eeprom_write_byte(uint8_t value, uint16_t address) {
  if (address >= ONBOARD_EEPROM_CAPACITY) {
    return false;
  }
  if (!eeprom_start(false) || !eeprom_checked_write(EEPROM_I2C_WRITE_ADDRESS) ||
      !eeprom_checked_write(address >> 8) || !eeprom_checked_write(address) ||
      !eeprom_checked_write(value) || !eeprom_stop()) {
    (void)eeprom_stop();
    return false;
  }
  return eeprom_ready();
}

bool eeprom_test(void) {
  uint8_t original, value;
  bool result = false, touched = false, restored = true;
  bool write_protection = EEPROM_WP_LAT;
  /* Do not steal I2C1 from an active protocol owner. */
  if (I2C1CON & _I2C1CON_I2CEN_MASK) {
    return false;
  }
  EEPROM_WP_LAT = HIGH;
  if (!eeprom_i2c_setup() || !eeprom_ready() || !eeprom_read_byte(0, &original)) {
    goto done;
  }
  EEPROM_WP_LAT = LOW;
  touched = true;
  result = eeprom_write_byte((uint8_t)~original, 0) &&
           eeprom_read_byte(0, &value) && value == (uint8_t)~original;

  /* A failed test is not permission to discard its saved byte. Recover the
   * internal controller, then make one bounded rollback attempt. */
  if (eeprom_error) {
    EEPROM_WP_LAT = HIGH;
    if (!eeprom_i2c_setup() || !eeprom_ready()) {
      restored = false;
      goto done;
    }
    EEPROM_WP_LAT = LOW;
  }
  restored = eeprom_write_byte(original, 0) &&
             eeprom_read_byte(0, &value) && value == original;
done:
  eeprom_release();
  /* Fail closed if restoration could not be verified. */
  EEPROM_WP_LAT = (touched && !restored) ? HIGH : write_protection;
  return result && restored;
}

#endif /* BUSPIRATEV4 */
