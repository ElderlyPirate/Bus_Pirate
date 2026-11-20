/* PIC24FJ256GB106 RTSP, DS39897C 5.6.1, examples 5-1 through 5-6. */
#include <xc.h>
#include "bootloader.h"

const unsigned int userversion
    __attribute__((space(prog), address(0x1FF6))) = (VER_H << 8) | VER_L;

static uint8_t commit(unsigned int operation) {
    NVMCON = operation;
    __builtin_disi(5);
    __builtin_write_NVM(); /* Unlock, WR, and the two required NOPs. */
    while (NVMCONbits.WR) {}
    NVMCONbits.WREN = 0;
    return !NVMCONbits.WRERR;
}

uint8_t flash_row(uint32_t address, uint8_t *data, uint8_t erase) {
    unsigned int offset = (unsigned int)address, i, low;
    TBLPAG = address >> 16;
    if (erase) {
        __builtin_tblwtl(offset, 0xFFFF);
        if (!commit(0x4042)) return 'V';
    }
    for (i = 0; i < ROW_BYTES; i += 3, offset += 2) {
        low = data[i + 1] | ((unsigned int)data[i + 2] << 8);
        __builtin_tblwth(offset, data[i]);
        __builtin_tblwtl(offset, low);
    }
    if (!commit(0x4001)) return 'V';
    offset = (unsigned int)address;
    for (i = 0; i < ROW_BYTES; i += 3, offset += 2) {
        low = __builtin_tblrdl(offset);
        if ((__builtin_tblrdh(offset) & 0xFF) != data[i] ||
            (uint8_t)low != data[i + 1] || (uint8_t)(low >> 8) != data[i + 2])
            return 'V';
    }
    return 'K';
}
