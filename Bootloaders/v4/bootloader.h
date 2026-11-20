/* ds30-compatible Bus Pirate v4 loader. GPL, as the original loader. */
#ifndef BOOTLOADER_H
#define BOOTLOADER_H
#include <stdint.h>
#define BLSTARTADDR 0x400UL
#define BLENDADDR 0x1FFFUL
#define APPLICATION_ADDRESS 0x2000u
#define CONFIG_PAGE 0x2A800UL
#define ROW_ADDRESSES 128u
#define PAGE_ADDRESSES 1024u
#define ROW_BYTES 192u
#define DEVICEID 241u /* ds30 protocol ID, not the silicon DEVID (0x1019). */
#define VER_H 5u
#define VER_L 0u
void boot_init(void);
void boot_poll(void);
extern uint8_t boot_restart;
uint8_t flash_row(uint32_t address, uint8_t *data, uint8_t erase);
#endif
