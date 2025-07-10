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

#include "../dp_usb/usb_stack_globals.h"    // USB stack only defines Not function related.

#include <string.h>

enum stopbits {
    one = 0, oneandahalf = 1, two = 2
};

enum parity {
    none = 0, odd = 1, even = 2, mark = 3, space = 4
};
const char parity_str[] = {'N', 'O', 'E', 'M', 'S'};

struct cdc_LineCodeing {
    unsigned long int dwDTERate;
    enum stopbits bCharFormat;
    enum parity bParityType;
    BYTE bDataBits;
} linecodeing;


#pragma udata usb_data
BYTE cdc_acm_in_buffer[CDC_NOTICE_BUFFER_SIZE]; //JTR NEWLY defined NOTICE BUFFER SIZE and increased from 8 to 10 bytes in usb_config.h

#pragma udata usb_data3
BYTE cdc_In_bufferA[CDC_BUFFER_SIZE];
BYTE cdc_In_bufferB[CDC_BUFFER_SIZE];
BYTE cdc_Out_bufferA[CDC_BUFFER_SIZE];
BYTE cdc_Out_bufferB[CDC_BUFFER_SIZE];

#pragma udata

struct _cdc_ControlLineState cls;
BYTE cdc_In_len; // total cdc In length
volatile BYTE cdc_Out_len; // total cdc out length
BYTE IsInBufferA;
BYTE IsOutBufferA;
BYTE *InPtr;
BYTE *OutPtr;
BYTE LineStateUpdated = 0;
BYTE cdc_timeout_count = 0;
BYTE ZLPpending = 0;
BYTE lock = 0;

BDentry *CDC_Outbdp, *CDC_Inbdp;

volatile BYTE cdc_trf_state; // JTR don't see that it is really volatile in current context may be in future.

void initCDC(void) {

    // JTR The function usb_init() is now called from main.c prior to anything else belonging to the CDC CLASS
    // If we have multiple functions we want the USB initialization to be in only one consistant place.
    // The sort of things we would do in InitCDC would be to setup I/O pins and the HARDWARE UART so it
    // is not transmitting junk between a RESET and the device being enumerated. Hardware CTS/RTS
    // would also be setup here if being used.

    linecodeing.dwDTERate = 115200;
    linecodeing.bCharFormat = one;
    linecodeing.bParityType = none;
    linecodeing.bDataBits = 8;
    cls.DTR = 0;
    cls.RTS = 0;
    usb_register_class_setup_handler(cdc_setup);
}

void user_configured_init(void) {
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
    cdc_trf_state = 0;
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
                    usb_set_out_handler(0, cdc_set_line_coding_data); // Register out handler function
                    break;

                case CDC_GET_LINE_CODING: // Optional, strongly recomended
                    // JTR reply length (7) is always going to be less than minimum EP0 size (8)

                    reply_len = *((unsigned int *) &packet[USB_wLength]);
                    if (sizeof (struct cdc_LineCodeing) < reply_len) {
                        reply_len = sizeof (struct cdc_LineCodeing);
                    }
                    memcpy(EP0_Inbdp->BDADDR, (const void *) &linecodeing, reply_len);
                    usb_ack_dat1(reply_len); // JTR common addition for STD and CLASS ACK
                    usb_set_in_handler(0, cdc_get_line_coding);
                    break;

                case CDC_SET_CONTROL_LINE_STATE: // Optional
                    cls = *((struct _cdc_ControlLineState *) &packet[USB_wValue]);
                    usb_set_in_handler(0, cdc_set_control_line_state_status); // JTR why bother?
                    usb_ack_dat1(0); // JTR common addition for STD and CLASS ACK
                    LineStateUpdated = 1;
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
    unsigned long dwBaud, dwBaudrem;

    memcpy(&linecodeing, (const void *) EP0_Outbdp->BDADDR, sizeof (struct cdc_LineCodeing));

    dwBaud = BAUDCLOCK_FREQ / linecodeing.dwDTERate;
    dwBaudrem = BAUDCLOCK_FREQ % linecodeing.dwDTERate;
    if (linecodeing.dwDTERate > (dwBaudrem << 1))
        dwBaud--;

    UART_BAUD_setup(dwBaud);

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
    while (CDC_Outbdp->BDSTAT & UOWN) {};
}

inline void WaitInReady(void) {
    while (CDC_Inbdp->BDSTAT & UOWN) {};
}

/******************************************************************************/
inline BYTE getOutReady(void) {
    return !(CDC_Outbdp->BDSTAT & UOWN); // Do we have a packet from host?
}

/******************************************************************************/
inline BYTE getInReady(void) {

    return !(CDC_Inbdp->BDSTAT & UOWN); // Is the CDC In buffer ready?
}

/******************************************************************************/
BYTE getda_cdc(void) {
    WaitOutReady();

    if ((IsOutBufferA & 1)) {
        OutPtr = &cdc_Out_bufferA[0];
        CDC_Outbdp->BDADDR = &cdc_Out_bufferB[0];
    } else {
        OutPtr = &cdc_Out_bufferB[0];
        CDC_Outbdp->BDADDR = &cdc_Out_bufferA[0];
    }
    IsOutBufferA ^= 0xFF;
    cdc_Out_len = CDC_Outbdp->BDCNT;
    CDC_Outbdp->BDCNT = CDC_BUFFER_SIZE;
    CDC_Outbdp->BDSTAT = ((CDC_Outbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
#ifndef USB_INTERRUPTS
    usb_handler();
#endif
    return cdc_Out_len;
}//end getCDC_Out_ArmNext

static void putda_cdc(BYTE count) {

    WaitInReady();

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
    lock = 0;
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
        } else if (ZLPpending) {
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
    if (cdc_In_len == CDC_BUFFER_SIZE)
        ZLPpending = 1;

    lock = 0;
}

BYTE* getinbuf_cdc(uint16_t* bufsize)
{
    *bufsize = CDC_BUFFER_SIZE - cdc_In_len;
    return InPtr;
}

void write_cdc(uint16_t count)
{
//    lock = 1;
    if (count == CDC_BUFFER_SIZE || (cdc_In_len + count == CDC_BUFFER_SIZE)) {
        putda_cdc(CDC_BUFFER_SIZE); // This will stall tranfers if both buffers are full then return when a buffer is available.
        cdc_In_len = 0;
    }
    else
    {
        cdc_In_len += count;
        InPtr += count;
    }
//    cdc_timeout_count = 0; //setup timer to throw data if the buffer doesn't fill
//    lock = 0;
}

/******************************************************************************/
void putc_cdc(BYTE c) {
    lock = 1; // Stops CDCFlushOnTimeout() from sending per chance it is on interrupts.
    *InPtr++ = c;
    cdc_In_len++;
    if (cdc_In_len == CDC_BUFFER_SIZE) {
        putda_cdc(cdc_In_len); // This will stall tranfers if both buffers are full then return when a buffer is available.
        cdc_In_len = 0;
        ZLPpending = 1; // timeout handled in the SOF handler below.
    }
    lock = 0;
    cdc_timeout_count = 0; //setup timer to throw data if the buffer doesn't fill
}

/******************************************************************************/
// Waits for a byte to be available and returns that byte as a
// function return value. The byte is removed from the CDC OUT queue.
// No return count is required as this function always returns one byte.

BYTE getc_cdc(void) { // Must be used only in double buffer mode.
    BYTE c = 0;

    if (cdc_Out_len == 0) {
        do {
            cdc_Out_len = getda_cdc();
        } while (cdc_Out_len == 0); // Skip any ZLP
    }
    c = *OutPtr;
    OutPtr++;
    cdc_Out_len--;
    return c;
}

BYTE* getoutbuf_cdc(uint16_t* bufsize)
{
    while (cdc_Out_len == 0)    // Skip any ZLP
        cdc_Out_len = getda_cdc();

    BYTE* ret = OutPtr;

    if (*bufsize > cdc_Out_len)
        *bufsize = cdc_Out_len;

    cdc_Out_len -= *bufsize;
    OutPtr += *bufsize;

    return ret;
}

uint16_t read_cdc(BYTE* data, uint16_t len)    // Must be used only in double buffer mode.
{
    if(len)
    {
        while (cdc_Out_len == 0)    // Skip any ZLP
            cdc_Out_len = getda_cdc();

        if (len > cdc_Out_len)
            len = cdc_Out_len;

        cdc_Out_len -= len;

        memcpy(data, OutPtr, len);
        OutPtr += len;
    }

    return len;
}

/******************************************************************************/
// Checks to see if there is a byte available in the CDC buffer.
// If so, it returns that byte at the dereferenced pointer *C
// and the function returns a count of 1. The byte is effectively
// removed from the queue.
// IF no byte is available function returns immediately with a count of zero.

BYTE poll_getc_cdc(BYTE * c) { // Must be used only in double buffer mode.

    if (cdc_Out_len) { // Do we have a byte waiting?
        *c = *OutPtr; // pass it on and adjust OutPtr and count
        OutPtr++;
        cdc_Out_len--;
        return 1; // Return byte count, always one.
    }
    if (getOutReady()) { // No byte in queue check for new arrivals.
        cdc_Out_len = getda_cdc();
        if (cdc_Out_len) {
            *c = *OutPtr;
            OutPtr++;
            cdc_Out_len--;
            return 1;
        }
    }
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

    if (cdc_Out_len) {
        *c = *OutPtr;
        return 1;
    }
    if (getOutReady()) {
        cdc_Out_len = getda_cdc();
        if (cdc_Out_len) {
            *c = *OutPtr;
            return 1;
        }
    }
    return 0;
}

