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

#include "i2c.h"

#ifdef BP_ENABLE_I2C_SUPPORT

#include "aux_pin.h"
#include "base.h"
#include "binary_io.h"
#include "bitbang.h"
#include "core.h"
#include "proc_menu.h"

#if defined(BUSPIRATEV4) && !defined(BP_I2C_USE_HW_BUS)
#error "Bus Pirate v4 must be able to use the hardware I2C interface!"
#endif /* BUSPIRATEV4 && !BP_I2C_USE_HW_BUS */

/**
 * Use a software I2C communication implementation
 */
#define I2C_TYPE_SOFTWARE 0

/**
 * Use the built-in hardware I2C communication implementation
 */
#define I2C_TYPE_HARDWARE 1

/**
 * I2C ACK bit value.
 */
#define I2C_ACK_BIT 0

/**
 * I2C NACK bit value.
 */
#define I2C_NACK_BIT 1

#define I2C_SNIFFER_ESCAPE '\\'
#define I2C_SNIFFER_NACK '-'
#define I2C_SNIFFER_ACK '+'
#define I2C_SNIFFER_START '['
#define I2C_SNIFFER_STOP ']'

typedef struct {

  /**
   * Flag indicating whether a software-only I2C implementation should be
   * used instead of the built-in hardware I2C interface.
   *
   * @see I2C_TYPE_SOFTWARE
   * @see I2C_TYPE_HARDWARE
   */
  uint8_t mode : 1;

  /**
   * Flag indicating whether there is either an ACK/NACK to be received.
   */
  uint8_t acknowledgment_pending : 1;

#ifdef BUSPIRATEV4

  /**
   * Flag indicating whether it is sending commands to the I2C EEPROM or not.
   */
  uint8_t to_eeprom : 1;

#endif /* BUSPIRATEV4 */

} i2c_state_t;

/**
 * Current I2C module state.
 */
static i2c_state_t i2c_state = {0};

#define SCL BP_CLK
#define SCL_TRIS BP_CLK_DIR
#define SDA BP_MOSI
#define SDA_TRIS BP_MOSI_DIR

extern bus_pirate_configuration_t bus_pirate_configuration;
extern mode_configuration_t mode_configuration;
extern command_t last_command;

/**
 * Handles a pending acknowledgement by sending either an ACK or a NACK on the
 * bus.
 *
 * @param[in] bus_bit false for sending an ACK, true for sending a NACK.
 *
 * @see I2C_ACK_BIT
 * @see I2C_NACK_BIT
 */
static void handle_pending_ack(const bool bus_bit);

#ifdef BP_I2C_USE_HW_BUS

/**
 * Frequency constants for the hardware I2C baud rate generator circuitry.
 */
static const uint8_t HARDWARE_I2C_BRG_SPEEDS[] = {
    157, /* 100 kHz. */
    37,  /* 400 kHz. */
    13,  /* 1000 kHz. */
};

/**
 * Last valid index of HARDWARE_I2C_BRG_SPEEDS.  The software driver has
 * one speed profile more than the hardware engine has rates (profile 4 is
 * software-only).
 */
#define HARDWARE_I2C_BRG_MAX_INDEX 2

static bool hardware_i2c_error = false;
static bool hardware_i2c_active = false;

/**
 * Returns the hardware baud rate generator value for the current session
 * speed, bounded to the table: the onboard-EEPROM macro keeps the
 * session speed, which can come from a software driver profile that has
 * no hardware rate.
 */
static uint8_t hardware_i2c_brg(void) {
  return HARDWARE_I2C_BRG_SPEEDS[mode_configuration.speed > HARDWARE_I2C_BRG_MAX_INDEX
                                     ? HARDWARE_I2C_BRG_MAX_INDEX
                                     : mode_configuration.speed];
}

/**
 * Performs hardware initialisation on the chosen hardware I2C interface.
 */
static void hardware_i2c_setup(void);

/**
 * Sends a start condition on the chosen hardware I2C interface.
 */
static void hardware_i2c_start(void);

/**
 * Sends a stop condition on the chosen hardware I2C interface.
 */
static void hardware_i2c_stop(void);

/**
 * Sends either an ACK or a NACK on the chosen hardware I2C interface.
 *
 * @param[in] ack false for ACK, true for NACK.
 *
 * @see I2C_ACK_BIT
 * @see I2C_NACK_BIT
 */
static void hardware_i2c_send_ack(bool ack);

/**
 * Gets the ACK state from the chosen hardware I2C interface.
 *
 * @return false for a ACK bit, true otherwise (NACK bit).
 *
 * @see I2C_ACK_BIT
 * @see I2C_NACK_BIT
 */
static bool hardware_i2c_get_ack(void);

/**
 * Writes the given byte to the chosen hardware I2C interface.
 *
 * @param[in] value the byte to write.
 */
static void hardware_i2c_write(const uint8_t value);

/**
 * Reads a byte from the chosen hardware I2C interface.
 *
 * @return the byte read from the interface.
 */
static uint8_t hardware_i2c_read(void);

#endif /* BP_I2C_USE_HW_BUS */

/**
 * Attempts to sniff data going through the chosen I2C interface.
 *
 * @param[in] interactive_mode true if data is shown to a human via the serial
 *                             command interface, false if data is sent as part
 *                             of a binary I/O command stream.
 */
static void i2c_sniffer(bool interactive_mode);

/**
 * Performs the bulk of the write-then-read I2C binary IO command.
 *
 * @return true if the operation was successful, false otherwise.
 */
static bool i2c_write_then_read(void);

static bool i2c_check_error(void) {
  bool failed = i2c_state.mode == I2C_TYPE_SOFTWARE && bitbang_i2c_error();
#ifdef BP_I2C_USE_HW_BUS
  if (i2c_state.mode == I2C_TYPE_HARDWARE) {
    failed = hardware_i2c_error;
  }
#endif
  if (failed) {
    i2c_state.acknowledgment_pending = false;
    bp_write_line("I2C bus error");
    mode_configuration.command_error = YES;
    return true;
  }
  return false;
}

uint16_t i2c_read(void) {
  uint8_t value;
  bitbang_i2c_clear_error();

  if (i2c_state.acknowledgment_pending) {
    handle_pending_ack(I2C_ACK_BIT);
    if (i2c_check_error()) {
      return 0xFF;
    }
    bpSP;
    MSG_ACK;
    bpSP;
  }

  if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
    value = bitbang_read_value(true);
  } else {
#ifdef BP_I2C_USE_HW_BUS
    value = hardware_i2c_read();
#else
    value = 0;
#endif /* BP_I2C_USE_HW_BUS */
  }
  if (i2c_check_error()) {
    return 0xFF;
  }
  i2c_state.acknowledgment_pending = true;
  return value;
}

unsigned int i2c_write(unsigned int c) {
  bitbang_i2c_clear_error();
  if (i2c_state.acknowledgment_pending) {
    handle_pending_ack(I2C_ACK_BIT);
    if (i2c_check_error()) {
      return 0x100;
    }
    bpSP;
    MSG_ACK;
    bpSP;
  }

  if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
    bitbang_write_value(c, true);
    c = bitbang_read_bit(true);
  } else {
#ifdef BP_I2C_USE_HW_BUS
    hardware_i2c_write(c);
    c = hardware_i2c_get_ack();
#endif /* BP_I2C_USE_HW_BUS */
  }

  if (i2c_check_error()) {
    return 0x100;
  }
  bpSP;
  if (c == 0) {
    MSG_ACK;
    return 0x300; // bit 9=ack
  }

  MSG_NACK;
  return 0x100; // bit 9=ack
}

void i2c_start(void) {
  bitbang_i2c_clear_error();
  /* Reset the bus state if an acknowledgement is pending. */

  if (i2c_state.acknowledgment_pending) {
    MSG_NACK;
    bpBR;

    handle_pending_ack(I2C_NACK_BIT);
    if (i2c_check_error()) {
      return;
    }
  }

  /* Send a start signal on the bus. */

  if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
    bitbang_i2c_start_wait(BITBANG_I2C_START_ONE_SHOT);
  } else {
#ifdef BP_I2C_USE_HW_BUS
    hardware_i2c_start();
#endif /* BP_I2C_USE_HW_BUS */
  }

  if (!i2c_check_error()) {
    MSG_I2C_START_BIT;
  }
}

void i2c_stop(void) {
  bitbang_i2c_clear_error();
  if (i2c_state.acknowledgment_pending) {
    MSG_NACK;
    bpBR;

    handle_pending_ack(I2C_NACK_BIT);
    if (i2c_check_error()) {
      return;
    }
  }

  if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
    bitbang_i2c_stop_wait();
  } else {
#ifdef BP_I2C_USE_HW_BUS
    hardware_i2c_stop();
#endif /* BP_I2C_USE_HW_BUS */
  }

  if (!i2c_check_error()) {
    MSG_I2C_STOP_BIT;
  }
}

void i2c_print_settings(void) {
  BPMSG1068;
#ifdef BP_I2C_USE_HW_BUS
  bp_write_dec_byte(i2c_state.mode);
#else
  bp_write_dec_byte(0);
#endif /* BP_I2C_USE_HW_BUS */
  bpSP;
  bp_write_dec_byte(mode_configuration.speed);
  bp_write_line(" )");
}

void i2c_setup_prepare(void) {
  int HW;
  int speed;

  HW = 0; // keep compiler happy if BP_USE_HW is not defined

#ifdef BP_I2C_USE_HW_BUS
  consumewhitechars();
  HW = getint();
#else
  i2c_state.mode = I2C_TYPE_SOFTWARE;
#endif /* BP_I2C_USE_HW_BUS */

  consumewhitechars();
  speed = getint();

#ifdef BP_I2C_USE_HW_BUS
  if ((HW > 0) && (HW <= 2)) {
    i2c_state.mode = HW - 1;
  } else {
    speed = 0;
  }

  /*
   * The engine has three rates (100 kHz, 400 kHz, 1 MHz); profile 4 is
   * for the software driver only.  Ask again rather than silently
   * accepting a rate the engine does not offer.
   */
  if ((i2c_state.mode == I2C_TYPE_HARDWARE) && (speed > 3)) {
    speed = 0;
  }
#endif /* BP_I2C_USE_HW_BUS */

  if ((speed > 0) && (speed <= 4)) {
    mode_configuration.speed = speed - 1;
  } else {
    speed = 0;
  }

  if (speed == 0) {
    mode_configuration.command_error = NO;

#ifdef BP_I2C_USE_HW_BUS
    BPMSG1064;
    i2c_state.mode = (getnumber(1, 1, 2, 0) - 1);
#else
    i2c_state.mode = I2C_TYPE_SOFTWARE;
#endif /* BP_I2C_USE_HW_BUS */

    if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
      MSG_SOFTWARE_MODE_SPEED_PROMPT;
      mode_configuration.speed = (getnumber(1, 1, 4, 0) - 1);
    } else {
#if defined(BUSPIRATEV3) && !defined(BPV3_IS_REV_B4_OR_LATER)
      if (bus_pirate_configuration.device_revision <= PIC_REV_A3) {
        BPMSG1066;
      }
#endif /* BUSPIRATEV3 && !BPV3_IS_REV_B4_OR_LATER */
      BPMSG1067;
      mode_configuration.speed = (getnumber(1, 1, 3, 0) - 1);
    }
  } else {
#if defined(BUSPIRATEV3) && !defined(BPV3_IS_REV_B4_OR_LATER)
    if (bus_pirate_configuration.device_revision <= PIC_REV_A3) {
      BPMSG1066;
    }
#endif /* BUSPIRATEV3 && !BPV3_IS_REV_B4_OR_LATER */
    i2c_print_settings();

    i2c_state.acknowledgment_pending = false;
  }

  mode_configuration.high_impedance = ON;
  bitbang_i2c_clear_error();

#ifdef BUSPIRATEV4
  /*
   * A new session starts on the external bus; the onboard-EEPROM macro
   * must not leak its I2C1 routing into the next session.
   */
  i2c_state.to_eeprom = false;
#endif /* BUSPIRATEV4 */

  /*
   * Software I2C takes its clock timing from the bit-bang delay profile;
   * apply it here as well because the interactive flow only runs
   * setup_execute() on the first power-on request (`W`), so a mode that is
   * never powered would otherwise clock with a stale profile.  The pins
   * are still configured by setup_execute() (hardware mode does its own
   * setup there).
   */
  if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
    bitbang_setup(2, mode_configuration.speed);
  }
}

void i2c_setup_execute(void) {
  if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
    SDA_TRIS = INPUT;
    SCL_TRIS = INPUT;
    SCL = LOW;
    SDA = LOW;
    bitbang_setup(2, mode_configuration.speed);
  } else {
#ifdef BP_I2C_USE_HW_BUS
    hardware_i2c_setup();
#endif /* BP_I2C_USE_HW_BUS */
  }
}

void i2c_cleanup(void) {
  /* Clear any pending ACK from previous use. */
  i2c_state.acknowledgment_pending = false;

#ifdef BP_I2C_USE_HW_BUS

  hardware_i2c_active = false;
  hardware_i2c_error = false;

#ifdef BUSPIRATEV4

    /* Disable external I2C module. */
    I2C3CONbits.I2CEN = OFF;

    /* Disable EEPROM I2C module. */
    I2C1CONbits.I2CEN = OFF;

#else

    /* Disable external I2C module. */
    I2C1CONbits.I2CEN = OFF;

#endif /* BUSPIRATEV4 */
#endif /* BP_I2C_USE_HW_BUS */
}

void i2c_macro(unsigned int c) {
  int i;

  switch (c) {

  case 0:
    BPMSG1069;
    break;

  case 1:
    bitbang_i2c_clear_error();
    // setup both lines high first
    bitbang_set_pins_high(MOSI | CLK, 0);
    BPMSG1070;
#ifdef BUSPIRATEV4
    if ((!i2c_state.to_eeprom && ((BP_CLK == LOW) || (BP_MOSI == LOW))) ||
        (i2c_state.to_eeprom && ((BP_EE_SDA == LOW) && (BP_EE_SCL == LOW)))) {
#else
    if ((BP_CLK == LOW) || (BP_MOSI == LOW)) {
#endif /* BUSPIRATEV4 */
      MSG_WARNING_HEADER;
      MSG_WARNING_SHORT_OR_NO_PULLUP;
      bpBR;
      return;
    }

    for (i = 0; i < 0x100; i++) {
      if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
        bitbang_i2c_start_wait(BITBANG_I2C_START_ONE_SHOT);
        bitbang_write_value(i, true);                        // send address
        c = bitbang_read_bit(true);                        // look for ack
      } else {
#ifdef BP_I2C_USE_HW_BUS
        hardware_i2c_start();
        hardware_i2c_write(i);
        c = hardware_i2c_get_ack();
#endif /* BP_I2C_USE_HW_BUS */
      }

      if (i2c_check_error()) {
        return;
      }
      if (c == I2C_ACK_BIT) {
        bp_write_formatted_integer(i);
        user_serial_transmit_character('(');
        bp_write_formatted_integer((i >> 1));
        /* If the first bit is set, this is a read address. */
        if ((i & 1) == 0) {
          MSG_I2C_WRITE_ADDRESS_END;
        } else {
          if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
            bitbang_read_value(true);
            bitbang_i2c_write_bit(HIGH);
          } else {
#ifdef BP_I2C_USE_HW_BUS
            hardware_i2c_read();
            hardware_i2c_send_ack(I2C_NACK_BIT);
#endif /* BP_I2C_USE_HW_BUS */
          }
          MSG_I2C_READ_ADDRESS_END;
        }
      }

      if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
        bitbang_i2c_stop_wait();
      } else {
#ifdef BP_I2C_USE_HW_BUS
        hardware_i2c_stop();
#endif /* BP_I2C_USE_HW_BUS */
      }
      if (i2c_check_error()) {
        return;
      }
    }
    bpBR;

    break;

  case 2:
    i2c_cleanup();

    MSG_SNIFFER_MESSAGE;
    MSG_ANY_KEY_TO_EXIT_PROMPT;
    i2c_sniffer(true);

#ifdef BP_I2C_USE_HW_BUS
    if (i2c_state.mode == I2C_TYPE_HARDWARE) {
      /* Setup the hardware I2C module once more. */
      hardware_i2c_setup();
    }
#endif /* BP_I2C_USE_HW_BUS */

    break;

#if defined(BUSPIRATEV4)

  case 3: {
    if (i2c_state.mode != I2C_TYPE_HARDWARE) {
      bp_write_line("Onboard EEPROM requires hardware I2C");
      mode_configuration.command_error = YES;
      break;
    }
    if (hardware_i2c_active) {
      i2c_stop();
      if (hardware_i2c_error) {
        break;
      }
    }
    I2C3CONbits.I2CEN = OFF;
    i2c_state.acknowledgment_pending = false;
    i2c_state.to_eeprom = true;
    hardware_i2c_setup();
    MSG_USING_ONBOARD_I2C_EEPROM;
    break;
  }

  case 4:
    if (i2c_state.to_eeprom) {
      MSG_ONBOARD_I2C_EEPROM_WRITE_PROTECT_DISABLED;
      BP_EE_WP = LOW;
    }

    break;

#endif /* BUSPIRATEV4 */

  default:
    MSG_UNKNOWN_MACRO_ERROR;
    break;
  }
}

void i2c_pins_state(void) { MSG_I2C_PINS_STATE; }

#if defined(BP_I2C_USE_HW_BUS)

/* DS39897C 16.1, registers 16-1/16-2: check completion before ACKSTAT.
 * Use word-wide STAT accesses also on old v3 silicon (DS39881 errata #26). */
#define I2C_HW_TIMEOUT 25000U
#define I2C_HW_SEQUENCES 0x001FU
#define I2C_HW_ERRORS (_I2C1STAT_BCL_MASK | _I2C1STAT_IWCOL_MASK | _I2C1STAT_I2COV_MASK)

typedef struct {
  volatile uint16_t *con;
  volatile uint16_t *stat;
  volatile uint16_t *trn;
  volatile uint16_t *rcv;
} hardware_i2c_registers_t;

static hardware_i2c_registers_t hardware_i2c_registers(void) {
#ifdef BUSPIRATEV4
  if (!i2c_state.to_eeprom) {
    hardware_i2c_registers_t bus = {&I2C3CON, &I2C3STAT, &I2C3TRN, &I2C3RCV};
    return bus;
  }
#endif
  hardware_i2c_registers_t bus = {&I2C1CON, &I2C1STAT, &I2C1TRN, &I2C1RCV};
  return bus;
}

static void hardware_i2c_abort(void) {
  hardware_i2c_registers_t bus = hardware_i2c_registers();
  hardware_i2c_error = true;
  hardware_i2c_active = false;
  i2c_state.acknowledgment_pending = false;
  *bus.con = 0;
  (void)*bus.rcv;
  *bus.stat = 0;
#ifdef BUSPIRATEV4
  if (i2c_state.to_eeprom) {
    BP_EE_SDA_DIR = INPUT;
    BP_EE_SCL_DIR = INPUT;
    return;
  }
#endif
  SDA_TRIS = INPUT;
  SCL_TRIS = INPUT;
}

static bool hardware_i2c_wait(uint16_t con_busy, uint16_t stat_busy,
                              uint16_t stat_required) {
  hardware_i2c_registers_t bus = hardware_i2c_registers();
  uint16_t waited;
  if (hardware_i2c_error) {
    return false;
  }
  for (waited = 0; waited < I2C_HW_TIMEOUT; waited++) {
    uint16_t status = *bus.stat;
    uint16_t control = *bus.con;
    if ((status & I2C_HW_ERRORS) || !(control & _I2C1CON_I2CEN_MASK)) {
      break;
    }
    if (!(control & con_busy) && !(status & stat_busy) &&
        (status & stat_required) == stat_required) {
      return true;
    }
    bp_delay_us(1);
  }
  hardware_i2c_abort();
  return false;
}

static bool hardware_i2c_wait_idle_lines(void) {
  uint16_t waited;
  for (waited = 0; waited < I2C_HW_TIMEOUT; waited++) {
#ifdef BUSPIRATEV4
    if (i2c_state.to_eeprom ? (BP_EE_SCL && BP_EE_SDA) : (BP_CLK && BP_MOSI)) {
#else
    if (BP_CLK && BP_MOSI) {
#endif
      return true;
    }
    bp_delay_us(1);
  }
  hardware_i2c_abort();
  return false;
}

static bool hardware_i2c_wait_gpio_high(uint16_t pin) {
  uint16_t waited;
  for (waited = 0; waited < I2C_HW_TIMEOUT; waited++) {
    if (IOPOR & pin) {
      return true;
    }
    bp_delay_us(1);
  }
  hardware_i2c_abort();
  return false;
}

static bool hardware_i2c_bus_clear(void) {
  uint16_t clock = CLK;
  uint16_t data = MOSI;
  uint8_t pulse;
#ifdef BUSPIRATEV4
  if (i2c_state.to_eeprom) {
    clock = (1U << 10);
    data = (1U << 9);
  }
#endif
  /* A new START after a fault requests recovery of this single-master bus.
     SHT3x-DIS 4.9 specifies nine released-SDA clocks
     to reset an interface stranded mid-read, followed by a new START. */
  IODIR |= clock | data;
  IOLAT &= ~(clock | data);
  if (!hardware_i2c_wait_gpio_high(clock)) {
    return false;
  }
  if (IOPOR & data) {
    return true;
  }
  for (pulse = 0; pulse < 9; pulse++) {
    IODIR &= ~clock;
    bp_delay_us(5);
    IODIR |= clock;
    if (!hardware_i2c_wait_gpio_high(clock)) {
      return false;
    }
    bp_delay_us(5);
  }
  if (!(IOPOR & data)) {
    hardware_i2c_abort();
    return false;
  }
  IODIR &= ~clock;
  IODIR &= ~data;
  bp_delay_us(5);
  IODIR |= clock;
  if (!hardware_i2c_wait_gpio_high(clock)) {
    return false;
  }
  bp_delay_us(5);
  IODIR |= data;
  bp_delay_us(5);
  return true;
}

void hardware_i2c_start(void) {
  hardware_i2c_registers_t bus = hardware_i2c_registers();
  if (hardware_i2c_error && !hardware_i2c_bus_clear()) {
    return;
  }
  if (hardware_i2c_error || !(*bus.con & _I2C1CON_I2CEN_MASK)) {
    hardware_i2c_setup();
  }
  if (!hardware_i2c_wait(I2C_HW_SEQUENCES, _I2C1STAT_TRSTAT_MASK, 0)) {
    return;
  }
  if (!hardware_i2c_active && !hardware_i2c_wait_idle_lines()) {
    return;
  }
  *bus.con |= hardware_i2c_active ? _I2C1CON_RSEN_MASK : _I2C1CON_SEN_MASK;
  if (hardware_i2c_wait(I2C_HW_SEQUENCES, 0, 0)) {
    hardware_i2c_active = true;
  }
}

void hardware_i2c_stop(void) {
  hardware_i2c_registers_t bus = hardware_i2c_registers();
  if (!hardware_i2c_wait(I2C_HW_SEQUENCES, _I2C1STAT_TRSTAT_MASK, 0)) {
    return;
  }
  *bus.con |= _I2C1CON_PEN_MASK;
  if (hardware_i2c_wait(_I2C1CON_PEN_MASK, 0, 0)) {
    hardware_i2c_active = false;
  }
}

bool hardware_i2c_get_ack(void) {
  hardware_i2c_registers_t bus = hardware_i2c_registers();
  return hardware_i2c_error || ((*bus.stat & _I2C1STAT_ACKSTAT_MASK) != 0);
}

static bool hardware_i2c_ready(void) {
  if (!hardware_i2c_active) {
    hardware_i2c_abort();
    return false;
  }
  return hardware_i2c_wait(I2C_HW_SEQUENCES,
                           _I2C1STAT_TRSTAT_MASK | _I2C1STAT_TBF_MASK, 0);
}

void hardware_i2c_send_ack(bool ack) {
  hardware_i2c_registers_t bus = hardware_i2c_registers();
  if (!hardware_i2c_ready()) {
    return;
  }
  *bus.con = (*bus.con & ~_I2C1CON_ACKDT_MASK) |
             (ack ? _I2C1CON_ACKDT_MASK : 0) | _I2C1CON_ACKEN_MASK;
  hardware_i2c_wait(_I2C1CON_ACKEN_MASK, 0, 0);
}

void hardware_i2c_write(const uint8_t value) {
  hardware_i2c_registers_t bus = hardware_i2c_registers();
  if (!hardware_i2c_ready()) {
    return;
  }
  *bus.trn = value;
  hardware_i2c_wait(0, _I2C1STAT_TRSTAT_MASK | _I2C1STAT_TBF_MASK, 0);
}

uint8_t hardware_i2c_read(void) {
  hardware_i2c_registers_t bus = hardware_i2c_registers();
  if (!hardware_i2c_ready()) {
    return 0xFF;
  }
  *bus.con |= _I2C1CON_RCEN_MASK;
  if (!hardware_i2c_wait(_I2C1CON_RCEN_MASK, 0, _I2C1STAT_RBF_MASK)) {
    return 0xFF;
  }
  return *bus.rcv;
}

void hardware_i2c_setup(void) {
  hardware_i2c_registers_t bus = hardware_i2c_registers();
  hardware_i2c_error = false;
  hardware_i2c_active = false;
  *bus.con = 0;
  (void)*bus.rcv;
  *bus.stat = 0;
  /* SCLREL is a slave-only control. SMEN=0 selects I2C input thresholds.
     Set slew explicitly: disabled at 100 kHz, enabled for faster profiles. */
  *bus.con = _I2C1CON_SCLREL_MASK |
             (mode_configuration.speed == 0 ? _I2C1CON_DISSLW_MASK : 0);
#ifdef BUSPIRATEV4
  if (!i2c_state.to_eeprom) {
    I2C3ADD = 0;
    I2C3MSK = 0;
    I2C3BRG = hardware_i2c_brg();
  } else {
    I2C1ADD = 0;
    I2C1MSK = 0;
    I2C1BRG = hardware_i2c_brg();
  }
#else
  I2C1ADD = 0;
  I2C1MSK = 0;
  I2C1BRG = hardware_i2c_brg();
#if !defined(BPV3_IS_REV_B4_OR_LATER)
  /* DS39881 errata #10: hold SDA low across module enable. */
  BP_MOSI_DIR = OUTPUT;
  bp_delay_us(200);
  LATBbits.LATB9 = OFF;
  bp_delay_us(200);
#endif
#endif
  *bus.con |= _I2C1CON_I2CEN_MASK;
}

#endif /* BP_I2C_USE_HW_BUS */

void i2c_sniffer(bool interactive_mode) {
  /* Setup UART ringbuffer. */
  user_serial_ringbuffer_setup();

  SDA_TRIS = INPUT;
  SCL_TRIS = INPUT;
  SCL = LOW;
  SDA = LOW;

  /* Enable change notice on SCL and SDA. */
  BP_MOSI_CN = ON;
  BP_CLK_CN = ON;

  /* Clear the change interrupt flag. */
  IFS1bits.CNIF = OFF;

  bool old_sda = SDA;
  bool old_scl = SCL;
  bool new_sda = old_sda;
  bool new_scl = old_scl;

  bool collect_data = false;
  uint8_t data_bits = 0;
  uint8_t data_value = 0;

  for (;;) {
    if (!IFS1bits.CNIF) {
      /* Change notice interrupt triggered. */

      /* Handle user I/O. */
      user_serial_ringbuffer_process();

      if (user_serial_ready_to_read()) {
        data_value = user_serial_read_byte();
        break;
      }

      continue;
    }

    /* Clear change notice interrupt flag. */
    IFS1bits.CNIF = OFF;

    /* Save I2C state. */
    new_sda = SDA;
    new_scl = SCL;

    /* Sample on SCL rising edge. */
    if (collect_data && !old_scl && new_scl) {
      if (data_bits < 8) {
        data_value = (data_value << 1) | new_sda;
        data_bits++;
      } else {
        if (interactive_mode) {
          /* Show data to a human. */
          bp_write_hex_byte_to_ringbuffer(data_value);
        } else {
          /* Report data via binary I/O. */
          user_serial_ringbuffer_append(I2C_SNIFFER_ESCAPE);
          user_serial_ringbuffer_append(data_value);
        }

        /* SDA high is NACK, SDA low is ACK. */
        user_serial_ringbuffer_append(new_sda ? I2C_SNIFFER_NACK
                                              : I2C_SNIFFER_ACK);

        /* Ready for next byte. */
        data_bits = 0;
      }
    } else {
      /* Data transition. */
      if (old_scl && new_scl) {
        /* Start condition. */
        if (old_sda && !new_sda) {
          collect_data = true;
          data_bits = 0;

          user_serial_ringbuffer_append(I2C_SNIFFER_START);
        } else {
          /* Stop condition. */
          if (!old_sda && new_sda) {
            collect_data = false;
            data_bits = 0;

            user_serial_ringbuffer_append(I2C_SNIFFER_STOP);
          }
        }
      }
    }

    /* Save last I2C state. */
    old_sda = new_sda;
    old_scl = new_scl;
  }

  /* Disable I2C pin change interrupts. */
  BP_MOSI_CN = OFF;
  BP_CLK_CN = OFF;

  if (interactive_mode) {
    bpBR;
  }
}

void handle_pending_ack(const bool bus_bit) {
  if (i2c_state.mode == I2C_TYPE_SOFTWARE) {
    bitbang_i2c_write_bit(bus_bit);
  } else {
#ifdef BP_I2C_USE_HW_BUS
    hardware_i2c_send_ack(bus_bit);
#endif /* BP_I2C_USE_HW_BUS */
  }
  i2c_state.acknowledgment_pending = false;
}

/*
rawI2C mode:
# 00000000//reset to BBIO
# 00000001 � mode version string (I2C1)
# 00000010 � I2C start bit
# 00000011 � I2C stop bit
# 00000100 - I2C read byte
# 00000110 - ACK bit
# 00000111 - NACK bit
# 0001xxxx � Bulk transfer, send 1-16 bytes (0=1byte!)
# (0110)000x - Set I2C speed, 3 = 400khz 2=100khz 1=50khz 0=5khz
# (0111)000x - Read speed, (planned)
# (0100)wxyz � Configure peripherals w=power, x=pullups, y=AUX, z=CS (was 0110)
# (0101)wxyz � read peripherals (planned, not implemented)
 */

void binary_io_enter_i2c_mode(void) {
  static unsigned char inByte, rawCommand, i;
  unsigned int fr;

  /*
   * The byte transport below (i2c_write()/i2c_read() ->
   * bitbang_write_value()/bitbang_read_value()) carries
   * mode_configuration.numbits bits per byte.  A width left over from
   * another mode - e.g. an interactive ;16 in a raw mode, whose exit path
   * does not run the mode cleanup - would turn every byte into a 16-bit
   * transfer: the wire carries a leading 0x00 byte and every slave NAKs
   * the address.  Pin the byte width at entry, the same way the raw-wire
   * binary handler in binary_io.c does.
   */
  mode_configuration.numbits = 8;
  mode_configuration.int16 = NO;

  SDA_TRIS = INPUT;
  SCL_TRIS = INPUT;
  SCL = LOW;
  SDA = LOW;

  // set CS pin direction to output on setup
  BP_CS_DIR = OUTPUT;

  mode_configuration.high_impedance = ON;
  mode_configuration.little_endian = NO;
  bitbang_setup(2, BITBANG_SPEED_MAXIMUM);
  bitbang_i2c_clear_error();
  MSG_I2C_MODE_IDENTIFIER;

  for (;;) {
    inByte = user_serial_read_byte();
    // get command bits in a separate variable
    rawCommand = (inByte >> 4);

    switch (rawCommand) {
    case 0: // reset/setup/config commands
      switch (inByte) {

      case 0:
        return;

      case 1: // 1 - id reply string
        MSG_I2C_MODE_IDENTIFIER;
        break;

      case 2: // I2C start bit
        bitbang_i2c_clear_error();
        user_serial_transmit_character(
            bitbang_i2c_start_wait(BITBANG_I2C_START_ONE_SHOT));
        break;

      case 3: // I2C stop bit
        bitbang_i2c_clear_error();
        user_serial_transmit_character(bitbang_i2c_stop_wait());
        break;

      case 4: // I2C read byte
        bitbang_i2c_clear_error();
        user_serial_transmit_character(bitbang_read_value(true));
        break;

      case 6: // I2C send ACK
        bitbang_i2c_clear_error();
        user_serial_transmit_character(bitbang_i2c_write_bit(LOW));
        break;

      case 7: // I2C send NACK
        bitbang_i2c_clear_error();
        user_serial_transmit_character(bitbang_i2c_write_bit(HIGH));
        break;

      case 0x0A: // Last bus operation status (read-byte framing stays compatible)
        user_serial_transmit_character(!bitbang_i2c_error());
        break;

      case 8: // write-then-read
        if (!i2c_write_then_read()) {
          REPORT_IO_FAILURE();
        }
        break; // 00001001 xxxxxxxx

      case 9: // extended AUX command
        // confirm that the command is known
        REPORT_IO_SUCCESS();
        // inByte - used as extended commmand
        // fr - used as result
        /*
         * Read the subcommand through the portable accessor: on v4 the
         * terminal link is the USB CDC, so the raw UART1 registers used
         * here originally would never receive a byte and the firmware
         * would hang until a replug (on v3 the accessor performs the same
         * raw UART1 read as before).
         */
        inByte = user_serial_read_byte();
        // 0x00 - AUX/CS low
        // 0x01 - AUX/CS high
        // 0x02 - AUX/CS HiZ
        // 0x03 - AUX read
        // 0x10 - use AUX
        // 0x20 - use CS
        fr = 1;
        switch (inByte) {

        case 0x00:
          bp_aux_pin_set_low();
          break;

        case 0x01:
          bp_aux_pin_set_high();
          break;

        case 0x02:
          bp_aux_pin_set_high_impedance();
          break;

        case 0x03:
          fr = bp_aux_pin_read();
          break;

        case 0x10:
          mode_configuration.alternate_aux = 0;
          break;

        case 0x20:
          mode_configuration.alternate_aux = 1;
          break;

        default:
          /* The sub-command is unknown: report failure. */
          fr = 0;
          break;
        }

        user_serial_transmit_character(fr); // result
        break;

      case 0b1111:
        i2c_sniffer(false);
        REPORT_IO_SUCCESS();
        break;

      default:
        REPORT_IO_FAILURE();
        break;
      }
      break;

    case 0b0001:            // get x+1 bytes
      bitbang_i2c_clear_error();
      inByte &= 0b00001111; // clear command portion
      inByte++;             // increment by 1, 0=1byte
      REPORT_IO_SUCCESS();

      for (i = 0; i < inByte; i++) {
        bitbang_write_value(
            user_serial_read_byte(), true); // JTR usb port //send byte
        user_serial_transmit_character(
            bitbang_read_bit(true)); // return ACK0 or NACK1
      }

      break;

    case 0b0110:                // set speed
      inByte &= 0b00000011;     // clear command portion
      bitbang_setup(2, inByte); // set I2C speed
      REPORT_IO_SUCCESS();
      break;

    case 0b0100: // configure peripherals w=power, x=pullups, y=AUX, z=CS
      bp_binary_io_peripherals_set(inByte);
      REPORT_IO_SUCCESS();
      break;

#ifdef BUSPIRATEV4
    case 0b0101:
      user_serial_transmit_character(bp_binary_io_pullup_control(inByte));
      break;
#endif /* BUSPIRATEV4 */

    default:
      REPORT_IO_FAILURE();
      break;
    }
  }
}

bool i2c_write_then_read(void) {
  bitbang_i2c_clear_error();
  /* Read the amount of bytes to write. */
  size_t bytes_to_write = user_serial_read_big_endian_word();

  /* Read the amount of bytes to read. */
  size_t bytes_to_read = user_serial_read_big_endian_word();

  /*
   * Make sure the payloads fit in the internal buffer: both the write and
   * the read payload are buffered in the terminal buffer, so both counts
   * must be bounded here (the read check used to test the write count,
   * leaving bytes_to_read host-controlled and unbounded).
   */
  if ((bytes_to_write > BP_TERMINAL_BUFFER_SIZE) ||
      (bytes_to_read > BP_TERMINAL_BUFFER_SIZE)) {
    return false;
  }

  /* The payload must include an address, even for a read-only transaction. */
  if (bytes_to_write == 0) {
    return false;
  }

  uint8_t i2c_address = 0;

  /* Read payload. */
  for (size_t index = 0; index < bytes_to_write; index++) {
    bus_pirate_configuration.terminal_input[index] = user_serial_read_byte();
  }

  /* Get I2C address. */
  i2c_address = bus_pirate_configuration.terminal_input[0];

  /* Signal write start. */
  if (!bitbang_i2c_start_wait(BITBANG_I2C_START_ONE_SHOT)) {
    return false;
  }

  /* Write the payload to the I2C bus. */
  for (size_t index = 0; index < bytes_to_write; index++) {
    bitbang_write_value(bus_pirate_configuration.terminal_input[index], true);

    if (bitbang_read_bit(true) == HIGH) {
      /* No ACK read on the bus, bailing out. */
      goto abort_transfer;
    }
  }

  if ((bytes_to_read > 0) && (bytes_to_write > 1)) {
    /* Send a restart signal on the I2C bus. */
    if (!bitbang_i2c_start_wait(BITBANG_I2C_RESTART)) {
      goto abort_transfer;
    }

    /* Send the I2C address. */
    bitbang_write_value(i2c_address | 0x01, true);

    if (bitbang_read_bit(true) == HIGH) {
      /* No ACK read on the bus, bailing out. */
      goto abort_transfer;
    }
  }

  /* Read the rest of the data. */
  bytes_to_write = bytes_to_read - 1;

  for (size_t index = 0; index < bytes_to_read; index++) {
    /* Read the byte from the I2C bus. */
    bus_pirate_configuration.terminal_input[index] = bitbang_read_value(true);

    if (bitbang_i2c_error()) {
      goto abort_transfer;
    }

    /* Report ACK or NACK depending on the length. */
    if (!bitbang_i2c_write_bit(index >= bytes_to_write ? HIGH : LOW)) {
      goto abort_transfer;
    }
  }

  /* Stop the I2C bus operations. */
  if (!bitbang_i2c_stop_wait()) {
    goto abort_transfer;
  }

  /* Report operation status. */
  REPORT_IO_SUCCESS();

  /* And send the I2C data over to the UART. */
  bp_write_buffer(&bus_pirate_configuration.terminal_input[0], bytes_to_read);

  return true;

abort_transfer:
  /* A timeout has already released both pins. A slave NACK still needs STOP. */
  if (!bitbang_i2c_error()) {
    bitbang_i2c_stop_wait();
  }
  return false;
}

#endif /* BP_ENABLE_I2C_SUPPORT */
