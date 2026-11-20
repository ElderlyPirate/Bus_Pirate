/* ds30-compatible Bus Pirate loader; GPL, as the original ds30 loader. */
#include "bootloader.h"
#include "usb_stack.h"
#include "cdc.h"

static uint8_t header[5], row[ROW_BYTES];
static uint16_t received, last_byte;
static uint8_t checksum, active, erase_pending;
static uint32_t erase_page;
uint8_t boot_restart;

void boot_init(void) {
    received = 0;
    checksum = active = erase_pending = boot_restart = 0;
}

static uint8_t command(void) {
    uint32_t address = ((uint32_t)header[0] << 16) |
                       ((uint16_t)header[1] << 8) | header[2];
    uint8_t cmd = header[3], size = header[4], erase;
    if (checksum) {
        erase_pending = 0;
        return 'N';
    }
    if (cmd == 0xFF && size == 1) {
        erase_pending = 0;
        boot_restart = 1;
        return 'K';
    }
    if (!((cmd == 1 && size == 1) || (cmd == 2 && size == ROW_BYTES + 1))) {
        erase_pending = 0;
        return 'U';
    }
    /* All addresses are PIC program/table addresses, never Intel HEX offsets.
     * DS39897C 5.2: 64-instruction rows, 512-instruction erase pages. */
    if ((address & (ROW_ADDRESSES - 1)) || address >= CONFIG_PAGE ||
        (address >= BLSTARTADDR && address <= BLENDADDR) ||
        (cmd == 1 && (address & (PAGE_ADDRESSES - 1)))) {
        erase_pending = 0;
        return 'P';
    }
    if (cmd == 1) {
        erase_page = address;
        erase_pending = 1;
        return 'K';
    }
    erase = erase_pending;
    erase_pending = 0;
    if (erase && (address & ~(uint32_t)(PAGE_ADDRESSES - 1)) != erase_page)
        return 'P';
    /* Page zero contains vectors, but the loader starts on the next page. */
    if (address == 0) {
        row[0] = 0x04;
        row[1] = (uint8_t)BLSTARTADDR;
        row[2] = (uint8_t)(BLSTARTADDR >> 8);
        row[3] = 0;
        row[4] = (uint8_t)(BLSTARTADDR >> 16);
        row[5] = 0;
    }
    return flash_row(address, row, erase);
}

void boot_poll(void) {
    uint8_t byte, reply[4];
    if (!usb_configured() || cdc_session_changed()) {
        boot_init();
        return;
    }
    if (boot_restart || !cdc_tx_ready()) return;
    if (!cdc_get(&byte)) {
        if (received && (uint16_t)(usb_millis() - last_byte) >= 1000u)
            boot_init(); /* Incomplete command: require a fresh hello, no NVM. */
        return;
    }
    last_byte = usb_millis();
    if (!active) {
        if (byte == 0xC1) {
            active = 1;
            reply[0] = DEVICEID;
            reply[1] = VER_H;
            reply[2] = VER_L;
            reply[3] = 'K';
            cdc_send(reply, 4);
        }
        return;
    }
    checksum += byte;
    if (received < 5) header[received] = byte;
    else if (received < ROW_BYTES + 5u) row[received - 5] = byte;
    ++received;
    if (received >= 5 && (!header[4] || received == (uint16_t)(5 + header[4]))) {
        reply[0] = !header[4] ? 'U' : command();
        if (!header[4]) erase_pending = 0;
        cdc_send(reply, 1);
        received = 0;
        checksum = 0;
    }
}
