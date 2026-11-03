/**
 * @file avrisp_nvm_tpi.c
 *
 * @brief AVRISP-MKII personality - TINY target NVM layer (TPI).
 *
 * Port of LUFA's TINYNVM.c (Copyright (C) Dean Camera, 2021;
 * MIT-style licence).
 */

#include <stdbool.h>

#include "base.h"
#include "avrisp_mkii.h"
#include "avrisp_v2.h"
#include "avrisp_xprog.h"

#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT

/** Sends the given 16-bit pointer address, LSB first. */
static void avrisp_tiny_send_pointer_address(const uint16_t address) {
  avrisp_xprog_send_byte(TPI_CMD_SSTPR | 0);
  avrisp_xprog_send_byte(address & 0xFF);
  avrisp_xprog_send_byte(TPI_CMD_SSTPR | 1);
  avrisp_xprog_send_byte(address >> 8);
}

/** Sends a SOUT command for the given I/O address, ready to write data. */
static void avrisp_tiny_send_write_nvm_register(const uint8_t address) {
  avrisp_xprog_send_byte(TPI_CMD_SOUT(address));
}

bool avrisp_tiny_wait_nvm_bus(void) {
  return avrisp_xprog_wait_status(TPI_CMD_SLDCS(TPI_REG_STATUS),
                                 TPI_STATUS_NVM, TPI_STATUS_NVM);
}

bool avrisp_tiny_wait_nvm_controller(void) {
  return avrisp_xprog_wait_status(TPI_CMD_SIN(avrisp_xprog_nvm_csr_reg),
                                 1 << 7, 0);
}

bool avrisp_tiny_enable_tpi(void) {
  avrisp_xprog_enable_target_tpi();

  /* Lower the direction change guard time to 32 TPI_CLK cycles. */
  avrisp_xprog_send_byte(TPI_CMD_SSTCS(TPI_REG_CTRL));
  avrisp_xprog_send_byte(0x02);

  /* Send the NVM enable key. */
  avrisp_xprog_send_nvm_key(TPI_CMD_SKEY);

  return avrisp_tiny_wait_nvm_bus();
}

void avrisp_tiny_disable_tpi(void) {
  avrisp_tiny_wait_nvm_bus();

  /* Clear the NVMEN bit in the STATUS register to disable TPI mode. */
  uint8_t status_value;

  do {
    avrisp_xprog_send_byte(TPI_CMD_SSTCS(TPI_REG_STATUS));
    avrisp_xprog_send_byte(0x00);

    avrisp_xprog_send_byte(TPI_CMD_SLDCS(TPI_REG_STATUS));
    status_value = avrisp_xprog_receive_byte();
  } while ((status_value != 0x00) && avrisp_timeout_ticks);

  avrisp_xprog_disable_target_tpi();
}

bool avrisp_tiny_read_memory(const uint16_t address, uint8_t *buffer,
                             uint16_t size) {
  if (!avrisp_tiny_wait_nvm_controller()) {
    return false;
  }

  /* Memory reads use the NO OP command. */
  avrisp_tiny_send_write_nvm_register(avrisp_xprog_nvm_cmd_reg);
  avrisp_xprog_send_byte(TINY_NVM_CMD_NOOP);

  avrisp_tiny_send_pointer_address(address);

  while (size-- && avrisp_timeout_ticks) {
    avrisp_xprog_send_byte(TPI_CMD_SLD(TPI_POINTER_INDIRECT_PI));
    *(buffer++) = avrisp_xprog_receive_byte();
  }

  return (avrisp_timeout_ticks > 0);
}

bool avrisp_tiny_write_memory(const uint16_t address, uint8_t *buffer,
                              uint16_t size) {
  if (!avrisp_tiny_wait_nvm_controller()) {
    return false;
  }

  /* Must have an integer number of words; pad an odd byte with 0xFF. */
  if (size & 0x01) {
    buffer[size++] = 0xFF;
  }

  avrisp_tiny_send_write_nvm_register(avrisp_xprog_nvm_cmd_reg);
  avrisp_xprog_send_byte(TINY_NVM_CMD_WORDWRITE);

  avrisp_tiny_send_pointer_address(address);

  while (size) {
    if (!avrisp_tiny_wait_nvm_controller()) {
      return false;
    }

    avrisp_xprog_send_byte(TPI_CMD_SST(TPI_POINTER_INDIRECT_PI));
    avrisp_xprog_send_byte(*(buffer++));

    avrisp_xprog_send_byte(TPI_CMD_SST(TPI_POINTER_INDIRECT_PI));
    avrisp_xprog_send_byte(*(buffer++));

    size -= 2;
  }

  return avrisp_tiny_wait_nvm_controller();
}

bool avrisp_tiny_erase_memory(const uint8_t erase_command,
                              const uint16_t address) {
  if (!avrisp_tiny_wait_nvm_controller()) {
    return false;
  }

  avrisp_tiny_send_write_nvm_register(avrisp_xprog_nvm_cmd_reg);
  avrisp_xprog_send_byte(erase_command);

  /* Write to a high byte location to start the erase. */
  avrisp_tiny_send_pointer_address(address | 0x0001);
  avrisp_xprog_send_byte(TPI_CMD_SST(TPI_POINTER_INDIRECT));
  avrisp_xprog_send_byte(0x00);

  if (!avrisp_tiny_wait_nvm_controller()) {
    return false;
  }

  return true;
}

#endif /* BP_ENABLE_AVRISP_MKII_SUPPORT */
