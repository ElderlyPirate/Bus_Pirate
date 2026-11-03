/**
 * @file avrisp_nvm_pdi.c
 *
 * @brief AVRISP-MKII personality - XMEGA target NVM layer (PDI).
 *
 * Port of LUFA's XMEGANVM.c (Copyright (C) Dean Camera, 2021;
 * MIT-style licence).
 */

#include <stdbool.h>

#include "base.h"
#include "avrisp_mkii.h"
#include "avrisp_v2.h"
#include "avrisp_xprog.h"

#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT

/** Sends the given 32-bit absolute address, LSB first. */
static void avrisp_xmega_send_address(const uint32_t address) {
  avrisp_xprog_send_byte(address & 0xFF);
  avrisp_xprog_send_byte(address >> 8);
  avrisp_xprog_send_byte(address >> 16);
  avrisp_xprog_send_byte(address >> 24);
}

/** Sends an NVM register address, offset from the configured NVM base. */
static void avrisp_xmega_send_nvm_reg_address(const uint8_t reg) {
  avrisp_xmega_send_address(avrisp_xprog_nvm_base + reg);
}

bool avrisp_xmega_wait_nvm_bus(void) {
  return avrisp_xprog_wait_status(PDI_CMD_LDCS(PDI_REG_STATUS),
                                 PDI_STATUS_NVM, PDI_STATUS_NVM);
}

bool avrisp_xmega_wait_nvm_controller(void) {
  /* Point the PDI pointer register at the NVM STATUS register. */
  avrisp_xprog_send_byte(PDI_CMD_ST(PDI_POINTER_DIRECT, PDI_DATASIZE_4BYTES));
  avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_STATUS);

  return avrisp_xprog_wait_status(
      PDI_CMD_LD(PDI_POINTER_INDIRECT, PDI_DATASIZE_1BYTE), 1 << 7, 0);
}

bool avrisp_xmega_enable_pdi(void) {
  avrisp_xprog_enable_target_pdi();

  /* Keep the XMEGA in reset via the RESET PDI register. */
  avrisp_xprog_send_byte(PDI_CMD_STCS(PDI_REG_RESET));
  avrisp_xprog_send_byte(PDI_RESET_KEY);

  /* Lower the direction change guard time to 32 PDI_CLK cycles. */
  avrisp_xprog_send_byte(PDI_CMD_STCS(PDI_REG_CTRL));
  avrisp_xprog_send_byte(0x02);

  /* Send the NVM enable key. */
  avrisp_xprog_send_nvm_key(PDI_CMD_KEY);

  return avrisp_xmega_wait_nvm_bus();
}

void avrisp_xmega_disable_pdi(void) {
  avrisp_xmega_wait_nvm_bus();

  /* Clear the RESET key; sometimes takes multiple writes (silicon). */
  uint8_t reset_value;

  do {
    avrisp_xprog_send_byte(PDI_CMD_STCS(PDI_REG_RESET));
    avrisp_xprog_send_byte(0x00);

    avrisp_xprog_send_byte(PDI_CMD_LDCS(PDI_REG_RESET));
    reset_value = avrisp_xprog_receive_byte();
  } while ((reset_value != 0x00) && avrisp_timeout_ticks);

  avrisp_xprog_disable_target_pdi();
}

bool avrisp_xmega_get_crc(const uint8_t crc_command, uint32_t *const crc) {
  *crc = 0;

  if (!avrisp_xmega_wait_nvm_controller()) {
    return false;
  }

  /* Set the NVM command to the requested CRC read. */
  avrisp_xprog_send_byte(PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
  avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
  avrisp_xprog_send_byte(crc_command);

  /* Set the CMDEX bit to start the CRC generation. */
  avrisp_xprog_send_byte(PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
  avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CTRLA);
  avrisp_xprog_send_byte(XMEGA_NVM_BIT_CTRLA_CMDEX);

  if (!avrisp_xmega_wait_nvm_bus()) {
    return false;
  }

  if (!avrisp_xmega_wait_nvm_controller()) {
    return false;
  }

  /* Point at the CRC data registers and read three bytes. */
  avrisp_xprog_send_byte(PDI_CMD_ST(PDI_POINTER_DIRECT, PDI_DATASIZE_4BYTES));
  avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_DAT0);

  avrisp_xprog_send_byte(PDI_CMD_REPEAT(PDI_DATASIZE_1BYTE));
  avrisp_xprog_send_byte(XMEGA_CRC_LENGTH_BYTES - 1);

  avrisp_xprog_send_byte(
      PDI_CMD_LD(PDI_POINTER_INDIRECT_PI, PDI_DATASIZE_1BYTE));

  for (uint8_t index = 0; index < XMEGA_CRC_LENGTH_BYTES; index++) {
    ((uint8_t *)crc)[index] = avrisp_xprog_receive_byte();
  }

  return (avrisp_timeout_ticks > 0);
}

bool avrisp_xmega_read_memory(const uint32_t address, uint8_t *buffer,
                              uint16_t size) {
  if (!size) {
    return true;
  }
  if (!avrisp_xmega_wait_nvm_controller()) {
    return false;
  }

  /* Arbitrary location reads use the READNVM command. */
  avrisp_xprog_send_byte(PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
  avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
  avrisp_xprog_send_byte(XMEGA_NVM_CMD_READNVM);

  if (size > 1) {
    avrisp_xprog_send_byte(
        PDI_CMD_ST(PDI_POINTER_DIRECT, PDI_DATASIZE_4BYTES));
    avrisp_xmega_send_address(address);

    avrisp_xprog_send_byte(PDI_CMD_REPEAT(PDI_DATASIZE_1BYTE));
    avrisp_xprog_send_byte(size - 1);

    avrisp_xprog_send_byte(
        PDI_CMD_LD(PDI_POINTER_INDIRECT_PI, PDI_DATASIZE_1BYTE));

    while (size-- && avrisp_timeout_ticks) {
      *(buffer++) = avrisp_xprog_receive_byte();
    }
  } else {
    avrisp_xprog_send_byte(
        PDI_CMD_LDS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_address(address);
    *(buffer++) = avrisp_xprog_receive_byte();
  }

  return (avrisp_timeout_ticks > 0);
}

bool avrisp_xmega_write_byte(const uint8_t command, const uint32_t address,
                             const uint8_t byte) {
  if (!avrisp_xmega_wait_nvm_controller()) {
    return false;
  }

  avrisp_xprog_send_byte(PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
  avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
  avrisp_xprog_send_byte(command);

  avrisp_xprog_send_byte(PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
  avrisp_xmega_send_address(address);
  avrisp_xprog_send_byte(byte);

  return avrisp_xmega_wait_nvm_controller();
}

bool avrisp_xmega_write_page(const uint8_t buffer_command,
                             const uint8_t erase_buffer_command,
                             const uint8_t write_page_command,
                             const uint8_t page_mode, const uint32_t address,
                             const uint8_t *buffer, uint16_t size) {
  if (page_mode & XPROG_PAGEMODE_ERASE) {
    if (!avrisp_xmega_wait_nvm_controller()) {
      return false;
    }

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
    avrisp_xprog_send_byte(erase_buffer_command);

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CTRLA);
    avrisp_xprog_send_byte(XMEGA_NVM_BIT_CTRLA_CMDEX);
  }

  if (size) {
    if (!avrisp_xmega_wait_nvm_controller()) {
      return false;
    }

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
    avrisp_xprog_send_byte(buffer_command);

    avrisp_xprog_send_byte(
        PDI_CMD_ST(PDI_POINTER_DIRECT, PDI_DATASIZE_4BYTES));
    avrisp_xmega_send_address(address);

    avrisp_xprog_send_byte(PDI_CMD_REPEAT(PDI_DATASIZE_1BYTE));
    avrisp_xprog_send_byte(size - 1);

    avrisp_xprog_send_byte(
        PDI_CMD_ST(PDI_POINTER_INDIRECT_PI, PDI_DATASIZE_1BYTE));

    while (size-- && avrisp_timeout_ticks) {
      avrisp_xprog_send_byte(*(buffer++));
    }
  }

  if (page_mode & XPROG_PAGEMODE_WRITE) {
    if (!avrisp_xmega_wait_nvm_controller()) {
      return false;
    }

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
    avrisp_xprog_send_byte(write_page_command);

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_address(address);
    avrisp_xprog_send_byte(0x00);
  }

  return avrisp_timeout_ticks && avrisp_xmega_wait_nvm_controller();
}

bool avrisp_xmega_erase_memory(const uint8_t erase_command,
                               const uint32_t address) {
  if (!avrisp_xmega_wait_nvm_controller()) {
    return false;
  }

  if (erase_command == XMEGA_NVM_CMD_CHIPERASE) {
    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
    avrisp_xprog_send_byte(erase_command);

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CTRLA);
    avrisp_xprog_send_byte(XMEGA_NVM_BIT_CTRLA_CMDEX);
  } else if (erase_command == XMEGA_NVM_CMD_ERASEEEPROM ||
             erase_command == XMEGA_NVM_CMD_ERASEEEPROMPAGE) {
    /* AU manual 33.4: both page and whole-EEPROM erases act only on
       locations tagged by loading the EEPROM page buffer. */
    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
    avrisp_xprog_send_byte(XMEGA_NVM_CMD_ERASEEEPROMPAGEBUFF);

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CTRLA);
    avrisp_xprog_send_byte(XMEGA_NVM_BIT_CTRLA_CMDEX);

    if (!avrisp_xmega_wait_nvm_controller()) {
      return false;
    }

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
    avrisp_xprog_send_byte(XMEGA_NVM_CMD_LOADEEPROMPAGEBUFF);

    avrisp_xprog_send_byte(
        PDI_CMD_ST(PDI_POINTER_DIRECT, PDI_DATASIZE_4BYTES));
    avrisp_xmega_send_address(address);

    avrisp_xprog_send_byte(PDI_CMD_REPEAT(PDI_DATASIZE_1BYTE));
    avrisp_xprog_send_byte(avrisp_xprog_eeprom_page_size - 1);

    avrisp_xprog_send_byte(
        PDI_CMD_ST(PDI_POINTER_INDIRECT_PI, PDI_DATASIZE_1BYTE));

    for (uint16_t page_byte = 0; page_byte < avrisp_xprog_eeprom_page_size &&
                                 avrisp_timeout_ticks;
         page_byte++) {
      avrisp_xprog_send_byte(0x00);
    }

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
    avrisp_xprog_send_byte(erase_command);

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    if (erase_command == XMEGA_NVM_CMD_ERASEEEPROM) {
      avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CTRLA);
      avrisp_xprog_send_byte(XMEGA_NVM_BIT_CTRLA_CMDEX);
    } else {
      avrisp_xmega_send_address(address);
      avrisp_xprog_send_byte(0x00);
    }
  } else {
    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_nvm_reg_address(XMEGA_NVM_REG_CMD);
    avrisp_xprog_send_byte(erase_command);

    avrisp_xprog_send_byte(
        PDI_CMD_STS(PDI_DATASIZE_4BYTES, PDI_DATASIZE_1BYTE));
    avrisp_xmega_send_address(address);
    avrisp_xprog_send_byte(0x00);
  }

  if (!avrisp_xmega_wait_nvm_bus()) {
    return false;
  }

  if (!avrisp_xmega_wait_nvm_controller()) {
    return false;
  }

  return true;
}

#endif /* BP_ENABLE_AVRISP_MKII_SUPPORT */
