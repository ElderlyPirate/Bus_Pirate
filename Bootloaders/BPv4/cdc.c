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

// JTR added


#include "globals.h"
#include <string.h>
BYTE FAST_usb_handler(void); // JTR2 added fast USB service. Pops non EP0 transfer flags from FIFO. Sevices RESET and SETUP

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
    unsigned char bDataBits;
} linecodeing;

struct _cdc_ControlLineState {
    int DTR : 1;
    int RTS : 1;
    int unused1 : 6;
    BYTE unused2;
};
struct _cdc_ControlLineState cls;
#pragma udata
BYTE cdc_In_len; // total cdc In length
BYTE cdc_Out_len; // total cdc out length
BYTE IsInBufferA;
BYTE IsOutBufferA;
BYTE *InPtr;
BYTE *OutPtr;
BYTE CDC_In_count;
BYTE LineStateUpdated = 0;
BYTE CDCFunctionError = 0;
unsigned char *data, *data_end;
BDentry *Outbdp, *Inbdp;
//volatile unsigned char CDC_trf_state;	// JTR don't see that it is really volatile in current context may be in future.

#pragma udata usb_data
//unsigned char cdc _acm _out _buffer[CDC_BUFFER_SIZE];	//JTR removed and trashed. not required with CDC - ACM.
unsigned char cdc_Out_buffer[CDC_BUFFER_SIZE];
unsigned char cdc_In_buffer[CDC_BUFFER_SIZE];
/*
BYTE cdc_In_bufferA[64];
BYTE cdc_In_bufferB[64];
BYTE cdc_Out_bufferA[64];
BYTE cdc_Out_bufferB[64];
 */

unsigned char cdc_acm_in_buffer[CDC_NOTICE_BUFFER_SIZE]; //JTR NEWLY defined NOTICE BUFFER SIZE and increased from 8 to 10 bytes in usb_config.h

#pragma udata

static struct _bootloaderusbbuffer {
    BYTE inBuf[CDC_BUFFER_SIZE];
    unsigned char cnt;
    unsigned char rdptr;
} usbbuf;

unsigned int SOFCOUNT;

void initCDC(void) {
    linecodeing.dwDTERate = 115200;
    linecodeing.bCharFormat = one;
    linecodeing.bParityType = none;
    linecodeing.bDataBits = 8;
    cls.DTR = 0;
    cls.RTS = 0;
    usb_register_class_setup_handler(cdc_setup);
    data = data_end = 0;
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
    usb_unset_out_handler(2); // JTR Macro has bug fix

    // JTR remove all USB_UEP[x] indexing from stack due to pointer bug (fixed?).
    // JTR experiment is the UEPx pointer system working? Lets try!
    // PASSED on PIC24

    //USB_UEP[1] = USB_EP_IN;
    //USB_UEP[2] = USB_EP_INOUT;

    USB_UEP1 = USB_EP_IN;
    USB_UEP2 = USB_EP_INOUT;

    /* Configure buffer descriptors */
#if USB_PP_BUF_MODE == 0
    // JTR Setup CDC LINE_NOTICE EP (Interrupt IN)
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDADDR = cdc_acm_in_buffer;
    usb_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTS + DTSEN; // Set DTS => First packet inverts, ie. is Data0

    usb_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)].BDCNT = CDC_BUFFER_SIZE; // JTR N/A endpoints[i].buffer_size;
    usb_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)].BDADDR = cdc_Out_buffer;
    usb_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)].BDSTAT = UOWN + DTSEN;

    usb_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
    usb_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)].BDADDR = cdc_In_buffer;
    usb_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTS + DTSEN; // Set DTS => First packet inverts, ie. is Data0

#else
    // TODO: Implement Ping-Pong buffering setup.
#error "PP Mode not implemented yet"
#endif
    usb_register_class_setup_handler(cdc_setup);
    Outbdp = &usb_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)];
    Inbdp = &usb_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)];
}

void cdc_setup(void) {
    vLedToggle();
    unsigned char *packet;
    size_t reply_len;
    packet = bdp->BDADDR;
    switch (packet[USB_bmRequestType] & (USB_bmRequestType_TypeMask | USB_bmRequestType_RecipientMask)) {
        case (USB_bmRequestType_Class | USB_bmRequestType_Interface):
            switch (packet[USB_bRequest]) {

                    //JTR This is just a dummy, nothing defined to do for CDC ACM
                case CDC_SEND_ENCAPSULATED_COMMAND: //
                    usb_ack_dat1(rbdp, 0);
                    break;

                    //JTR This is just a dummy, nothing defined to do for CDC ACM
                case CDC_GET_ENCAPSULATED_RESPONSE:
                    usb_ack_dat1(rbdp, 0);
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
                    reply_len = *((unsigned int *) &packet[USB_wLength]);
                    if (sizeof (struct cdc_LineCodeing) < reply_len) {
                        reply_len = sizeof (struct cdc_LineCodeing);
                    }
                    memcpy(rbdp->BDADDR, (const void *) &linecodeing, reply_len);
                    usb_ack_dat1(rbdp, reply_len); // JTR common addition for STD and CLASS ACK
                    //usb_set_in_handler(0, cdc_get_line_coding); // JTR why bother? There is nothing more to do.
                    break;

                case CDC_SET_CONTROL_LINE_STATE: // Optional
                    cls = *((struct _cdc_ControlLineState *) &packet[USB_wValue]);
                    usb_set_in_handler(0, cdc_set_control_line_state_status); // JTR why bother?
                    usb_ack_dat1(rbdp, 0); // JTR common addition for STD and CLASS ACK
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
    memcpy(&linecodeing, (const void *) bdp->BDADDR, sizeof (struct cdc_LineCodeing));
    usb_unset_out_handler(0); // Unregister OUT handler; JTR serious bug fix in macro!
    usb_set_in_handler(0, cdc_set_line_coding_status); // JTR why bother?
    usb_ack_dat1(rbdp, 0); // JTR common addition for STD and CLASS ACK

    // JTR This part of the USB-CDC stack is worth highlighting
    // This is the only place that we have an OUT DATA packet on
    // EP0. At this point it has been completed. This stack unlike
    // the microchip stack does not have a common IN or OUT data
    // packet complete tail and therefore it is the responsibility
    // of each section to ensure that EP0 is set-up correctly for
    // the next setup packet.

    //  This next line inverts the DTS so that it is now ready for
    //  a DAT0 packet. However it only works because we had one data
    //  packet. For any amount of EVEN data packets it would not be
    //  correct.
    //	usb_ack_out(bdp);  // JTR N/A Good for only odd number of data packets.

    //  The correct thing to do is to force EP0 OUT to the DAT0 state
    //  after we have all our data packets.
    bdp->BDCNT = USB_EP0_BUFFER_SIZE;
    bdp->BDSTAT = UOWN | DTSEN;
}

void cdc_set_line_coding_status(void) {
    usb_unset_in_handler(0);
}

void cdc_set_control_line_state_status(void) {
    usb_unset_in_handler(0);
}

// JTR N/A. There is no LINE_NOTIFICATION_OUT in CDC-ACM
//void cdc _acm _out( void ) {
//}

// JTR keep this fragment as it may be useful later if reworked a little
//void cdc_acm_in( void ) {

// JTR LINE_NOTIFICATION EP has been increased to ten bytes.
// This is because if it were ever to be used it is more likely
// that it will be for the Serial State Notification which has
// two bytes of data to include in the packet.

// Also we will not actually come to this code if the LINE_NOTIFICATION
// endpoint is not already armed and we do not arm this end point
// until we have something to send. Therefore we have a chicken and egg
// deadlock and this function is of no value the way it is currently coded.
// There is no IN token if the endpoint is not armed!
// No IN handler is required nor a state machine as there is no need for a ZLP

/*
if (0) { // Response Available Notification
        // Not TODO: Probably never implement this, we're not a modem.
        // Is this correct placement of the response notification?
        bdp->BDADDR[USB_bmRequestType]	= USB_bmRequestType_D2H | USB_bmRequestType_Class | USB_bmRequestType_Interface;
        bdp->BDADDR[USB_bRequest]		= CDC_RESPONSE_AVAILABLE;
        bdp->BDADDR[USB_wValue]			= 0;
        bdp->BDADDR[USB_wValueHigh]		= 0;
        bdp->BDADDR[USB_wIndex]			= 0;
        bdp->BDADDR[USB_wIndexHigh]		= 0;
        bdp->BDADDR[USB_wLength]		= 0;
        bdp->BDADDR[USB_wLengthHigh]	= 0;
        // JTR past below bdp->BDCNT = 8;
        usb_ack_dat1(bdp, 8);
} else if (0) {	// Network Connection Notification
} else if (0) {	// Serial State Notification
}
} */

/* END OF CDC CLASS REQUESTS HANDLING

 Below are the CDC USBUART functions */

void sof_counter_handler() {
    if (SOFCOUNT == 0) return;
    SOFCOUNT--;
}

BYTE WaitOutReady() // JTR2 added reduced overhead
{
    while ((Outbdp->BDSTAT & UOWN)) {
        if (!TestUsbInterruptEnabled()) {
            if (0 != FAST_usb_handler()) // JTR2 Pop non-EP0 (USB IN) tranfers from the FIFO and also give SETUP PACKETs a chance.
                return 0;
        }
    }
    return 1;
}//end WaitOutReady

/**********************************************************************************/

/**********************************************************************************
  Function:
        BYTE WaitInReady()

  // JTR2 addition
  Summary:
    As this CDC stack writes directly into USB RAM this function checks for
    for its availability.
    It is a semi-blocking function. It waits for the cdc In Buffer to
    become available before returning. However if the USB interrupt is
    not enabled it call the fast USB housekeeper to ensure that the USB
    stack is always serviced in the meantime. If in polling mode and a
    setup packet or bus reset happens it returns FALSE (00) to alert the calling
    function that the transfer may have been interfered with.

 *****************************************************************************/

/*****************************************************************************/
BYTE WaitInReady() // JTR2 added reduced overhead
{
    while ((Inbdp->BDSTAT & UOWN)) {
        if (!TestUsbInterruptEnabled()) {
            if (0 != FAST_usb_handler()) // JTR2 Pop non-EP0 (USB IN) tranfers from the FIFO and also give SETUP PACKETs a chance.
                return 0;
        }
    }
    return 1;
}//end WaitInReady

/**********************************************************************************
  Function:
        BYTE putPARTARRAYUSBUSART()

  // JTR2 addition
  Summary:
    Fast, low overhead CDC IN packet does not track or send ZLP.
    It is a semi-blocking function. It waits for the cdc In Buffer to
    become available before returning. However if the USB interrupt is
    not enabled it call the fast USB housekeeper to ensure that the USB
    stack is always serviced in the meantime. If in polling mode and a
    setup packet or bus reset happens it returns 0xFF to alert the calling
    function that the transfer may have been interfered with.

    Note that the data must reside in USB RAM.

 *****************************************************************************/
/*****************************************************************************/
// JTR2 Does NOT send ZLP after each packet only one at the end is required.

BYTE putPARTARRAYUSBUSART(BYTE *data, BYTE length) // JTR2 added reduced overhead
{
    while ((Inbdp->BDSTAT & UOWN)) {
        if (!TestUsbInterruptEnabled()) {

            if (0 != FAST_usb_handler()) { // JTR2 Pop non-EP0 (USB IN) tranfers from the FIFO and also give SETUP PACKETs a chance.
                return 0xff;
            }
        }
    }
    Inbdp->BDADDR = data;
    Inbdp->BDCNT = length;
    Inbdp->BDSTAT = ((Inbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
    FAST_usb_handler();
    return length;
}//end putUSBUSARTFAST

/**********************************************************************************
  Function:
        BYTE putFULLARRAYUSBUSART()

  // JTR2 addition
  Summary:
    Fast, low overhead CDC In packet does not track or send ZLP.
    It is a non-blocking function. It does not wait for the cdc In Buffer to
    become available. Instead it returns '0' to notify the caller that
    data has not been sent. Otherwise it return the endpoint size.

 *****************************************************************************/
/*****************************************************************************/
/*
BYTE putFULLARRAYUSBUSART() // JTR2 added reduced overhead
{
    if ((Inbdp->BDSTAT & UOWN)) {
        return 0;
    } else {
        Inbdp->BDCNT = CDC_BUFFER_SIZE;
        Inbdp->BDSTAT = ((Inbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
    }
    return CDC_BUFFER_SIZE;
}//end putUSBUSARTFAST
 */

/**********************************************************************************/
/*
BYTE getCDC_Out_ArmNext(void) {

    CDCFunctionError = 0;


    //first, get 64 bytes OR to FF FF
    if (WaitOutReady()) {

        if (IsOutBufferA) {
            OutPtr = &cdc_Out_bufferA[0];
            Outbdp->BDADDR = &cdc_Out_bufferB[0];
        } else {
            OutPtr = &cdc_Out_bufferB[0];
            Outbdp->BDADDR = &cdc_Out_bufferA[0];
        }
        IsOutBufferA ^= 0xFF;
        cdc_Out_len = Outbdp->BDCNT;
        Outbdp->BDCNT = CDC_BUFFER_SIZE;
        Outbdp->BDSTAT = ((Outbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;

    } else {
        cdc_Out_len = 0;
        CDCFunctionError = 1;
    }
    return cdc_Out_len;
}//end getCDC_Out_ArmNext

BYTE SendCDC_In_ArmNext(BYTE count) {

    CDCFunctionError = 0;
    if (WaitInReady()) {

        if (IsInBufferA) {
            Inbdp->BDADDR = cdc_In_bufferA;
            InPtr = cdc_In_bufferB;
        } else {
            Inbdp->BDADDR = cdc_In_bufferB;
            InPtr = cdc_In_bufferA;
        }
        Inbdp->BDCNT = CDC_In_count;
        Inbdp->BDSTAT = ((Inbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
        IsInBufferA ^= 0xFF;
        CDC_In_count = 0;
    } else {
        CDCFunctionError = 1;
    }
    return CDCFunctionError;
}

void ArmCDCOutDB(void) {
    Outbdp->BDSTAT &= ~UOWN; // JTR3 immediately reclaim out buffer
    Outbdp->BDADDR = &cdc_Out_bufferA[0];
    Outbdp->BDCNT = CDC_BUFFER_SIZE;
    Outbdp->BDSTAT |= UOWN;
    IsOutBufferA = 0xFF;
}

void ArmCDCInDB(void) {
    WaitInReady();
    //Inbdp->BDSTAT &= ~UOWN; // JTR3 immediately reclaim In buffer
    Inbdp->BDADDR = &cdc_In_bufferA[0];
    Inbdp->BDCNT = CDC_BUFFER_SIZE;
    //Inbdp->BDSTAT |= UOWN;
    IsInBufferA = 0xFF;
    InPtr = cdc_In_bufferA;
    CDC_In_count = 0;
}

void DisArmCDCOutDB(void) {
    Outbdp->BDSTAT &= ~UOWN; // JTR3 immediately reclaim out buffer
    Outbdp->BDADDR = &cdc_Out_buffer[0];
    Outbdp->BDCNT = CDC_BUFFER_SIZE;
    Outbdp->BDSTAT |= UOWN;
}

void DisArmCDCInDB(void) {
    WaitInReady();
    Inbdp->BDSTAT &= ~UOWN; // JTR3 immediately reclaim In buffer
    Inbdp->BDADDR = &cdc_In_buffer[0];
    Inbdp->BDCNT = CDC_BUFFER_SIZE;
    Inbdp->BDSTAT |= UOWN;
}

void SendZLP(void) {
    Inbdp->BDCNT = 0;
    Inbdp->BDSTAT = ((Inbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;

}
 */

/**********************************************************************************/
BYTE getOutReady(void) {

    return !(Outbdp->BDSTAT & UOWN); // Do we have a packet from host?
}

/**********************************************************************************/
BYTE getInReady(void) {

    return !(Inbdp->BDSTAT & UOWN); // Is the CDC In buffer ready?
}

/************************************************************************/

BYTE getsUSBUSART(BYTE *buffer, BYTE len) {
    cdc_Out_len = 0;
    if (0 == (Outbdp->BDSTAT & UOWN)) // Do we have a packet from host?
    {
        // Adjust the expected number of BYTEs to equal
        // the actual number of BYTEs received.

        if (len > Outbdp->BDCNT)
            len = Outbdp->BDCNT;

        // Copy data from dual-ram buffer to user's buffer

        for (cdc_Out_len = 0; cdc_Out_len < len; cdc_Out_len++)
            buffer[cdc_Out_len] = cdc_Out_buffer[cdc_Out_len];

        // Prepare dual-ram buffer for next OUT transaction

        if (!TestUsbInterruptEnabled()) {
            FAST_usb_handler(); // JTR2 Pop non-EP0 (USB IN) tranfers from the FIFO and also give SETUP PACKETs a chance.
        }
        Outbdp->BDCNT = CDC_BUFFER_SIZE;
        Outbdp->BDSTAT = ((Outbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;
    }//end if

    return cdc_Out_len;

}//end getsUSBUSART

void usbbufservice(void) {

    if (usbbuf.cnt == 0) {//if the buffer is empty, get more data
        usbbuf.cnt = getsUSBUSART(usbbuf.inBuf, CDC_BUFFER_SIZE); //JTR2
        usb_handler();
        usbbuf.rdptr = 0;
    }
}

//puts a byte from the buffer in the byte, returns 1 if byte

unsigned char usbbufgetbyte(unsigned char* c) {
    do {
        usbbufservice();
    } while (usbbuf.cnt == 0);
    *c = usbbuf.inBuf[usbbuf.rdptr];
    usbbuf.cnt--;
    usbbuf.rdptr++;
    return 1;
}





