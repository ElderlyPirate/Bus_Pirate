/* Derived from Honken/JTR CDC, CC BY 3.0; see COPYING-USB.txt. */
#include <string.h>
#include "usb_stack.h"
#include "cdc.h"

static uint8_t line_coding[7];
static uint8_t rx[64], tx[64], notice[10];
static uint8_t position, available, changed;

void cdc_init(void) {
    static const uint8_t defaults[7] = {0, 0xC2, 1, 0, 0, 0, 8};
    memcpy(line_coding, defaults, 7);
    cdc_reset();
}

void cdc_reset(void) {
    position = available = 0;
    changed = 1;
}

uint8_t cdc_session_changed(void) {
    uint8_t result = changed;
    changed = 0;
    return result;
}

void cdc_configure(void) {
    cdc_reset();
    usb_bdt[3].BDADDR = notice;
    usb_bdt[3].BDCNT = 0;
    usb_bdt[3].BDSTAT = DTS;
    usb_bdt[4].BDADDR = rx;
    usb_bdt[4].BDCNT = sizeof(rx);
    usb_bdt[4].BDSTAT = UOWN | DTSEN;
    usb_bdt[5].BDADDR = tx;
    usb_bdt[5].BDCNT = 0;
    usb_bdt[5].BDSTAT = DTS;
    USB_UEP1 = USB_EP_IN;
    USB_UEP2 = USB_EP_INOUT;
}

uint8_t cdc_setup(const uint8_t *s) {
    uint16_t value = usb_word(s + 2), length = usb_word(s + 6);
    if (!usb_configured() || usb_word(s + 4)) return 0;
    if (s[0] == 0x21 && s[1] == 0x20 && !value && length == 7) {
        usb_expect_line_coding();
        return 1;
    }
    if (s[0] == 0xA1 && s[1] == 0x21 && !value) {
        usb_reply(line_coding, 7, length);
        return 1;
    }
    if (s[0] == 0x21 && s[1] == 0x22 && !(value & ~3u) && !length) {
        usb_reply(0, 0, 0); /* DTR/RTS are virtual metadata, not board I/O. */
        return 1;
    }
    return 0;
}

uint8_t cdc_line_data(const uint8_t *data, uint8_t length) {
    if (length != 7 || data[4] > 2 || data[5] > 4 ||
        !(data[6] == 5 || data[6] == 6 || data[6] == 7 || data[6] == 8 || data[6] == 16))
        return 0;
    memcpy(line_coding, data, 7);
    return 1;
}

uint8_t cdc_get(uint8_t *byte) {
    BDentry *bd = &usb_bdt[4];
    if (!usb_configured() || usb_halted(4)) return 0;
    if (!available) {
        if (bd->BDSTAT & UOWN) return 0;
        available = bd->BDCNT;
        if (available > sizeof(rx)) available = 0;
        position = 0;
        if (!available) {
            bd->BDCNT = sizeof(rx);
            bd->BDSTAT = ((bd->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
            return 0; /* Recycle OUT ZLPs without inventing a stream byte. */
        }
    }
    *byte = rx[position++];
    if (!--available) {
        bd->BDCNT = sizeof(rx);
        bd->BDSTAT = ((bd->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
    }
    return 1;
}

uint8_t cdc_tx_ready(void) {
    return usb_configured() && !usb_halted(5) && !(usb_bdt[5].BDSTAT & UOWN);
}

uint8_t cdc_send(const uint8_t *data, uint8_t length) {
    BDentry *bd = &usb_bdt[5];
    /* Protocol replies are 1 or 4 bytes: always short, no bulk ZLP needed. */
    if (!cdc_tx_ready() || length > 4) return 0;
    memcpy(tx, data, length);
    bd->BDCNT = length;
    bd->BDSTAT = ((bd->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
    return 1;
}
