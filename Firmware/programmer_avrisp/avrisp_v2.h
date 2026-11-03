/**
 * @file avrisp_v2.h
 *
 * @brief AVRISP-MKII personality - V2 protocol (STK500v2) constants and
 * command handlers.  Ported from LUFA's AVRISP-MKII project
 * (Copyright (C) Dean Camera, 2021; MIT-style licence).
 */

#ifndef BP_AVRISP_V2_H
#define BP_AVRISP_V2_H

#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT

/* Command constants (AVR068 / LUFA V2ProtocolConstants.h). */
#define CMD_SIGN_ON                 0x01
#define CMD_SET_PARAMETER           0x02
#define CMD_GET_PARAMETER           0x03
#define CMD_OSCCAL                  0x05
#define CMD_LOAD_ADDRESS            0x06
#define CMD_FIRMWARE_UPGRADE        0x07
#define CMD_RESET_PROTECTION        0x0A
#define CMD_ENTER_PROGMODE_ISP      0x10
#define CMD_LEAVE_PROGMODE_ISP      0x11
#define CMD_CHIP_ERASE_ISP          0x12
#define CMD_PROGRAM_FLASH_ISP       0x13
#define CMD_READ_FLASH_ISP          0x14
#define CMD_PROGRAM_EEPROM_ISP      0x15
#define CMD_READ_EEPROM_ISP         0x16
#define CMD_PROGRAM_FUSE_ISP        0x17
#define CMD_READ_FUSE_ISP           0x18
#define CMD_PROGRAM_LOCK_ISP        0x19
#define CMD_READ_LOCK_ISP           0x1A
#define CMD_READ_SIGNATURE_ISP      0x1B
#define CMD_READ_OSCCAL_ISP         0x1C
#define CMD_SPI_MULTI               0x1D
#define CMD_XPROG                   0x50
#define CMD_XPROG_SETMODE           0x51

/* Status constants. */
#define STATUS_CMD_OK               0x00
#define STATUS_CMD_TOUT             0x80
#define STATUS_RDY_BSY_TOUT         0x81
#define STATUS_SET_PARAM_MISSING    0x82
#define STATUS_CMD_FAILED           0xC0
#define STATUS_CMD_UNKNOWN          0xC9
#define STATUS_CMD_ILLEGAL_PARAM    0xCA
#define STATUS_ISP_READY            0x00
#define STATUS_CONN_FAIL_MOSI       0x01
#define STATUS_CONN_FAIL_RST        0x02
#define STATUS_CONN_FAIL_SCK        0x04
#define STATUS_TGT_NOT_DETECTED     0x10
#define STATUS_TGT_REVERSE_INSERTED 0x20

/* Parameter constants. */
#define PARAM_BUILD_NUMBER_LOW      0x80
#define PARAM_BUILD_NUMBER_HIGH     0x81
#define PARAM_HW_VER                0x90
#define PARAM_SW_MAJOR              0x91
#define PARAM_SW_MINOR              0x92
#define PARAM_VTARGET               0x94
#define PARAM_SCK_DURATION          0x98
#define PARAM_RESET_POLARITY        0x9E
#define PARAM_STATUS_TGT_CONN       0xA1
#define PARAM_DISCHARGEDELAY        0xA4

/** Prepares the engine state for a new mode session. */
void avrisp_v2_initialise(void);

/** Processes one V2 protocol command from the active transport. */
void avrisp_v2_process_command(void);

/** Returns a parameter's current value. */
uint8_t avrisp_param_get(const uint8_t id);

/** Stores a parameter's value. */
void avrisp_param_set(const uint8_t id, const uint8_t value);

/** Current target address for FLASH/EEPROM operations. */
extern uint32_t avrisp_current_address;

/** True when the next memory operation must send LOAD EXTENDED ADDRESS. */
extern bool avrisp_load_extended_address;

/* ISP protocol command handlers. */
void avrisp_isp_enter_mode(void);
void avrisp_isp_leave_mode(void);
void avrisp_isp_program_memory(const uint8_t command);
void avrisp_isp_read_memory(const uint8_t command);
void avrisp_isp_chip_erase(void);
void avrisp_isp_calibrate(void);
void avrisp_isp_read_fuse_lock_osccal(const uint8_t command);
void avrisp_isp_write_fuse_lock(const uint8_t command);
void avrisp_isp_spi_multi(void);

/* XPROG protocol command handlers. */
void avrisp_xprog_set_mode(void);
void avrisp_xprog_command(void);

/* ISP mode plumbing (avrisp_isp.c). */
void avrisp_isp_initialise(void);
void avrisp_isp_shutdown(void);

#endif /* BP_ENABLE_AVRISP_MKII_SUPPORT */

#endif /* !BP_AVRISP_V2_H */