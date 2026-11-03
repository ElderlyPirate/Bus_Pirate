/**
 * @file avrisp_xprog.h
 *
 * @brief AVRISP-MKII personality - XPROG (PDI/TPI) protocol constants,
 * physical layer and NVM layers.  Ported from LUFA's AVRISP-MKII project
 * (Copyright (C) Dean Camera, 2021; MIT-style licence).
 */

#ifndef BP_AVRISP_XPROG_H
#define BP_AVRISP_XPROG_H

#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT

/* XPROG wrapper constants (LUFA XPROGProtocol.h). */
#define XPROG_CMD_ENTER_PROGMODE             0x01
#define XPROG_CMD_LEAVE_PROGMODE             0x02
#define XPROG_CMD_ERASE                      0x03
#define XPROG_CMD_WRITE_MEM                  0x04
#define XPROG_CMD_READ_MEM                   0x05
#define XPROG_CMD_CRC                        0x06
#define XPROG_CMD_SET_PARAM                  0x07

#define XPROG_MEM_TYPE_APPL                  1
#define XPROG_MEM_TYPE_BOOT                  2
#define XPROG_MEM_TYPE_EEPROM                3
#define XPROG_MEM_TYPE_FUSE                  4
#define XPROG_MEM_TYPE_LOCKBITS              5
#define XPROG_MEM_TYPE_USERSIG               6
#define XPROG_MEM_TYPE_FACTORY_CALIBRATION   7

#define XPROG_ERASE_CHIP                     1
#define XPROG_ERASE_APP                      2
#define XPROG_ERASE_BOOT                     3
#define XPROG_ERASE_EEPROM                   4
#define XPROG_ERASE_APP_PAGE                 5
#define XPROG_ERASE_BOOT_PAGE                6
#define XPROG_ERASE_EEPROM_PAGE              7
#define XPROG_ERASE_USERSIG                  8
#define XPROG_ERASE_CONFIG                   9

#define XPROG_MEM_WRITE_ERASE                0
#define XPROG_MEM_WRITE_WRITE                1

#define XPROG_CRC_APP                        1
#define XPROG_CRC_BOOT                       2
#define XPROG_CRC_FLASH                      3

#define XPROG_ERR_OK                         0
#define XPROG_ERR_FAILED                     1
#define XPROG_ERR_COLLISION                  2
#define XPROG_ERR_TIMEOUT                    3

#define XPROG_PARAM_NVMBASE                  0x01
#define XPROG_PARAM_EEPPAGESIZE              0x02
#define XPROG_PARAM_NVMCMD_REG               0x03
#define XPROG_PARAM_NVMCSR_REG               0x04
#define XPROG_PARAM_UNKNOWN_1                0x05

#define XPROG_PROTOCOL_PDI                   0x00
#define XPROG_PROTOCOL_JTAG                  0x01
#define XPROG_PROTOCOL_TPI                   0x02

#define XPROG_PAGEMODE_WRITE                 (1 << 1)
#define XPROG_PAGEMODE_ERASE                 (1 << 0)

/* PDI constants (LUFA XPROGTarget.h). */
#define PDI_CMD_LDS(AddressSize, DataSize)  (0x00 | ((AddressSize) << 2) | (DataSize))
#define PDI_CMD_LD(PointerAccess, DataSize) (0x20 | ((PointerAccess) << 2) | (DataSize))
#define PDI_CMD_STS(AddressSize, DataSize)  (0x40 | ((AddressSize) << 2) | (DataSize))
#define PDI_CMD_ST(PointerAccess, DataSize) (0x60 | ((PointerAccess) << 2) | (DataSize))
#define PDI_CMD_LDCS(PDIReg)                (0x80 | (PDIReg))
#define PDI_CMD_REPEAT(DataSize)            (0xA0 | (DataSize))
#define PDI_CMD_STCS(PDIReg)                (0xC0 | (PDIReg))
#define PDI_CMD_KEY                         0xE0

#define PDI_REG_STATUS                       0
#define PDI_REG_RESET                        1
#define PDI_REG_CTRL                         2

#define PDI_STATUS_NVM                       (1 << 1)

#define PDI_RESET_KEY                        0x59

#define PDI_DATASIZE_1BYTE                   0
#define PDI_DATASIZE_2BYTES                  1
#define PDI_DATASIZE_3BYTES                  2
#define PDI_DATASIZE_4BYTES                  3

#define PDI_POINTER_INDIRECT                 0
#define PDI_POINTER_INDIRECT_PI              1
#define PDI_POINTER_DIRECT                   2

/* TPI constants (LUFA XPROGTarget.h). */
#define TPI_CMD_SLD(PointerAccess)          (0x20 | (PointerAccess))
#define TPI_CMD_SST(PointerAccess)          (0x60 | (PointerAccess))
#define TPI_CMD_SSTPR                        0x68
#define TPI_CMD_SIN(Address)                (0x10 | (((Address) & 0x30) << 1) | ((Address) & 0x0F))
#define TPI_CMD_SOUT(Address)               (0x90 | (((Address) & 0x30) << 1) | ((Address) & 0x0F))
#define TPI_CMD_SLDCS(TPIReg)               (0x80 | (TPIReg))
#define TPI_CMD_SSTCS(TPIReg)               (0xC0 | (TPIReg))
#define TPI_CMD_SKEY                         0xE0

#define TPI_REG_STATUS                       0x00
#define TPI_REG_CTRL                         0x02
#define TPI_REG_ID                           0x0F

#define TPI_STATUS_NVM                       (1 << 1)

#define TPI_POINTER_INDIRECT                 0
#define TPI_POINTER_INDIRECT_PI              4

/* Shared XPROG parameters and state (avrisp_xprog.c). */
extern uint32_t avrisp_xprog_nvm_base;
extern uint16_t avrisp_xprog_eeprom_page_size;
extern uint8_t  avrisp_xprog_nvm_cmd_reg;
extern uint8_t  avrisp_xprog_nvm_csr_reg;
extern uint8_t  avrisp_xprog_selected_protocol;

/* Physical layer (avrisp_xprog.c). */
void    avrisp_xprog_initialise(void);
void    avrisp_xprog_shutdown(void);
void    avrisp_xprog_pause_clock(void);
void    avrisp_xprog_resume_clock(void);
void    avrisp_xprog_enable_target_pdi(void);
void    avrisp_xprog_enable_target_tpi(void);
void    avrisp_xprog_disable_target_pdi(void);
void    avrisp_xprog_disable_target_tpi(void);
void    avrisp_xprog_send_byte(const uint8_t byte);
uint8_t avrisp_xprog_receive_byte(void);

/** Common PDI/TPI NVM key and timeout-aware status polling transactions. */
void avrisp_xprog_send_nvm_key(uint8_t instruction);
bool avrisp_xprog_wait_status(uint8_t instruction, uint8_t mask,
                             uint8_t ready_value);
void    avrisp_xprog_send_idle(void);

/* XMEGA NVM register/command constants (LUFA XMEGANVM.h). */
#define XMEGA_CRC_LENGTH_BYTES               3
#define XMEGA_NVM_REG_ADDR0                  0x00
#define XMEGA_NVM_REG_ADDR1                  0x01
#define XMEGA_NVM_REG_ADDR2                  0x02
#define XMEGA_NVM_REG_DAT0                   0x04
#define XMEGA_NVM_REG_DAT1                   0x05
#define XMEGA_NVM_REG_DAT2                   0x06
#define XMEGA_NVM_REG_CMD                    0x0A
#define XMEGA_NVM_REG_CTRLA                  0x0B
#define XMEGA_NVM_REG_CTRLB                  0x0C
#define XMEGA_NVM_REG_INTCTRL                0x0D
#define XMEGA_NVM_REG_STATUS                 0x0F
#define XMEGA_NVM_REG_LOCKBITS               0x10
#define XMEGA_NVM_BIT_CTRLA_CMDEX            (1 << 0)
#define XMEGA_NVM_CMD_NOOP                   0x00
#define XMEGA_NVM_CMD_CHIPERASE              0x40
#define XMEGA_NVM_CMD_READNVM                0x43
#define XMEGA_NVM_CMD_LOADFLASHPAGEBUFF      0x23
#define XMEGA_NVM_CMD_ERASEFLASHPAGEBUFF     0x26
#define XMEGA_NVM_CMD_ERASEFLASHPAGE         0x2B
#define XMEGA_NVM_CMD_WRITEFLASHPAGE         0x2E
#define XMEGA_NVM_CMD_ERASEWRITEFLASH        0x2F
#define XMEGA_NVM_CMD_FLASHCRC               0x78
#define XMEGA_NVM_CMD_ERASEAPPSEC            0x20
#define XMEGA_NVM_CMD_ERASEAPPSECPAGE        0x22
#define XMEGA_NVM_CMD_WRITEAPPSECPAGE        0x24
#define XMEGA_NVM_CMD_ERASEWRITEAPPSECPAGE   0x25
#define XMEGA_NVM_CMD_APPCRC                 0x38
#define XMEGA_NVM_CMD_ERASEBOOTSEC           0x68
#define XMEGA_NVM_CMD_ERASEBOOTSECPAGE       0x2A
#define XMEGA_NVM_CMD_WRITEBOOTSECPAGE       0x2C
#define XMEGA_NVM_CMD_ERASEWRITEBOOTSECPAGE  0x2D
#define XMEGA_NVM_CMD_BOOTCRC                0x39
#define XMEGA_NVM_CMD_READUSERSIG            0x01
#define XMEGA_NVM_CMD_ERASEUSERSIG           0x18
#define XMEGA_NVM_CMD_WRITEUSERSIG           0x1A
#define XMEGA_NVM_CMD_READCALIBRATION        0x02
#define XMEGA_NVM_CMD_READFUSE               0x07
#define XMEGA_NVM_CMD_WRITEFUSE              0x4C
#define XMEGA_NVM_CMD_WRITELOCK              0x08
#define XMEGA_NVM_CMD_LOADEEPROMPAGEBUFF     0x33
#define XMEGA_NVM_CMD_ERASEEEPROMPAGEBUFF    0x36
#define XMEGA_NVM_CMD_ERASEEEPROM            0x30
#define XMEGA_NVM_CMD_ERASEEEPROMPAGE        0x32
#define XMEGA_NVM_CMD_WRITEEEPROMPAGE        0x34
#define XMEGA_NVM_CMD_ERASEWRITEEEPROMPAGE   0x35
#define XMEGA_NVM_CMD_READEEPROM             0x06

/* TINY NVM command constants (LUFA TINYNVM.h). */
#define TINY_NVM_CMD_NOOP              0x00
#define TINY_NVM_CMD_CHIPERASE         0x10
#define TINY_NVM_CMD_SECTIONERASE      0x14
#define TINY_NVM_CMD_WORDWRITE         0x1D

/* XMEGA (PDI) NVM layer (avrisp_nvm_pdi.c). */
bool avrisp_xmega_wait_nvm_bus(void);
bool avrisp_xmega_wait_nvm_controller(void);
bool avrisp_xmega_enable_pdi(void);
void avrisp_xmega_disable_pdi(void);
bool avrisp_xmega_get_crc(const uint8_t crc_command, uint32_t *const crc);
bool avrisp_xmega_read_memory(const uint32_t address, uint8_t *buffer, uint16_t size);
bool avrisp_xmega_write_byte(const uint8_t command, const uint32_t address, const uint8_t byte);
bool avrisp_xmega_write_page(const uint8_t buffer_command, const uint8_t erase_buffer_command,
                             const uint8_t write_page_command, const uint8_t page_mode,
                             const uint32_t address, const uint8_t *buffer, uint16_t size);
bool avrisp_xmega_erase_memory(const uint8_t erase_command, const uint32_t address);

/* TINY (TPI) NVM layer (avrisp_nvm_tpi.c). */
bool avrisp_tiny_wait_nvm_bus(void);
bool avrisp_tiny_wait_nvm_controller(void);
bool avrisp_tiny_enable_tpi(void);
void avrisp_tiny_disable_tpi(void);
bool avrisp_tiny_read_memory(const uint16_t address, uint8_t *buffer, uint16_t size);
bool avrisp_tiny_write_memory(const uint16_t address, uint8_t *buffer, uint16_t size);
bool avrisp_tiny_erase_memory(const uint8_t erase_command, const uint16_t address);

#endif /* BP_ENABLE_AVRISP_MKII_SUPPORT */

#endif /* !BP_AVRISP_XPROG_H */
