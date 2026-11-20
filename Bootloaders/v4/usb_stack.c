/* Derived from Honken/JTR USB stack, CC BY 3.0; see COPYING-USB.txt.
 * Polled device-only stack: EP0 control, EP1 notification, EP2 CDC bulk.
 * Hardware contracts: DS39897C 18.2, 18.3.1 and registers 18-1..18-21.
 */
#include <string.h>
#include "usb_stack.h"
#include "descriptors.h"
#include "cdc.h"

BDentry usb_bdt[6] __attribute__((aligned(512)));
static uint8_t ep0_out[8], ep0_in[8], setup[8], halted[6];
static uint8_t configured, address_pending, phase, need_zlp;
static const uint8_t *reply_ptr;
static uint16_t reply_left, milliseconds, timer_last, timer_fraction;
enum { IDLE, DATA_IN, STATUS_OUT, LINE_OUT, STATUS_IN, STALLED };

uint16_t usb_word(const uint8_t *p) {
    return p[0] | ((uint16_t)p[1] << 8);
}

uint16_t usb_millis(void) { return milliseconds; }
uint8_t usb_configured(void) { return configured; }
uint8_t usb_halted(uint8_t bd) { return bd < 6 ? halted[bd] : 1; }

static void arm_out(uint8_t flags) {
    usb_bdt[0].BDCNT = sizeof(ep0_out);
    usb_bdt[0].BDSTAT = UOWN | flags;
}

static void stall(void) {
    phase = STALLED;
    address_pending = 0xFF;
    usb_bdt[1].BDSTAT = UOWN | BSTALL;
    arm_out(BSTALL);
}

static void in_packet(uint8_t length, uint8_t toggle) {
    usb_bdt[1].BDCNT = length;
    usb_bdt[1].BDSTAT = UOWN | DTSEN | toggle;
}

static void next_in(void) {
    uint8_t length = reply_left > sizeof(ep0_in) ? sizeof(ep0_in) : reply_left;
    if (length) memcpy(ep0_in, reply_ptr, length);
    if (length) reply_ptr += length;
    reply_left -= length;
    if (!length) need_zlp = 0;
    in_packet(length, (usb_bdt[1].BDSTAT ^ DTS) & DTS);
    if (!reply_left && !need_zlp) phase = STATUS_OUT;
}

void usb_reply(const uint8_t *data, uint16_t length, uint16_t requested) {
    if (!requested) {
        phase = STATUS_IN;
        in_packet(0, DTS);
        return;
    }
    need_zlp = length < requested && !(length % sizeof(ep0_in));
    reply_left = length < requested ? length : requested;
    reply_ptr = data;
    phase = DATA_IN;
    usb_bdt[1].BDSTAT = 0; /* The first control data packet is DATA1. */
    next_in();
}

void usb_expect_line_coding(void) { phase = LINE_OUT; }

static void unconfigure(void) {
    uint8_t i;
    configured = 0;
    USB_UEP1 = USB_UEP2 = 0;
    for (i = 2; i < 6; ++i) {
        usb_bdt[i].BDSTAT = 0;
        halted[i] = 0;
    }
    cdc_reset();
}

static void reset(void) {
    uint8_t i;
    /* Disable endpoints before reclaiming descriptors owned by the SIE. */
    for (i = 0; i < 16; ++i) USB_UEP[i] = 0;
    U1CONbits.PPBRST = 1;
    U1CONbits.PPBRST = 0;
    unconfigure();
    usb_bdt[0].BDSTAT = usb_bdt[1].BDSTAT = 0;
    usb_bdt[0].BDADDR = ep0_out;
    usb_bdt[1].BDADDR = ep0_in;
    U1ADDR = 0;
    address_pending = 0xFF;
    phase = IDLE;
    reply_left = need_zlp = 0;
    U1EP0 = USB_EP_CONTROL;
    arm_out(DTSEN);
    U1CONbits.PKTDIS = 0;
}

void usb_init(void) {
    IEC5bits.USB1IE = 0; /* Bootloader never enables peripheral interrupts. */
    U1CON = 0;
    U1OTGCON = 0;
    U1PWRC = 0;
    U1IE = U1EIE = U1OTGIE = 0;
    U1IR = U1EIR = U1OTGIR = 0xFF; /* Write-one-to-clear, never RMW. */
    U1CNFG1 = U1CNFG2 = 0;
    U1BDTP1 = (uintptr_t)usb_bdt >> 8;
    cdc_init();
    reset();
    timer_last = TMR1;
    timer_fraction = milliseconds = 0;
    U1OTGCON = 0x80;
    U1PWRCbits.USBPWR = 1;
    U1CONbits.USBEN = 1; /* No SE0 busy wait: reset/enumeration remain polled. */
}

static uint8_t endpoint_index(uint16_t address) {
    if (address == 0 || address == 0x80) return address ? 1 : 0;
    if (!configured) return 0xFF;
    if (address == 0x81) return 3;
    if (address == 2) return 4;
    if (address == 0x82) return 5;
    return 0xFF;
}

static void endpoint_halt(uint8_t bd, uint8_t halt) {
    uint8_t ep = bd / 2;
    unsigned int enable = bd & 1 ? 4u : 8u;
    /* Stop DMA for this direction before reclaiming a possibly armed BD. */
    USB_UEP[ep] &= ~enable;
    halted[bd] = halt;
    if (halt) usb_bdt[bd].BDSTAT = UOWN | BSTALL;
    else if (bd == 4) {
        usb_bdt[bd].BDCNT = 64;
        usb_bdt[bd].BDSTAT = UOWN | DTSEN; /* Next OUT must be DATA0. */
    } else usb_bdt[bd].BDSTAT = DTS; /* Next submitted IN toggles to DATA0. */
    USB_UEP[ep] |= enable;
    USB_UEP[ep] &= ~2u; /* Clear the hardware's observed EPSTALL latch. */
    cdc_reset(); /* A halted stream cannot preserve a partial command. */
}

static uint8_t standard_request(void) {
    uint8_t request = setup[1], type = setup[0], bd;
    uint16_t value = usb_word(setup + 2), index = usb_word(setup + 4);
    uint16_t length = usb_word(setup + 6), size;
    const uint8_t *descriptor;
    if (type == 0x80 && request == 6) {
        switch (value >> 8) {
        case 1:
            if ((value & 0xFF) || index) return 0;
            descriptor = cdc_device_descriptor;
            size = sizeof(cdc_device_descriptor);
            break;
        case 2:
            if ((value & 0xFF) || index) return 0;
            descriptor = cdc_config_descriptor;
            size = sizeof(cdc_config_descriptor);
            break;
        case 3:
            if ((value & 0xFF) >= USB_NUM_STRINGS ||
                ((value & 0xFF) ? index != 0x0409 : index != 0)) return 0;
            descriptor = cdc_str_descs;
            for (bd = 0; bd < (value & 0xFF); ++bd) descriptor += descriptor[0];
            size = descriptor[0];
            break;
        default: return 0;
        }
        usb_reply(descriptor, size, length);
        return 1;
    }
    if (type == 0 && request == 5 && value <= 127 && !index && !length && !configured) {
        address_pending = value;
        usb_reply(0, 0, 0);
        return 1;
    }
    if (type == 0 && request == 9 && value <= 1 && !index && !length && U1ADDR) {
        unconfigure();
        if (value) {
            configured = 1;
            cdc_configure();
        }
        usb_reply(0, 0, 0);
        return 1;
    }
    if (type == 0x80 && request == 8 && !value && !index && length == 1 && U1ADDR) {
        usb_reply(&configured, 1, length);
        return 1;
    }
    if ((type == 0x80 || type == 0x81 || type == 0x82) && request == 0 && !value && length == 2) {
        static uint8_t status[2];
        status[0] = status[1] = 0;
        if (type == 0x80 && index) return 0;
        if (type == 0x81 && (!configured || index >= 2)) return 0;
        if (type == 0x82) {
            bd = endpoint_index(index);
            if (bd == 0xFF) return 0;
            status[0] = halted[bd]; /* GET_STATUS must not change a BD. */
        }
        usb_reply(status, 2, length);
        return 1;
    }
    if (type == 0x02 && (request == 1 || request == 3) && !value && !length) {
        bd = endpoint_index(index);
        if (bd < 2 || bd == 0xFF) return 0;
        endpoint_halt(bd, request == 3);
        usb_reply(0, 0, 0);
        return 1;
    }
    if (configured && index < 2 && !value) {
        if (type == 0x81 && request == 10 && length == 1) {
            static const uint8_t alternate = 0;
            usb_reply(&alternate, 1, length);
            return 1;
        }
        if (type == 0x01 && request == 11 && !length) {
            /* Only alternate zero exists; reinitialize interface endpoints. */
            unconfigure();
            configured = 1;
            cdc_configure();
            usb_reply(0, 0, 0);
            return 1;
        }
    }
    return 0; /* No remote wakeup, vendor requests, or unsupported selectors. */
}

static void transaction(uint8_t status) {
    uint8_t bd = status >> 3, token;
    if (bd >= 6) return;
    token = usb_bdt[bd].BDSTAT & 0x3C;
    if (bd == 0 && token == 0x34) {
        /* SETUP aborts every old transfer, including deferred SET_ADDRESS. */
        usb_bdt[1].BDSTAT = 0;
        phase = IDLE;
        address_pending = 0xFF;
        reply_left = need_zlp = 0;
        U1EP0 &= ~2u;
        if (usb_bdt[0].BDCNT != 8) stall();
        else {
            memcpy(setup, ep0_out, 8);
            if (!standard_request() && !cdc_setup(setup)) stall();
            if (phase != STALLED) arm_out(DTS | DTSEN);
        }
        U1CONbits.PKTDIS = 0;
    } else if (bd == 1 && token == 0x24) {
        if (phase == DATA_IN) next_in();
        else if (phase == STATUS_IN) {
            if (address_pending != 0xFF) U1ADDR = address_pending;
            address_pending = 0xFF;
            phase = IDLE;
        }
    } else if (bd == 0 && token == 0x04) {
        if (phase == LINE_OUT && cdc_line_data(ep0_out, usb_bdt[0].BDCNT)) {
            usb_reply(0, 0, 0);
            arm_out(DTSEN);
        } else if (phase == STATUS_OUT && !usb_bdt[0].BDCNT) {
            usb_bdt[1].BDSTAT = 0; /* Cancel a prequeued data packet if host aborts. */
            phase = IDLE;
            arm_out(DTSEN);
        } else stall();
    }
    /* Bulk BDs are consumed by CDC, never discarded or rearmed here. */
}

void usb_handler(void) {
    uint16_t now = TMR1, elapsed = now - timer_last;
    timer_last = now;
    /* Timer1 at FCY/64 = 250 kHz. Poll more often than its 262 ms wrap. */
    milliseconds += elapsed / 250;
    timer_fraction += elapsed % 250;
    if (timer_fraction >= 250) { ++milliseconds; timer_fraction -= 250; }
    if (U1IRbits.URSTIF) {
        U1CONbits.PKTDIS = 1;
        U1EP0 = USB_UEP1 = USB_UEP2 = 0;
        /* The four-deep STAT FIFO takes six instruction cycles to advance. */
        while (U1IRbits.TRNIF) {
            U1IR = 8;
            __asm__ volatile("nop\n nop\n nop\n nop\n nop\n nop");
        }
        reset();
        U1IR = 1;
    }
    if (U1IRbits.UERRIF) U1EIR = 0xFF;
    if (U1IR & 0xB4) U1IR = U1IR & 0xB4; /* Excludes TRN and RESET. */
    if (U1IRbits.TRNIF) {
        transaction(U1STAT);
        U1IR = 8; /* Exactly one FIFO pop; the next poll observes its delay. */
    }
}
