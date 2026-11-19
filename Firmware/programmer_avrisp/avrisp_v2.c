/**
 * @file avrisp_v2.c
 *
 * @brief AVRISP-MKII personality - V2 protocol (STK500v2) engine.
 *
 * This is the port of LUFA's V2Protocol.c / V2ProtocolParams.c handler
 * set (Copyright (C) Dean Camera, 2021; MIT-style licence).  All AVR
 * device specific sequences (programming opcodes, polling commands)
 * are supplied by the host; this layer only executes them.
 */

#include <stdbool.h>

#include "base.h"
#include "avrisp_mkii.h"
#include "avrisp_v2.h"
#include "avrisp_xprog.h"

#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT

/** Parameter privilege masks. */
#define PARAM_PRIV_READ  (1 << 0)
#define PARAM_PRIV_WRITE (1 << 1)

/** Version reported to the host (LUFA parity, expected by Atmel Studio). */
#define AVRISP_FIRMWARE_VERSION_MINOR 0x18

typedef struct {
  const uint8_t id;
  const uint8_t privileges;
  uint8_t value;
  const uint8_t default_value;
} avrisp_param_t;

/** Current memory address for FLASH/EEPROM operations. */
uint32_t avrisp_current_address;

/** True when the next memory operation must send LOAD EXTENDED ADDRESS. */
bool avrisp_load_extended_address;

static avrisp_param_t avrisp_params[] = {
    {PARAM_BUILD_NUMBER_LOW, PARAM_PRIV_READ, 0, 0},
    {PARAM_BUILD_NUMBER_HIGH, PARAM_PRIV_READ, 0, 0},
    {PARAM_HW_VER, PARAM_PRIV_READ, 0x00, 0x00},
    {PARAM_SW_MAJOR, PARAM_PRIV_READ, 0x01, 0x01},
    {PARAM_SW_MINOR, PARAM_PRIV_READ, AVRISP_FIRMWARE_VERSION_MINOR,
     AVRISP_FIRMWARE_VERSION_MINOR},
    {PARAM_VTARGET, PARAM_PRIV_READ, 0x21, 0x21},
    {PARAM_SCK_DURATION, PARAM_PRIV_READ | PARAM_PRIV_WRITE, 6, 6},
    {PARAM_RESET_POLARITY, PARAM_PRIV_READ | PARAM_PRIV_WRITE, 0x01, 0x01},
    {PARAM_STATUS_TGT_CONN, PARAM_PRIV_READ, STATUS_ISP_READY, STATUS_ISP_READY},
    {PARAM_DISCHARGEDELAY, PARAM_PRIV_READ | PARAM_PRIV_WRITE, 0x00, 0x00},
};

static avrisp_param_t *avrisp_param_find(const uint8_t id) {
  for (uint8_t index = 0;
       index < (sizeof(avrisp_params) / sizeof(avrisp_params[0])); index++) {
    if (avrisp_params[index].id == id) {
      return &avrisp_params[index];
    }
  }

  return NULL;
}

uint8_t avrisp_param_get(const uint8_t id) {
  avrisp_param_t *param = avrisp_param_find(id);
  return param ? param->value : 0;
}

void avrisp_param_set(const uint8_t id, const uint8_t value) {
  avrisp_param_t *param = avrisp_param_find(id);

  if (param) {
    param->value = value;
  }
}

void avrisp_v2_initialise(void) {
  avrisp_current_address = 0;
  avrisp_load_extended_address = false;

  for (uint8_t index = 0;
       index < (sizeof(avrisp_params) / sizeof(avrisp_params[0])); index++) {
    avrisp_params[index].value = avrisp_params[index].default_value;
  }

  /* Configure the ADC used for the VTARGET measurement. */
  bp_adc_pin_setup();
  AD1CON1 = 0x00E0; /* Unsigned integer, automatic conversion, manual sample. */
  AD1CSSL = 0x0000;
  AD1CON3 = 0x1F02;
  AD1CON2 = 0x0000;
}

/** Refreshes the VTARGET parameter from the voltage probe input. */
static void avrisp_v2_refresh_vtarget(void) {
  bp_enable_adc();
  uint16_t adc = bp_read_adc(BP_ADC_PROBE);
  bp_disable_adc();

  /* Probe divider 2:1, AVdd=3.3 V; round to the nearest 0.1 V. */
  uint16_t value = ((uint32_t)adc * 66 + 512) / 1024;

  if (value > 255) {
    value = 255;
  }

  avrisp_param_set(PARAM_VTARGET, (uint8_t)value);
}

/** Handler for the CMD_SIGN_ON command. */
static void avrisp_v2_sign_on(void) {
  /* The USB personality must identify as an AVRISP mkII; the serial
     fallback must identify as an STK500v2 so the host keeps the framed
     serial protocol path. */
  const char *identifier = avrisp_is_usb() ? "AVRISP_MK2" : "STK500_2";
  uint8_t length = 0;

  while (identifier[length]) {
    length++;
  }

  avrisp_response_begin();
  avrisp_write_byte(CMD_SIGN_ON);
  avrisp_write_byte(STATUS_CMD_OK);
  avrisp_write_byte(length);

  for (uint8_t index = 0; index < length; index++) {
    avrisp_write_byte(identifier[index]);
  }

  avrisp_response_end();
}

/** Handler for the CMD_RESET_PROTECTION command (dummy acknowledgement). */
static void avrisp_v2_reset_protection(void) {
  avrisp_response_begin();
  avrisp_write_byte(CMD_RESET_PROTECTION);
  avrisp_write_byte(STATUS_CMD_OK);
  avrisp_response_end();
}

/** Handler for the CMD_SET_PARAMETER and CMD_GET_PARAMETER commands. */
static void avrisp_v2_get_set_param(const uint8_t command) {
  uint8_t param_id = avrisp_read_byte();
  uint8_t param_value = 0;

  if (command == CMD_SET_PARAMETER) {
    param_value = avrisp_read_byte();
  }

  avrisp_param_t *param = avrisp_param_find(param_id);

  avrisp_response_begin();
  avrisp_write_byte(command);

  if ((command == CMD_SET_PARAMETER) && param &&
      (param->privileges & PARAM_PRIV_WRITE)) {
    avrisp_write_byte(STATUS_CMD_OK);
    param->value = param_value;
  } else if ((command == CMD_GET_PARAMETER) && param &&
             (param->privileges & PARAM_PRIV_READ)) {
    avrisp_write_byte(STATUS_CMD_OK);
    avrisp_write_byte(param->value);
  } else {
    avrisp_write_byte(STATUS_CMD_FAILED);
  }

  avrisp_response_end();
}

/** Handler for the CMD_LOAD_ADDRESS command. */
static void avrisp_v2_load_address(void) {
  uint32_t address = 0;

  for (uint8_t index = 0; index < sizeof(address); index++) {
    address = (address << 8) | avrisp_read_byte();
  }

  avrisp_current_address = address;

  if (address & 0x80000000UL) {
    avrisp_load_extended_address = true;
  }

  avrisp_response_begin();
  avrisp_write_byte(CMD_LOAD_ADDRESS);
  avrisp_write_byte(STATUS_CMD_OK);
  avrisp_response_end();
}

/** Handler for unknown V2 protocol commands. */
static void avrisp_v2_unknown_command(const uint8_t command) {
  avrisp_discard_incoming();

  avrisp_response_begin();
  avrisp_write_byte(command);
  avrisp_write_byte(STATUS_CMD_UNKNOWN);
  avrisp_response_end();
}

/* Validate the complete transfer before a handler can touch the target. */
static bool avrisp_v2_valid_request(const uint8_t command) {
  const uint8_t *p = avrisp_command_buffer;
  uint16_t size = avrisp_request_size();
  uint16_t expected = 1;
  uint16_t count;

  if (size > AVRISP_BUFFER_SIZE) {
    return false;
  }
  switch (command) {
  case CMD_SIGN_ON:
  case CMD_OSCCAL:
  case CMD_RESET_PROTECTION:
    break;
  case CMD_GET_PARAMETER:
  case CMD_XPROG_SETMODE:
    expected = 2;
    break;
  case CMD_SET_PARAMETER:
  case CMD_LEAVE_PROGMODE_ISP:
    expected = 3;
    break;
  case CMD_LOAD_ADDRESS:
  case CMD_PROGRAM_FUSE_ISP:
  case CMD_PROGRAM_LOCK_ISP:
    expected = 5;
    break;
  case CMD_READ_FUSE_ISP:
  case CMD_READ_LOCK_ISP:
  case CMD_READ_SIGNATURE_ISP:
  case CMD_READ_OSCCAL_ISP:
    if (size != 6 || p[1] < 1 || p[1] > 4) {
      return false;
    }
    expected = 6;
    break;
  case CMD_ENTER_PROGMODE_ISP:
    if (size != 12 || p[7] > 4) {
      return false;
    }
    expected = 12;
    break;
  case CMD_CHIP_ERASE_ISP:
    expected = 7;
    break;
  case CMD_PROGRAM_FLASH_ISP:
  case CMD_PROGRAM_EEPROM_ISP:
  case CMD_READ_FLASH_ISP:
  case CMD_READ_EEPROM_ISP:
    if (size < 4) {
      return false;
    }
    count = ((uint16_t)p[1] << 8) | p[2];
    if (count > 256) {
      return false;
    }
    expected = (command == CMD_PROGRAM_FLASH_ISP ||
                command == CMD_PROGRAM_EEPROM_ISP) ? 10 + count : 4;
    break;
  case CMD_SPI_MULTI:
    if (size < 4) {
      return false;
    }
    expected = 4u + p[1];
    break;
  case CMD_XPROG:
    if (size < 2) {
      return false;
    }
    switch (p[1]) {
    case XPROG_CMD_ENTER_PROGMODE:
    case XPROG_CMD_LEAVE_PROGMODE:
      expected = 2;
      break;
    case XPROG_CMD_ERASE:
      expected = 7;
      break;
    case XPROG_CMD_WRITE_MEM:
      if (size < 10) {
        return false;
      }
      count = ((uint16_t)p[8] << 8) | p[9];
      if (count > 256) {
        return false;
      }
      expected = 10 + count;
      break;
    case XPROG_CMD_READ_MEM:
      if (size != 9 || (((uint16_t)p[7] << 8) | p[8]) > 256) {
        return false;
      }
      expected = 9;
      break;
    case XPROG_CMD_CRC:
      expected = 3;
      break;
    case XPROG_CMD_SET_PARAM:
      if (size < 3) {
        return false;
      }
      switch (p[2]) {
      case XPROG_PARAM_NVMBASE: expected = 7; break;
      case XPROG_PARAM_EEPPAGESIZE:
      case XPROG_PARAM_UNKNOWN_1: expected = 5; break;
      case XPROG_PARAM_NVMCMD_REG:
      case XPROG_PARAM_NVMCSR_REG: expected = 4; break;
      default: return true;
      }
      break;
    default:
      return true;
    }
    break;
  default:
    return true;
  }
  return size == expected;
}

void avrisp_v2_process_command(void) {
  avrisp_v2_refresh_vtarget();

  avrisp_timeout_ticks = AVRISP_TIMEOUT_TICKS;

  uint8_t command = avrisp_read_byte();

  if (!avrisp_v2_valid_request(command)) {
    avrisp_response_begin();
    avrisp_write_byte(command);
    if (command == CMD_XPROG) {
      avrisp_write_byte(avrisp_request_size() >= 2 ?
                        avrisp_command_buffer[1] : 0);
      avrisp_write_byte(XPROG_ERR_FAILED);
    } else {
      avrisp_write_byte(STATUS_CMD_FAILED);
    }
    avrisp_response_end();
    return;
  }

  switch (command) {
  case CMD_SIGN_ON:
    avrisp_v2_sign_on();
    break;

  case CMD_SET_PARAMETER:
  case CMD_GET_PARAMETER:
    avrisp_v2_get_set_param(command);
    break;

  case CMD_LOAD_ADDRESS:
    avrisp_v2_load_address();
    break;

  case CMD_RESET_PROTECTION:
    avrisp_v2_reset_protection();
    break;

  case CMD_ENTER_PROGMODE_ISP:
    avrisp_isp_enter_mode();
    break;

  case CMD_LEAVE_PROGMODE_ISP:
    avrisp_isp_leave_mode();
    break;

  case CMD_PROGRAM_FLASH_ISP:
  case CMD_PROGRAM_EEPROM_ISP:
    avrisp_isp_program_memory(command);
    break;

  case CMD_READ_FLASH_ISP:
  case CMD_READ_EEPROM_ISP:
    avrisp_isp_read_memory(command);
    break;

  case CMD_CHIP_ERASE_ISP:
    avrisp_isp_chip_erase();
    break;

  case CMD_OSCCAL:
    avrisp_isp_calibrate();
    break;

  case CMD_READ_FUSE_ISP:
  case CMD_READ_LOCK_ISP:
  case CMD_READ_SIGNATURE_ISP:
  case CMD_READ_OSCCAL_ISP:
    avrisp_isp_read_fuse_lock_osccal(command);
    break;

  case CMD_PROGRAM_FUSE_ISP:
  case CMD_PROGRAM_LOCK_ISP:
    avrisp_isp_write_fuse_lock(command);
    break;

  case CMD_SPI_MULTI:
    avrisp_isp_spi_multi();
    break;

  case CMD_XPROG_SETMODE:
    avrisp_xprog_set_mode();
    break;

  case CMD_XPROG:
    avrisp_xprog_command();
    break;

  default:
    avrisp_v2_unknown_command(command);
    break;
  }
  avrisp_xprog_resume_clock();
}

#endif /* BP_ENABLE_AVRISP_MKII_SUPPORT */
