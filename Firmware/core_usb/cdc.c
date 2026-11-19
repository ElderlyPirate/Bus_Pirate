/*
This work is licensed under the Creative Commons Attribution 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/
or send a letter to
        Creative Commons,
        171 Second Street,
        Suite 300,
        San Francisco,
        California,
        94105,
        USA.
 */

// JTR V0.1a
// JTR v0.1b
// JTR v0.1c	// tidy up added code line code handler to set the SPBRG based on line coding.
// JTR V0.2a   // 26th Jan 2012

#include "usb_stack_globals.h"    // USB stack only defines Not function related.

#include <string.h>
#include "../configuration.h"

#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT
#include "../programmer_avrisp/avrisp_mkii.h"
#endif

/* CDC PSTN 1.2 Table 17: seven wire bytes, not a native enum/struct ABI. */
static BYTE line_coding[7];


#pragma udata usb_data
BYTE cdc_acm_in_buffer[CDC_NOTICE_BUFFER_SIZE]; //JTR NEWLY defined NOTICE BUFFER SIZE and increased from 8 to 10 bytes in usb_config.h

#pragma udata usb_data3
BYTE cdc_In_bufferA[CDC_BUFFER_SIZE];
BYTE cdc_In_bufferB[CDC_BUFFER_SIZE];
BYTE cdc_Out_bufferA[CDC_BUFFER_SIZE];
BYTE cdc_Out_bufferB[CDC_BUFFER_SIZE];

#pragma udata

/*
 * The variables below are shared between the USB start-of-frame interrupt
 * handler (CDCFlushOnTimeout(), called through sof_handler) and the
 * foreground code, so they must be volatile.  Marking them volatile is what
 * makes the compiler re-read them on every access instead of caching them in
 * a register, which is the same class of bug that was fixed for BDSTAT in
 * issue #11.
 */
volatile BYTE cdc_In_len; // total cdc In length
volatile BYTE cdc_Out_len; // total cdc out length
volatile BYTE IsInBufferA;
volatile BYTE IsOutBufferA;
BYTE * volatile InPtr;
BYTE * volatile OutPtr;
volatile BYTE cdc_timeout_count = 0;
volatile BYTE ZLPpending = 0;
volatile BYTE lock = 0;

BDentry *CDC_Outbdp, *CDC_Inbdp;

void initCDC(void) {

    // JTR The function usb_init() is now called from main.c prior to anything else belonging to the CDC CLASS
    // If we have multiple functions we want the USB initialization to be in only one consistant place.
    // The sort of things we would do in InitCDC would be to setup I/O pins and the HARDWARE UART so it
    // is not transmitting junk between a RESET and the device being enumerated. Hardware CTS/RTS
    // would also be setup here if being used.

    line_coding[0] = 0x00; /* 115200, little endian */
    line_coding[1] = 0xc2;
    line_coding[2] = 0x01;
    line_coding[3] = 0x00;
    line_coding[4] = 0; /* one stop bit */
    line_coding[5] = 0; /* no parity */
    line_coding[6] = 8;
    usb_register_class_setup_handler(cdc_setup);
}

void user_configured_init(void) {
#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT
    if (avrisp_usb_personality_active) {
        avrisp_usb_configure_endpoints();
        return;
    }
#endif
    // JTR NEW FUNCTION
    // After the device is enumerated and configured then we set up non EP0 endpoints.
    // We only enable the endpoints we are using, not all of them.
    // Prior to this they are held in a disarmed state.

    // This function belongs to the current USB function and IS NOT generic. This is CLASS specific
    // and will vary from implementation to implementation.

    usb_unset_in_handler(1);
    usb_unset_in_handler(2);
    usb_unset_out_handler(2);

    USB_UEP1 = USB_EP_IN;
    USB_UEP2 = USB_EP_INOUT;

    /* Configure buffer descriptors */
#if USB_PP_BUF_MODE == 0
    // JTR Setup CDC LINE_NOTICE EP (Interrupt IN)
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDADDR = cdc_acm_in_buffer;
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTS + DTSEN; // Set DTS => First packet inverts, ie. is Data0
#else
    // TODO: Implement Ping-Pong buffering setup.
#error "PP Mode not implemented yet"
#endif

    usb_register_class_setup_handler(cdc_setup);
    usb_register_sof_handler(CDCFlushOnTimeout);
    cdc_timeout_count = 0;
    ZLPpending = 0;
    CDC_Outbdp = &usb_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)];
    CDC_Inbdp = &usb_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)];

    IsInBufferA = 0xFF;
    InPtr = cdc_In_bufferA;
    cdc_In_len = 0;
    CDC_Inbdp->BDADDR = &cdc_In_bufferA[0];
    CDC_Inbdp->BDCNT = 0;
    CDC_Inbdp->BDSTAT = DTS + DTSEN;

    cdc_Out_len = 0;
    IsOutBufferA = 0xFF;
    OutPtr = cdc_Out_bufferA;
    CDC_Outbdp->BDCNT = CDC_BUFFER_SIZE;
    CDC_Outbdp->BDADDR = &cdc_Out_bufferA[0];
    CDC_Outbdp->BDSTAT = UOWN + DTSEN;
}

void cdc_setup(void) {
    BYTE *packet;
    size_t reply_len;
    packet = EP0_Outbdp->BDADDR;

    if (packet[USB_wIndex] != 0 || packet[USB_wIndexHigh] != 0) {
        usb_RequestError(); /* ACM requests belong to the control interface. */
        return;
    }

    switch (packet[USB_bmRequestType] & (USB_bmRequestType_TypeMask | USB_bmRequestType_RecipientMask)) {
        case (USB_bmRequestType_Class | USB_bmRequestType_Interface):
            switch (packet[USB_bRequest]) {

                    //JTR This is just a dummy, nothing defined to do for CDC ACM
                case CDC_SEND_ENCAPSULATED_COMMAND:
                    usb_ack_dat1(0);
                    break;

                    //JTR This is just a dummy, nothing defined to do for CDC ACM
                case CDC_GET_ENCAPSULATED_RESPONSE:
                    //usb_ack_zero(rbdp);
                    usb_ack_dat1(0);
                    break;

                case CDC_SET_COMM_FEATURE: // Optional
                case CDC_GET_COMM_FEATURE: // Optional
                case CDC_CLEAR_COMM_FEATURE: // Optional
                    usb_RequestError(); // Not advertised in ACM functional descriptor
                    break;

                case CDC_SET_LINE_CODING: // Optional, strongly recomended
                    if (packet[USB_bmRequestType] != 0x21 ||
                            packet[USB_wValue] || packet[USB_wValueHigh] ||
                            packet[USB_wLength] != sizeof(line_coding) || packet[USB_wLengthHigh]) {
                        usb_RequestError();
                        break;
                    }
                    usb_set_out_handler(0, cdc_set_line_coding_data); // Register out handler function
                    break;

                case CDC_GET_LINE_CODING: // Optional, strongly recomended
                    // JTR reply length (7) is always going to be less than minimum EP0 size (8)

                    if (packet[USB_bmRequestType] != 0xa1 ||
                            packet[USB_wValue] || packet[USB_wValueHigh]) {
                        usb_RequestError();
                        break;
                    }
                    reply_len = packet[USB_wLength] | ((unsigned int)packet[USB_wLengthHigh] << 8);
                    if (sizeof(line_coding) < reply_len)
                        reply_len = sizeof(line_coding);
                    memcpy(EP0_Inbdp->BDADDR, line_coding, reply_len);
                    usb_ack_dat1(reply_len); // JTR common addition for STD and CLASS ACK
                    usb_set_in_handler(0, cdc_get_line_coding);
                    break;

                case CDC_SET_CONTROL_LINE_STATE: // Optional
                    if (packet[USB_bmRequestType] != 0x21 ||
                            (packet[USB_wValue] & ~3u) || packet[USB_wValueHigh] ||
                            packet[USB_wLength] || packet[USB_wLengthHigh]) {
                        usb_RequestError();
                        break;
                    }
                    usb_set_in_handler(0, cdc_set_control_line_state_status); // JTR why bother?
                    usb_ack_dat1(0); // JTR common addition for STD and CLASS ACK
                    break;

                case CDC_SEND_BREAK: // Optional
                default:
                    usb_RequestError();
            }
            break;
        default:
            usb_RequestError();
    }
}

void cdc_get_line_coding(void) {
    usb_unset_in_handler(0); // Unregister IN handler;
}

void cdc_set_line_coding_data(void) { // JTR handling an OUT token In the CDC stack this is the only function that handles an OUT data stage.
    BYTE *data = EP0_Outbdp->BDADDR;
    if (EP0_Outbdp->BDCNT != sizeof(line_coding) || data[4] > 2 || data[5] > 4 ||
            !(data[6] == 5 || data[6] == 6 || data[6] == 7 || data[6] == 8 || data[6] == 16)) {
        usb_RequestError();
        return;
    }
    /* Virtual COM metadata must not divide by a host value or touch UART1. */
    memcpy(line_coding, data, sizeof(line_coding));

    usb_unset_out_handler(0); // Unregister OUT handler; JTR serious bug fix in macro!
    usb_set_in_handler(0, cdc_set_line_coding_status); // JTR why bother?
    usb_ack_dat1(0); // JTR common addition for STD and CLASS ACK

    // JTR This part of the USB-CDC stack is worth highlighting
    // This is the only place that we have an OUT DATA packet on
    // EP0. At this point it has been completed. This stack unlike
    // the microchip stack does not have a common IN or OUT data
    // packet complete tail and therefore it is the responsibility
    // of each section to ensure that EP0 is set-up correctly for
    // the next setup packet.


    //  Force EP0 OUT to the DAT0 state
    //  after we have all our data packets.
    EP0_Outbdp->BDCNT = USB_EP0_BUFFER_SIZE;
    EP0_Outbdp->BDSTAT = UOWN | DTSEN;
}

inline void cdc_set_line_coding_status(void) {
    usb_unset_in_handler(0);
}

inline void cdc_set_control_line_state_status(void) {
    usb_unset_in_handler(0);
}

inline void WaitOutReady(void) {
    while (usb_device_state == CONFIGURED_STATE && (CDC_Outbdp->BDSTAT & UOWN)) {};
}

inline void WaitInReady(void) {
    while (usb_device_state == CONFIGURED_STATE && (CDC_Inbdp->BDSTAT & UOWN)) {};
}

/******************************************************************************/
inline BYTE getOutReady(void) {
    BYTE irq = TestGlobalUsbInterruptEnable();
    BYTE ready = 0;
    DisableGlobalUsbInterrupt();
    if (usb_device_state == CONFIGURED_STATE && !(CDC_Outbdp->BDSTAT & UOWN)) {
        if (CDC_Outbdp->BDCNT && CDC_Outbdp->BDCNT <= CDC_BUFFER_SIZE)
            ready = 1;
        /* A ZLP is not an application byte, but must be recycled to avoid NAKing
         * every subsequent OUT. Treat an invalid DMA count the same way. */
        else getda_cdc();
    }
    if (irq) EnableUsbGlobalInterrupt();
    return ready;
}

/******************************************************************************/
inline BYTE getInReady(void) {

    return usb_device_state == CONFIGURED_STATE && !(CDC_Inbdp->BDSTAT & UOWN);
}

/******************************************************************************/
BYTE getda_cdc(void) {
    BYTE irq;
    WaitOutReady();
    irq = TestGlobalUsbInterruptEnable();
    DisableGlobalUsbInterrupt();
    if (usb_device_state != CONFIGURED_STATE || (CDC_Outbdp->BDSTAT & UOWN)) {
        if (irq) EnableUsbGlobalInterrupt();
        return 0;
    }

    if ((IsOutBufferA & 1)) {
        OutPtr = &cdc_Out_bufferA[0];
        CDC_Outbdp->BDADDR = &cdc_Out_bufferB[0];
    } else {
        OutPtr = &cdc_Out_bufferB[0];
        CDC_Outbdp->BDADDR = &cdc_Out_bufferA[0];
    }
    IsOutBufferA ^= 0xFF;
    cdc_Out_len = CDC_Outbdp->BDCNT;
    if (cdc_Out_len > CDC_BUFFER_SIZE)
        cdc_Out_len = 0;
    CDC_Outbdp->BDCNT = CDC_BUFFER_SIZE;
    CDC_Outbdp->BDSTAT = ((CDC_Outbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
    if (irq) EnableUsbGlobalInterrupt();
#ifndef USB_INTERRUPTS
    usb_handler();
#endif
    return cdc_Out_len;
}//end getCDC_Out_ArmNext

static void putda_cdc(BYTE count) {
    BYTE irq;
    WaitInReady();
    irq = TestGlobalUsbInterruptEnable();
    DisableGlobalUsbInterrupt();
    if (usb_device_state != CONFIGURED_STATE) {
        if (irq) EnableUsbGlobalInterrupt();
        return;
    }

    if (IsInBufferA) {
        CDC_Inbdp->BDADDR = cdc_In_bufferA;
        InPtr = cdc_In_bufferB;
    } else {
        CDC_Inbdp->BDADDR = cdc_In_bufferB;
        InPtr = cdc_In_bufferA;
    }

    CDC_Inbdp->BDCNT = count;
    CDC_Inbdp->BDSTAT = ((CDC_Inbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
    IsInBufferA ^= 0xFF;
    if (irq) EnableUsbGlobalInterrupt();
#ifndef USB_INTERRUPTS
    usb_handler();
#endif
}

void SendZLP(void) {
    WaitInReady();

    CDC_Inbdp->BDCNT = 0;
    CDC_Inbdp->BDSTAT = ((CDC_Inbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;

#ifndef USB_INTERRUPTS
    usb_handler();
#endif
    ZLPpending = 0;
    cdc_timeout_count = 0;
}

/******************************************************************************/
void CDC_Flush_In_Now(void) {
    BYTE was_locked = lock;
    lock = 1;
    if (cdc_In_len > 0) {
        putda_cdc(cdc_In_len);
        if (cdc_In_len == CDC_BUFFER_SIZE) {
            ZLPpending = 1;
        } else {
            ZLPpending = 0;
        }
        cdc_In_len = 0;
        cdc_timeout_count = 0;
    }
    lock = was_locked;
}

/******************************************************************************/
void CDCFlushOnTimeout(void) {

    if (cdc_timeout_count >= CDC_FLUSH_MS) { // For timeout value see: cdc_config.h -> [hardware] -> CDC_FLUSH_MS

        if (cdc_In_len > 0) {
            if ((lock == 0) && getInReady()) {
                putda_cdc(cdc_In_len);
                if (cdc_In_len == CDC_BUFFER_SIZE) {
                    ZLPpending = 1;
                } else {
                    ZLPpending = 0;
                }
                cdc_In_len = 0;
                cdc_timeout_count = 0;
            }
        } else if (ZLPpending && (lock == 0) && getInReady()) {
            /* Never race a locked writer or wait for IN completion in the ISR. */
            SendZLP();
        }
    } else {
        cdc_timeout_count++;
    }
}

void setLock(void)
{
    lock = 1;
}

void freeLock(void)
{
    CDC_Flush_In_Now();
    lock = 0;
}

BYTE* getinbuf_cdc(uint16_t* bufsize)
{
    BYTE irq = TestGlobalUsbInterruptEnable();
    BYTE *ret;
    DisableGlobalUsbInterrupt();
    *bufsize = CDC_BUFFER_SIZE - cdc_In_len;
    ret = InPtr;
    if (irq) EnableUsbGlobalInterrupt();
    return ret;
}

void write_cdc(uint16_t count)
{
    BYTE irq = TestGlobalUsbInterruptEnable();
    DisableGlobalUsbInterrupt();
    if (count > CDC_BUFFER_SIZE - cdc_In_len) {
        if (irq) EnableUsbGlobalInterrupt();
        return; /* The caller may commit only the span returned by getinbuf_cdc. */
    }
    if (cdc_In_len + count == CDC_BUFFER_SIZE) {
        if (irq) EnableUsbGlobalInterrupt();
        putda_cdc(CDC_BUFFER_SIZE); // This will stall tranfers if both buffers are full then return when a buffer is available.
        cdc_In_len = 0;
        /* A full final packet needs a short-packet terminator on timeout. */
        ZLPpending = 1;
    }
    else
    {
        cdc_In_len += count;
        InPtr += count;
        if (irq) EnableUsbGlobalInterrupt();
    }
    cdc_timeout_count = 0; //setup timer to throw data if the buffer doesn't fill
}

/******************************************************************************/
void putc_cdc(BYTE c) {
    BYTE irq = TestGlobalUsbInterruptEnable();
    BYTE count;
    BYTE was_locked = lock;
    DisableGlobalUsbInterrupt();
    lock = 1; // Stops CDCFlushOnTimeout() from sending per chance it is on interrupts.
    *InPtr++ = c;
    cdc_In_len++;
    count = cdc_In_len;
    if (irq) EnableUsbGlobalInterrupt();
    if (count == CDC_BUFFER_SIZE) {
        putda_cdc(count); // Wait only outside the critical section.
        cdc_In_len = 0;
        ZLPpending = 1; // timeout handled in the SOF handler below.
    }
    cdc_timeout_count = 0; //setup timer to throw data if the buffer doesn't fill
    lock = was_locked;
}

/******************************************************************************/
// Waits for a byte to be available and returns that byte as a
// function return value. The byte is removed from the CDC OUT queue.
// No return count is required as this function always returns one byte.

BYTE getc_cdc(void) { // Must be used only in double buffer mode.
    BYTE irq, c;
    for (;;) {
        while (cdc_Out_len == 0)
            getda_cdc();
        irq = TestGlobalUsbInterruptEnable();
        DisableGlobalUsbInterrupt();
        if (cdc_Out_len) {
            c = *OutPtr++;
            cdc_Out_len--;
            if (irq) EnableUsbGlobalInterrupt();
            return c;
        }
        if (irq) EnableUsbGlobalInterrupt();
    }
}

BYTE* getoutbuf_cdc(uint16_t* bufsize)
{
    BYTE irq;
    BYTE *ret;
    if (*bufsize == 0)
        return OutPtr;
    for (;;) {
        while (cdc_Out_len == 0) /* Skip ZLPs; all waits keep interrupts enabled. */
            getda_cdc();
        irq = TestGlobalUsbInterruptEnable();
        DisableGlobalUsbInterrupt();
        if (cdc_Out_len)
            break;
        if (irq) EnableUsbGlobalInterrupt();
    }
    ret = OutPtr;
    if (*bufsize > cdc_Out_len)
        *bufsize = cdc_Out_len;

    cdc_Out_len -= *bufsize;
    OutPtr += *bufsize;
    if (irq) EnableUsbGlobalInterrupt();
    return ret;
}

uint16_t read_cdc(BYTE* data, uint16_t len)    // Must be used only in double buffer mode.
{
    BYTE *data_in = getoutbuf_cdc(&len);
    memcpy(data, data_in, len);
    return len;
}

/******************************************************************************/
// Checks to see if there is a byte available in the CDC buffer.
// If so, it returns that byte at the dereferenced pointer *C
// and the function returns a count of 1. The byte is effectively
// removed from the queue.
// IF no byte is available function returns immediately with a count of zero.

BYTE poll_getc_cdc(BYTE * c) { // Must be used only in double buffer mode.
    BYTE irq = TestGlobalUsbInterruptEnable();
    DisableGlobalUsbInterrupt();
    if (!cdc_Out_len && getOutReady())
        getda_cdc();
    if (cdc_Out_len) { // Do we have a byte waiting?
        *c = *OutPtr; // pass it on and adjust OutPtr and count
        OutPtr++;
        cdc_Out_len--;
        if (irq) EnableUsbGlobalInterrupt();
        return 1; // Return byte count, always one.
    }
    if (irq) EnableUsbGlobalInterrupt();
    return 0;
}

/******************************************************************************/
// Checks (PEEKS) to see if there is a byte available in the CDC buffer.
// If so, it returns that byte at the dereferenced pointer *C
// and the function returns a count of 1. The byte however is NOT
// removed from the queue and can still be read with the poll_getc_cdc()
// and getc_cdc() functions that will remove it from the queue.
// IF no byte is available function returns immediately with a count of zero.

BYTE peek_getc_cdc(BYTE * c) { // Must be used only in double buffer mode.
    BYTE irq = TestGlobalUsbInterruptEnable();
    DisableGlobalUsbInterrupt();
    if (!cdc_Out_len && getOutReady())
        getda_cdc();
    if (cdc_Out_len) {
        *c = *OutPtr;
        if (irq) EnableUsbGlobalInterrupt();
        return 1;
    }
    if (irq) EnableUsbGlobalInterrupt();
    return 0;
}

