/**
 * @file avrisp_isp.c
 *
 * @brief AVRISP-MKII personality - ISP protocol handlers and physical layer.
 *
 * Port of LUFA's ISPProtocol.c / ISPTarget.c (Copyright (C) Dean Camera,
 * 2021; Copyright 2019 Jacob September (jacobseptember [at] gmail [dot] com);
 * MIT-style licence).  Hardware SPI runs through the PIC24 SPI
 * module with PPS routing; the slow end of the AVR069 speed ladder uses a
 * blocking software SPI.  The target reset line is the AUX pin and the
 * 4 MHz rescue clock is generated on the CS pin by output compare 5.
 */

#include <stdbool.h>

#include "base.h"
#include "avrisp_mkii.h"
#include "avrisp_v2.h"
#include "avrisp_xprog.h"

#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT

/** Mask for high byte selection in a memory operation. */
#define READ_WRITE_HIGH_BYTE_MASK (1 << 3)

/** Programming mode completion check masks (AVR068). */
#define PROG_MODE_PAGED_WRITES_MASK    (1 << 0)
#define PROG_MODE_WORD_TIMEDELAY_MASK  (1 << 1)
#define PROG_MODE_WORD_VALUE_MASK      (1 << 2)
#define PROG_MODE_WORD_READYBUSY_MASK  (1 << 3)
#define PROG_MODE_PAGED_TIMEDELAY_MASK (1 << 4)
#define PROG_MODE_PAGED_VALUE_MASK     (1 << 5)
#define PROG_MODE_PAGED_READYBUSY_MASK (1 << 6)
#define PROG_MODE_COMMIT_PAGE_MASK     (1 << 7)

/** Low level device command to issue an extended FLASH address. */
#define LOAD_EXTENDED_ADDRESS_CMD 0x4D

/** Highest SCK duration index supported by the AVR069 speed ladder. */
#define ISP_MAX_SCK_DURATION 163

/** Hardware SPI settings for SCK duration indices 0..6 (8 MHz .. 125 kHz). */
static const uint8_t avrisp_isp_hardware_speeds[] = {
    0b00011011, /*   8 MHz - PPRE 1:1  / SPRE 2:1 */
    0b00011110, /*   4 MHz - PPRE 4:1  / SPRE 1:1 */
    0b00011010, /*   2 MHz - PPRE 4:1  / SPRE 2:1 */
    0b00011101, /*   1 MHz - PPRE 16:1 / SPRE 1:1 */
    0b00011001, /* 500 kHz - PPRE 16:1 / SPRE 2:1 */
    0b00011100, /* 250 kHz - PPRE 64:1 / SPRE 1:1 */
    0b00011000, /* 125 kHz - PPRE 64:1 / SPRE 2:1 */
};

/* Half-period in microseconds for SCK durations 7..163 (AVR069 ladder). */
static const uint16_t avrisp_soft_sck_half_us[] = {
    5, 6, 6, 6, 7, 7, 7, 8, 8, 9,
    9, 10, 10, 10, 11, 12, 12, 13, 13, 14,
    15, 15, 16, 17, 18, 18, 19, 20, 21, 22,
    24, 25, 26, 27, 28, 30, 31, 33, 34, 36,
    38, 40, 42, 43, 46, 48, 50, 53, 55, 58,
    61, 64, 67, 70, 73, 77, 81, 85, 89, 93,
    98, 103, 108, 113, 119, 124, 131, 137, 144, 151,
    158, 166, 174, 183, 191, 201, 211, 222, 232, 244,
    256, 268, 281, 295, 310, 325, 341, 358, 375, 393,
    413, 433, 454, 477, 500, 525, 550, 577, 605, 635,
    667, 699, 733, 769, 808, 847, 888, 933, 978, 1027,
    1075, 1129, 1185, 1244, 1302, 1366, 1433, 1506, 1577, 1656,
    1736, 1825, 1916, 2008, 2101, 2212, 2315, 2427, 2551, 2674,
    2809, 2941, 3086, 3247, 3401, 3571, 3731, 3906, 4098, 4310,
    4505, 4762, 5000, 5241, 5501, 5774, 6053, 6353, 6667, 6993,
    7331, 7692, 8078, 8475, 8881, 9328, 9785,
};

/** True when the ISP interface uses the hardware SPI module. */
static bool avrisp_isp_hardware_spi;

/** Currently selected SCK duration index. */
static uint8_t avrisp_isp_sck_duration;

/* -------------------------------------------------------------------------
 * Rescue clock (4 MHz on the CS pin).
 * ------------------------------------------------------------------------- */

static void avrisp_isp_rescue_clock_enable(void) {
  /* Same pattern as the PWM helper in binary_io.c: output compare 5 on
     timer 2 with the output routed to the CS pin. */
  T2CON = 0x0000;
  OC5CON = 0x0000;
  TMR2 = 0;

  /* Enhanced OC: OCxRS is the period, OCxR the duty, self-synchronized. */
  OC5CON2 = 0x001F;
  OC5TMR = 0;
  OC5R = 2;
  OC5RS = 3;

  BP_CS_RPOUT = OC5_IO;
  BP_CS_DIR = OUTPUT;

  PR2 = 3; /* Four 62.5 ns ticks: 4 MHz, 50% duty cycle. */

  OC5CON = 0x0006; /* PWM mode, no fault pin. */
  T2CONbits.TON = ON;
}

static void avrisp_isp_rescue_clock_disable(void) {
  T2CONbits.TON = OFF;
  T2CON = 0x0000;
  OC5CON = 0x0000;
  OC5CON2 = 0;

  BP_CS_RPOUT = 0;
  AVRISP_CS_LAT = LOW;
  BP_CS_DIR = INPUT;
}

/* -------------------------------------------------------------------------
 * Physical layer.
 * ------------------------------------------------------------------------- */

/** Blocking delay in milliseconds, bounded by the command watchdog. */
static void avrisp_isp_delay_ms(uint8_t milliseconds) {
  while (milliseconds-- && avrisp_timeout_ticks) {
    bp_delay_ms(1);
  }
}

/** Blocking delay in microseconds, bounded by the command watchdog. */
static void avrisp_isp_delay_us_checked(uint16_t microseconds) {
  while (microseconds && avrisp_timeout_ticks) {
    uint16_t chunk = (microseconds > 1000) ? 1000 : microseconds;
    bp_delay_us(chunk);
    microseconds -= chunk;
  }
}

static uint8_t avrisp_isp_transfer_hardware(const uint8_t value) {
  IFS0bits.SPI1IF = 0;
  SPI1BUF = value;

  while (!IFS0bits.SPI1IF && avrisp_timeout_ticks) {
  }

  return SPI1BUF;
}

static uint8_t avrisp_isp_transfer_software(const uint8_t value) {
  uint16_t half_period = avrisp_soft_sck_half_us[avrisp_isp_sck_duration - 7];
  uint8_t data = value;
  uint8_t received = 0;

  for (uint8_t bit = 0; bit < 8; bit++) {
    AVRISP_CLK_LAT = LOW;
    AVRISP_MOSI_LAT = (data & 0x80) ? HIGH : LOW;
    data <<= 1;
    avrisp_isp_delay_us_checked(half_period);

    AVRISP_CLK_LAT = HIGH;
    avrisp_isp_delay_us_checked(half_period);
    received = (received << 1) | ((BP_MISO == HIGH) ? 1 : 0);

    if (!avrisp_timeout_ticks) {
      break;
    }
  }

  AVRISP_CLK_LAT = LOW;
  return received;
}

static uint8_t avrisp_isp_transfer_byte(const uint8_t value) {
  if (!avrisp_timeout_ticks) {
    return 0xFF;
  }
  if (avrisp_isp_hardware_spi) {
    return avrisp_isp_transfer_hardware(value);
  }

  return avrisp_isp_transfer_software(value);
}

static void avrisp_isp_send_byte(const uint8_t value) {
  (void)avrisp_isp_transfer_byte(value);
}

static uint8_t avrisp_isp_receive_byte(void) {
  return avrisp_isp_transfer_byte(0x00);
}

static void avrisp_isp_enable_target(void) {
  avrisp_isp_sck_duration = avrisp_param_get(PARAM_SCK_DURATION);

  if (avrisp_isp_sck_duration > ISP_MAX_SCK_DURATION) {
    avrisp_isp_sck_duration = ISP_MAX_SCK_DURATION;
  }

  SPI1STATbits.SPIEN = OFF;

  if (avrisp_isp_sck_duration <
      (sizeof(avrisp_isp_hardware_speeds) /
       sizeof(avrisp_isp_hardware_speeds[0]))) {
    avrisp_isp_hardware_spi = true;

    RPINR20bits.SDI1R = BP_MISO_RPIN;
    BP_MOSI_RPOUT = SDO1_IO;
    BP_CLK_RPOUT = SCK1OUT_IO;

    BP_MISO_DIR = INPUT;
    BP_MOSI_DIR = OUTPUT;
    BP_CLK_DIR = OUTPUT;

    /* ISP uses SPI mode 0: clock idle low, data sampled on the rising
       edge (LUFA: SPI_ORDER_MSB_FIRST, SPI_SCK_LEAD_RISING). */
    SPI1CON1 = (avrisp_isp_hardware_speeds[avrisp_isp_sck_duration]
                << _SPI1CON1_PPRE_POSITION) |
               (ON << _SPI1CON1_MSTEN_POSITION) |
               (1 << _SPI1CON1_CKE_POSITION);
    SPI1CON2 = 0x0000;
    SPI1STAT = 0x0000;
    SPI1STATbits.SPIEN = ON;
  } else {
    avrisp_isp_hardware_spi = false;

    RPINR20bits.SDI1R = 0b11111;
    BP_MOSI_RPOUT = 0;
    BP_CLK_RPOUT = 0;

    BP_MISO_DIR = INPUT;
    BP_MOSI_DIR = OUTPUT;
    BP_CLK_DIR = OUTPUT;
    AVRISP_CLK_LAT = LOW;
  }
}

static void avrisp_isp_disable_target(void) {
  SPI1STATbits.SPIEN = OFF;

  RPINR20bits.SDI1R = 0b11111;
  BP_MOSI_RPOUT = 0;
  BP_CLK_RPOUT = 0;

  AVRISP_MOSI_LAT = LOW;
  BP_MOSI_DIR = INPUT;
  AVRISP_CLK_LAT = LOW;
  BP_CLK_DIR = INPUT;
  BP_MISO_DIR = INPUT;
}

/**
 * Asserts or releases the target's reset line, using the polarity set by
 * the host.  When not asserted the line is tristated.
 */
static void avrisp_isp_change_reset_line(const bool assert) {
  if (assert) {
    if (!avrisp_param_get(PARAM_RESET_POLARITY)) {
      AVRISP_AUX_LAT = HIGH;
    } else {
      AVRISP_AUX_LAT = LOW;
    }
    BP_AUX0_DIR = OUTPUT;
  } else {
    BP_AUX0_DIR = INPUT;
    AVRISP_AUX_LAT = LOW;
  }
}

static uint8_t avrisp_isp_wait_while_busy(void) {
  do {
    avrisp_isp_send_byte(0xF0);
    avrisp_isp_send_byte(0x00);
    avrisp_isp_send_byte(0x00);
  } while ((avrisp_isp_receive_byte() & 0x01) && avrisp_timeout_ticks);

  return avrisp_timeout_ticks ? STATUS_CMD_OK : STATUS_RDY_BSY_TOUT;
}

static void avrisp_isp_load_extended_address(void) {
  avrisp_isp_send_byte(LOAD_EXTENDED_ADDRESS_CMD);
  avrisp_isp_send_byte(0x00);
  avrisp_isp_send_byte(avrisp_current_address >> 16);
  avrisp_isp_send_byte(0x00);
}

static uint8_t avrisp_isp_wait_for_prog_complete(
    const uint8_t programming_mode, const uint16_t poll_address,
    const uint8_t poll_value, const uint8_t delay_ms,
    const uint8_t read_memory_command) {
  uint8_t status = STATUS_CMD_OK;

  switch (programming_mode & ~(PROG_MODE_PAGED_WRITES_MASK |
                               PROG_MODE_COMMIT_PAGE_MASK)) {
  case PROG_MODE_WORD_TIMEDELAY_MASK:
  case PROG_MODE_PAGED_TIMEDELAY_MASK:
    avrisp_isp_delay_ms(delay_ms);
    if (!avrisp_timeout_ticks) {
      status = STATUS_CMD_TOUT;
    }
    break;

  case PROG_MODE_WORD_VALUE_MASK:
  case PROG_MODE_PAGED_VALUE_MASK:
    do {
      avrisp_isp_send_byte(read_memory_command);
      avrisp_isp_send_byte(poll_address >> 8);
      avrisp_isp_send_byte(poll_address & 0xFF);
    } while ((avrisp_isp_transfer_byte(0x00) == poll_value) &&
             avrisp_timeout_ticks);

    if (!avrisp_timeout_ticks) {
      status = STATUS_CMD_TOUT;
    }
    break;

  case PROG_MODE_WORD_READYBUSY_MASK:
  case PROG_MODE_PAGED_READYBUSY_MASK:
    status = avrisp_isp_wait_while_busy();
    break;
  }

  /* Programming complete: LUFA resets the timeout at this point. */
  avrisp_timeout_ticks = AVRISP_TIMEOUT_TICKS;

  return status;
}

/* -------------------------------------------------------------------------
 * ISP mode management (called by the mode plumbing).
 * ------------------------------------------------------------------------- */

void avrisp_isp_initialise(void) {
  SPI1STATbits.SPIEN = OFF;
  BP_MOSI_RPOUT = 0;
  BP_CLK_RPOUT = 0;
  BP_MISO_RPOUT = 0;
  BP_AUX_RPOUT = 0;
  BP_MOSI_ODC = 0;
  BP_CLK_ODC = 0;
  BP_MISO_ODC = 0;
  BP_CS_ODC = 0;
  BP_AUX_ODC = 0;
  avrisp_isp_rescue_clock_enable();
}

void avrisp_isp_shutdown(void) {
  avrisp_isp_change_reset_line(false);
  avrisp_isp_disable_target();
  avrisp_isp_rescue_clock_disable();
}

/* -------------------------------------------------------------------------
 * ISP protocol command handlers.
 * ------------------------------------------------------------------------- */

void avrisp_isp_enter_mode(void) {
  uint8_t timeout_ms = avrisp_read_byte();
  uint8_t pin_stab_delay = avrisp_read_byte();
  uint8_t execution_delay = avrisp_read_byte();
  uint8_t synch_loops = avrisp_read_byte();
  uint8_t byte_delay = avrisp_read_byte();
  uint8_t poll_value = avrisp_read_byte();
  uint8_t poll_index = avrisp_read_byte();
  uint8_t enter_programming_bytes[4];

  for (uint8_t index = 0; index < sizeof(enter_programming_bytes); index++) {
    enter_programming_bytes[index] = avrisp_read_byte();
  }

  (void)timeout_ms;

  avrisp_response_begin();

  uint8_t response_status = STATUS_CMD_FAILED;

  avrisp_current_address = 0;
  avrisp_load_extended_address = false;
  avrisp_xprog_shutdown();

  avrisp_isp_delay_ms(execution_delay);
  avrisp_isp_enable_target();

  avrisp_isp_change_reset_line(true);
  avrisp_isp_delay_ms(pin_stab_delay);

  while (synch_loops-- && avrisp_timeout_ticks) {
    uint8_t response_bytes[4];

    for (uint8_t index = 0; index < sizeof(response_bytes); index++) {
      avrisp_isp_delay_ms(byte_delay);
      response_bytes[index] =
          avrisp_isp_transfer_byte(enter_programming_bytes[index]);
    }

    if (!poll_index || (response_bytes[poll_index - 1] == poll_value)) {
      response_status = STATUS_CMD_OK;
      break;
    }

    avrisp_isp_change_reset_line(false);
    avrisp_isp_delay_ms(pin_stab_delay);
    avrisp_isp_change_reset_line(true);
    avrisp_isp_delay_ms(pin_stab_delay);
  }

  avrisp_write_byte(CMD_ENTER_PROGMODE_ISP);
  avrisp_write_byte(response_status);
  avrisp_response_end();
}

void avrisp_isp_leave_mode(void) {
  uint8_t pre_delay = avrisp_read_byte();
  uint8_t post_delay = avrisp_read_byte();

  avrisp_response_begin();

  avrisp_isp_delay_ms(pre_delay);
  avrisp_isp_change_reset_line(false);
  avrisp_isp_disable_target();
  avrisp_isp_delay_ms(post_delay);

  avrisp_write_byte(CMD_LEAVE_PROGMODE_ISP);
  avrisp_write_byte(STATUS_CMD_OK);
  avrisp_response_end();
}

void avrisp_isp_program_memory(const uint8_t command) {
  uint8_t size_high = avrisp_read_byte();
  uint8_t size_low = avrisp_read_byte();
  uint16_t bytes_to_write = ((uint16_t)size_high << 8) | size_low;
  uint8_t programming_mode = avrisp_read_byte();
  uint8_t delay_ms = avrisp_read_byte();
  uint8_t programming_commands[3];
  uint8_t poll_value1;
  uint8_t poll_value2;

  for (uint8_t index = 0; index < sizeof(programming_commands); index++) {
    programming_commands[index] = avrisp_read_byte();
  }

  poll_value1 = avrisp_read_byte();
  poll_value2 = avrisp_read_byte();

  if (bytes_to_write > 256) {
    avrisp_response_begin();
    avrisp_write_byte(command);
    avrisp_write_byte(STATUS_CMD_FAILED);
    avrisp_response_end();
    return;
  }

  for (uint16_t index = 0; index < bytes_to_write; index++) {
    avrisp_command_buffer[index] = avrisp_read_byte();
  }

  /* May consume the transfer-terminating ZLP before the reply. */
  avrisp_response_begin();

  uint8_t programming_status = STATUS_CMD_OK;
  uint8_t poll_value = (command == CMD_PROGRAM_FLASH_ISP) ? poll_value1
                                                          : poll_value2;
  uint16_t poll_address = 0;
  bool poll_address_valid = false;
  uint16_t page_start_address = (avrisp_current_address & 0xFFFF);

  for (uint16_t current_byte = 0; current_byte < bytes_to_write;
       current_byte++) {
    uint8_t byte_to_write = avrisp_command_buffer[current_byte];
    uint8_t byte_mode = programming_mode;

    if (avrisp_load_extended_address) {
      avrisp_isp_load_extended_address();
      avrisp_load_extended_address = false;
    }

    avrisp_isp_send_byte(programming_commands[0]);
    avrisp_isp_send_byte(avrisp_current_address >> 8);
    avrisp_isp_send_byte(avrisp_current_address & 0xFF);
    avrisp_isp_send_byte(byte_to_write);

    if (!avrisp_timeout_ticks) {
      programming_status = STATUS_CMD_TOUT;
      break;
    }

    /* FLASH addressing writes the high byte on odd byte indices. */
    if (command == CMD_PROGRAM_FLASH_ISP) {
      programming_commands[0] ^= READ_WRITE_HIGH_BYTE_MASK;
    }

    if (!poll_address_valid && (byte_to_write != poll_value)) {
      if ((current_byte & 0x01) && (command == CMD_PROGRAM_FLASH_ISP)) {
        programming_commands[2] |= READ_WRITE_HIGH_BYTE_MASK;
      } else {
        programming_commands[2] &= ~READ_WRITE_HIGH_BYTE_MASK;
      }

      poll_address = (avrisp_current_address & 0xFFFF);
      poll_address_valid = true;
    }

    if (!(programming_mode & PROG_MODE_PAGED_WRITES_MASK)) {
      if (!poll_address_valid &&
          !(programming_mode & PROG_MODE_WORD_READYBUSY_MASK)) {
        byte_mode = (programming_mode & ~PROG_MODE_WORD_VALUE_MASK) |
                           PROG_MODE_WORD_TIMEDELAY_MASK;
      }

      programming_status = avrisp_isp_wait_for_prog_complete(
          byte_mode, poll_address, poll_value, delay_ms,
          programming_commands[2]);

      if (programming_status != STATUS_CMD_OK) {
        break;
      }

      poll_address = 0;
      poll_address_valid = false;
    }

    if ((current_byte & 0x01) || (command == CMD_PROGRAM_EEPROM_ISP)) {
      avrisp_current_address++;

      if ((command == CMD_PROGRAM_FLASH_ISP) &&
          !(avrisp_current_address & 0xFFFF)) {
        avrisp_load_extended_address = true;
      }
    }
  }

  if ((programming_mode & PROG_MODE_COMMIT_PAGE_MASK) &&
      programming_status == STATUS_CMD_OK) {
    avrisp_isp_send_byte(programming_commands[1]);
    avrisp_isp_send_byte(page_start_address >> 8);
    avrisp_isp_send_byte(page_start_address & 0xFF);
    avrisp_isp_send_byte(0x00);

    if ((programming_mode & PROG_MODE_PAGED_VALUE_MASK) && !poll_address_valid) {
      programming_mode = (programming_mode & ~PROG_MODE_PAGED_VALUE_MASK) |
                         PROG_MODE_PAGED_TIMEDELAY_MASK;
    }

    programming_status = avrisp_isp_wait_for_prog_complete(
        programming_mode, poll_address, poll_value, delay_ms,
        programming_commands[2]);

    if ((command == CMD_PROGRAM_FLASH_ISP) &&
        !(avrisp_current_address & 0xFFFF)) {
      avrisp_load_extended_address = true;
    }
  }

  avrisp_write_byte(command);
  avrisp_write_byte(programming_status);
  avrisp_response_end();
}

void avrisp_isp_read_memory(const uint8_t command) {
  uint8_t size_high = avrisp_read_byte();
  uint8_t size_low = avrisp_read_byte();
  uint16_t bytes_to_read = ((uint16_t)size_high << 8) | size_low;
  uint8_t read_memory_command = avrisp_read_byte();

  /* The response buffer bounds the transfer; hosts use at most 256. */
  if (bytes_to_read > 256) {
    bytes_to_read = 256;
  }

  avrisp_response_begin();
  avrisp_write_byte(command);
  avrisp_write_byte(STATUS_CMD_OK);

  for (uint16_t current_byte = 0; current_byte < bytes_to_read;
       current_byte++) {
    if (avrisp_load_extended_address) {
      avrisp_isp_load_extended_address();
      avrisp_load_extended_address = false;
    }

    avrisp_isp_send_byte(read_memory_command);
    avrisp_isp_send_byte(avrisp_current_address >> 8);
    avrisp_isp_send_byte(avrisp_current_address & 0xFF);
    avrisp_write_byte(avrisp_isp_receive_byte());

    /* FLASH addressing reads the high byte on odd byte indices. */
    if (command == CMD_READ_FLASH_ISP) {
      read_memory_command ^= READ_WRITE_HIGH_BYTE_MASK;
    }

    if ((current_byte & 0x01) || (command == CMD_READ_EEPROM_ISP)) {
      avrisp_current_address++;

      if ((command != CMD_READ_EEPROM_ISP) &&
          !(avrisp_current_address & 0xFFFF)) {
        avrisp_load_extended_address = true;
      }
    }
  }

  avrisp_write_byte(avrisp_timeout_ticks ? STATUS_CMD_OK : STATUS_CMD_TOUT);
  avrisp_response_end();
}

void avrisp_isp_chip_erase(void) {
  uint8_t erase_delay = avrisp_read_byte();
  uint8_t poll_method = avrisp_read_byte();
  uint8_t erase_commands[4];

  for (uint8_t index = 0; index < sizeof(erase_commands); index++) {
    erase_commands[index] = avrisp_read_byte();
  }

  avrisp_response_begin();

  uint8_t response_status = STATUS_CMD_OK;

  for (uint8_t index = 0; index < sizeof(erase_commands); index++) {
    avrisp_isp_send_byte(erase_commands[index]);
  }

  if (!poll_method) {
    avrisp_isp_delay_ms(erase_delay);
    if (!avrisp_timeout_ticks) {
      response_status = STATUS_CMD_TOUT;
    }
  } else {
    response_status = avrisp_isp_wait_while_busy();
  }

  avrisp_write_byte(CMD_CHIP_ERASE_ISP);
  avrisp_write_byte(response_status);
  avrisp_response_end();
}

void avrisp_isp_calibrate(void) {
  /* AVR053 section 2: a hardware C-clock avoids software/USB ISR jitter. */
  uint8_t response_status = STATUS_CMD_OK;
  avrisp_xprog_shutdown();

  SPI1STATbits.SPIEN = OFF;
  BP_MOSI_RPOUT = 0;
  BP_CLK_RPOUT = 0;
  BP_MOSI_DIR = OUTPUT;
  BP_CLK_DIR = INPUT;
  BP_MISO_DIR = INPUT;

  avrisp_response_begin();

  avrisp_isp_rescue_clock_disable();
  TMR2 = 0;
  PR2 = 487; /* 16 MHz / 488 = 32786.9 Hz (+0.058% vs 32768 Hz). */
  OC5R = 244;
  OC5CON2 = 0x001F;
  OC5TMR = 0;
  OC5RS = 487;
  BP_MOSI_RPOUT = OC5_IO;
  OC5CON = 0x0006;
  IFS0bits.T2IF = 0;
  T2CONbits.TON = ON;

  uint16_t saved_pullup = CNPU4bits.CN52PUE;
  CNPU4bits.CN52PUE = 1;

  /* Release ~RESET. */
  avrisp_isp_change_reset_line(false);

  uint16_t cycles_remaining = 1024 + 25;
  uint8_t toggles_remaining = 8;
  bool previous_level = (BP_MISO == HIGH);

  while (cycles_remaining && toggles_remaining && avrisp_timeout_ticks) {
    if (IFS0bits.T2IF) {
      IFS0bits.T2IF = 0;
      cycles_remaining--;
    }
    bool current_level = (BP_MISO == HIGH);

    if (current_level != previous_level) {
      toggles_remaining--;
      previous_level = current_level;
    }
  }

  if (toggles_remaining) {
    response_status = STATUS_CMD_TOUT;
  }

  OC5CON = 0;
  BP_MOSI_RPOUT = 0;
  AVRISP_MOSI_LAT = LOW;
  BP_MOSI_DIR = INPUT;
  CNPU4bits.CN52PUE = saved_pullup;
  avrisp_isp_rescue_clock_enable();

  avrisp_write_byte(CMD_OSCCAL);
  avrisp_write_byte(response_status);
  avrisp_response_end();
}

void avrisp_isp_read_fuse_lock_osccal(const uint8_t command) {
  uint8_t return_byte = avrisp_read_byte();
  uint8_t read_commands[4];
  uint8_t response_bytes[4];

  for (uint8_t index = 0; index < sizeof(read_commands); index++) {
    read_commands[index] = avrisp_read_byte();
  }

  avrisp_response_begin();

  for (uint8_t index = 0; index < sizeof(response_bytes); index++) {
    response_bytes[index] = avrisp_isp_transfer_byte(read_commands[index]);
  }

  avrisp_write_byte(command);
  avrisp_write_byte(STATUS_CMD_OK);
  avrisp_write_byte((return_byte >= 1 && return_byte <= 4)
                        ? response_bytes[return_byte - 1]
                        : 0x00);
  avrisp_write_byte(STATUS_CMD_OK);
  avrisp_response_end();
}

void avrisp_isp_write_fuse_lock(const uint8_t command) {
  uint8_t write_commands[4];

  for (uint8_t index = 0; index < sizeof(write_commands); index++) {
    write_commands[index] = avrisp_read_byte();
  }

  avrisp_response_begin();

  for (uint8_t index = 0; index < sizeof(write_commands); index++) {
    avrisp_isp_send_byte(write_commands[index]);
  }

  avrisp_write_byte(command);
  avrisp_write_byte(STATUS_CMD_OK);
  avrisp_write_byte(STATUS_CMD_OK);
  avrisp_response_end();
}

void avrisp_isp_spi_multi(void) {
  uint8_t tx_bytes = avrisp_read_byte();
  uint8_t rx_bytes = avrisp_read_byte();
  uint8_t rx_start = avrisp_read_byte();

  for (uint8_t index = 0; index < tx_bytes; index++) {
    avrisp_command_buffer[index] = avrisp_read_byte();
  }

  avrisp_response_begin();
  avrisp_write_byte(CMD_SPI_MULTI);
  avrisp_write_byte(STATUS_CMD_OK);

  uint16_t tx_position = 0;
  uint8_t rx_position = 0;

  while (tx_position < rx_start) {
    avrisp_isp_send_byte((tx_position < tx_bytes)
                             ? avrisp_command_buffer[tx_position]
                             : 0x00);
    tx_position++;
  }

  while (rx_position < rx_bytes) {
    uint8_t received =
        (tx_position < tx_bytes)
            ? avrisp_isp_transfer_byte(avrisp_command_buffer[tx_position++])
            : avrisp_isp_receive_byte();

    avrisp_write_byte(received);
    rx_position++;
  }

  while (tx_position < tx_bytes) {
    avrisp_isp_send_byte(avrisp_command_buffer[tx_position++]);
  }

  avrisp_write_byte(STATUS_CMD_OK);
  avrisp_response_end();
}

#endif /* BP_ENABLE_AVRISP_MKII_SUPPORT */
