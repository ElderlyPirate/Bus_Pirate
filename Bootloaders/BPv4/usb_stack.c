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

Influence and inspiration taken from http://pe.ece.olin.edu/ece/projects.html
 */

// JTR v0.1d

#include "globals.h"    // Compiler STD etc. Not function related.

#if USB_EP0_BUFFER_SIZE == 8u
#elif USB_EP0_BUFFER_SIZE == 16u
#elif USB_EP0_BUFFER_SIZE == 32u
#elif USB_EP0_BUFFER_SIZE == 64u
#else
#error "USB_EP0_BUFFER_SIZE needs to be 8, 16, 32 or 64 bytes"
#endif


ROMPTR const unsigned char *usb_device_descriptor;
ROMPTR const unsigned char *usb_config_descriptor;
ROMPTR const unsigned char *usb_string_descriptor;
int usb_num_string_descriptors;

usb_handler_t sof_handler;
usb_handler_t class_setup_handler, vendor_setup_handler;

usb_ep_t endpoints[MAX_CHIP_EP]; // JTR change. MAX_CHIP_EP is the number of hardware endpoints on the silicon. See picusb.h

// JTR comments. This below goes part way to ridding us of the need for a linker script (PIC18).
// It floats the EP0 buffers to occupy space immediately behind the buffer descriptors.
// However there are problems. It only works for the EP0 buffers because they are the only
// buffers defined in this block. It is not possible to do arithmatic within a #pragma
// therefore there seems to be no way to cause EPs in other blocks to float.
// All the buffers would need to be defined here and this would break the universal nature
// of this module. As such efforts to create a custom linker script free stack have been
// put on hold for now but can be revisited at any stage.
// Please see http://dangerousprototypes.com/forum/viewtopic.php?f=39&t=1651&start=120#p17401


#pragma udata usb_bdt
BDentry usb_bdt[2 + 2 * MAX_EPNUM_USED] __attribute__((aligned(512))); // JTR changed index from 32 to variable TODO: Dynamic allocation reflecting number of used endpoints. (How to do counting in preprocessor?)
#if USB_PP_BUF_MODE == 0
unsigned char usb_ep0_out_buf[USB_EP0_BUFFER_SIZE];
unsigned char usb_ep0_in_buf[USB_EP0_BUFFER_SIZE];
#else
#error "Ping pong buffer not implemented yet!"
#endif


#pragma udata
unsigned int usb_device_status;
unsigned int usb_current_cfg;
volatile unsigned char usb_device_state;
unsigned char usb_addr_pending;
usb_status_t trn_status; // Global since it is needed everywere
BDentry *bdp, *rbdp; // Dito

const unsigned char *usb_desc_ptr;
size_t usb_desc_len;

/* Forward Reference Prototypes */
void usb_handle_error(void);
void usb_handle_reset(void);
void usb_handle_transaction(void);
void usb_handle_setup(void);
void usb_handle_out(void);
void usb_handle_in(void);
void usb_handle_StandardDeviceRequest(BDentry*);
void usb_handle_StandardInterfaceRequest(BDentry*);
void usb_handle_StandardEndpointRequest(BDentry*);
//void usb_handle_ClassRequest( void );
//void usb_handle_VendorRequest( void );
void usb_RequestError(void);

void usb_ack_dat1(BDentry *rbdp, int bdcnt); // JTR added
void usb_set_address(void);
void usb_send_descriptor(void);
void user_configured_init(void); //JTR added.Non EP0 endpoints are arm after USB device enters CONFIGURED STATE. This is a CALLBACK function.
BYTE FAST_usb_handler(void); // JTR2 added fast USB service. Pops non EP0 transfer flags from FIFO. Sevices RESET and SETUP

void usb_init(ROMPTR const unsigned char *device_descriptor,
        ROMPTR const unsigned char *config_descriptor,
        ROMPTR const unsigned char *string_descriptor,
        int num_string_descriptors) {
    int i;


    usb_device_descriptor = device_descriptor;
    usb_config_descriptor = config_descriptor;
    usb_string_descriptor = string_descriptor;
    usb_num_string_descriptors = num_string_descriptors;

    SetUsbAddress(0); // JTR added here. Probably not required though
    ResetPPbuffers();
    DisableUsbInterrupts();
    DisableAllUsbInterrupts();
    ClearAllUsbErrorInterruptFlags();
    ClearAllUsbInterruptFlags();
    ConfigureUsbHardware();

    sof_handler = NULL;
    class_setup_handler = NULL;
    vendor_setup_handler = NULL;

    for (i = 0; i < MAX_CHIP_EP; i++) {
        endpoints[i].out_handler = NULL;
        endpoints[i].in_handler = NULL;
    }
    // Register ep0 - no handlers

    usb_unset_in_handler(0);
    usb_unset_out_handler(0);

    // JTR All UEPx SPRs are hard coded. I cannot see much point of tables or indexing for these.
    // On the PIC16C765 such indexing was required for the STALL bit but now the STALL feature
    // is in the Buffer descriptor table not in the UEPx SPRs.
    // See changes to "USB_REQUEST_GET_STATUS"

    //	USB_UEP[0] = endpoints[0].type;

    USB_UEP0 = USB_EP_CONTROL;
    /* Configure endpoints TODO: Only ep0 ? */
    // JTR Right! code for other end points snipped...At this point we are only setting up EP0

#ifdef USB_SELF_POWERED
    usb_device_status = 0x0001;
#else
    usb_device_status = 0x0000;
#endif
    usb_device_state = 0x00; // JTR added flag byte for enumeration state
    usb_current_cfg = 0; // JTR formally usb_configured
    usb_addr_pending = 0x00;

#if USB_PP_BUF_MODE == 0
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDCNT = USB_EP0_BUFFER_SIZE; // JTR endpoints[0].buffer_size; same thing done more obviously
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDADDR = usb_ep0_out_buf; //endpoints[0].out_buffer;
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDSTAT = UOWN + DTSEN;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDADDR = usb_ep0_in_buf; //endpoints[0].in_buffer;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTS; // Set DTS => First packet inverts, ie. is Data0
#else
    // TODO: Implement Ping-Pong buffering setup.
#error "PP Mode not implemented yet"
#endif
}

void usb_start(void) {
    EnableUsb(); // Enable USB-hardware
    //uLedOn();
    while (SingleEndedZeroIsSet()); // Busywait for initial power-up
    uLedOn();

#ifdef USB_INTERRUPT
    // EnableUsbInterrupt(USB_STALL + USB_IDLE + USB_TRN + USB_SOF + USB_UERR + USB_URST);  //We don't enable ACTIV unless we are going into suspend mode.
    EnableAllUsbErrorInterrupts();
    EnableUsbInterrupt(USB_TRN + USB_SOF + USB_UERR + USB_URST);
    EnableUSBHighInterrupts();
#endif
}

void usb_handler(void) {
    if (USB_RESET_FLAG) {
        usb_handle_reset();
        ClearUsbInterruptFlag(USB_URST);
    } else if (USB_ERROR_FLAG) {
        usb_handle_error();
        ClearUsbInterruptFlag(USB_UERR);
    } else if (USB_ACTIVITY_FLAG) {
        /* Activity - unsuspend */
        //WakeupUsb();
        ClearUsbInterruptFlag(USB_ACTIV);
    } else if (USB_IDLE_FLAG) {
        /* Idle - suspend */
        //SuspendUsb();
        //usb_low_power_request();
        ClearUsbInterruptFlag(USB_IDLE);
    } else if (USB_STALL_FLAG) {
        /* Stall detected
         * Not sure when this interrupt fires
         * as the host never should send out a stall.
         * Perhaps as a respons to our own stalls?
         * For now just ignore it. */
        ClearUsbInterruptFlag(USB_STALL);
    } else if (USB_SOF_FLAG) {
        /* Start-of-frame */
        //if (sof_handler) sof_handler();
        {
            ClearUsbInterruptFlag(USB_SOF);
        }
    } else {
        // process all pending transactions
        while (USB_TRANSACTION_FLAG) {
            usb_handle_transaction();
            ClearUsbInterruptFlag(USB_TRN);
        } // Side effect: advance USTAT Fifo
    }
}

void usb_handle_error(void) {
    /* No errorhandler for now, just clear offending flag*/
    ClearAllUsbErrorInterruptFlags();
}

void usb_handle_reset(void) {
    int i;
    mLedToggle();
#ifdef USB_SELF_POWERED  // JTR see note about this in usb_init()
    usb_device_status = 0x0001;
#else
    usb_device_status = 0x0000;
#endif

    do {
        ClearUsbInterruptFlag(USB_TRN); // JTR corrected Must poll TRN Flag and clear, then wait 6 cycles. for next flag set.
        usb_current_cfg = 0;
        usb_device_state = 0x00; // This creates the requied 6 cycle delay for TRNF to reassert.
        usb_addr_pending = 0x00;
    } while (USB_TRANSACTION_FLAG);

    //        for (i=0; i < MAX_CHIP_EP; i++ )      // JTR this loop seems to work proving that the USB_UEP ptr is now correct.
    //        {
    //            USB_UEP[i] = 0;                   // JTR Char on PIC18 and int ptr on PIC24
    //        }
    USB_UEP0 = 0; //  Disable all endpoints
    USB_UEP1 = 0;
    USB_UEP2 = 0;
    USB_UEP3 = 0;
    USB_UEP4 = 0;
    USB_UEP5 = 0;
    USB_UEP6 = 0;
    USB_UEP7 = 0;
#if MAX_CHIP_EP > 8 // JTR alteration
    USB_UEP8 = 0;
    USB_UEP9 = 0;
    USB_UEP10 = 0;
    USB_UEP11 = 0;
    USB_UEP12 = 0;
    USB_UEP13 = 0;
    USB_UEP14 = 0;
    USB_UEP15 = 0;

#endif
    SetUsbAddress(0); // After reset we don't have an address
    ClearAllUsbInterruptFlags();
    ClearAllUsbErrorInterruptFlags();

    // JTR added. Clear all BD STAT registers so that they are owned by the CPU and it is safe to change them.

    for (i = 0; i < (2 + 2 * MAX_EPNUM_USED); i += 4) {
        usb_bdt[i].BDSTAT = 0;
    }
    //		USB_UEP[0] = endpoints[0].type;		// JTR removed all such indexing on UEPx
    USB_UEP0 = USB_EP_CONTROL; // JTR hard coded instead.

    /* Configure buffer descriptors */

#if USB_PP_BUF_MODE == 0
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDCNT = USB_EP0_BUFFER_SIZE; //endpoints[0].buffer_size;
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDADDR = usb_ep0_out_buf; //endpoints[0].out_buffer;
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDSTAT = UOWN + DTSEN;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDADDR = usb_ep0_in_buf; //endpoints[0].in_buffer;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTS; // Set DTS => First packet inverts, ie. is Data0

#else
    // TODO: Implement Ping-Pong buffering setup.
#error "PP Mode not implemented yet"
#endif
    EnablePacketTransfer();
}

void usb_handle_transaction(void) {
    uLedToggle();
    trn_status = GetUsbTransaction();
    bdp = &usb_bdt[USB_USTAT2BD(trn_status)];
    // JTR PIC24 fixup Added DIRBIT as it is different for PIC24F ( and different again for the PIC24E/dsPIC33E)
    // the USB_USTAT2BD macro is also altered to shift the result the correct number of places.

    rbdp = &usb_bdt[USB_USTAT2BD(trn_status | DIRBIT)]; // All replies in IN direction

    switch (bdp->BDSTAT & USB_TOKEN_Mask) {
        case USB_TOKEN_SETUP:
            usb_handle_setup();
            break;
        case USB_TOKEN_OUT:
            usb_handle_out();
            break;
        case USB_TOKEN_IN:
            usb_handle_in();
            break;
            //default:
            /* Default case of unknown TOKEN - discard */
    }
}

void usb_handle_setup(void) {
    rbdp->BDSTAT = DTSEN; // Reclaim reply buffer
    switch (bdp->BDADDR[USB_bmRequestType] & USB_bmRequestType_TypeMask) {
        case USB_bmRequestType_Standard:
            switch (bdp->BDADDR[USB_bmRequestType] & USB_bmRequestType_RecipientMask) {
                case USB_bmRequestType_Device:
                    usb_handle_StandardDeviceRequest(bdp);
                    break;
                case USB_bmRequestType_Interface:
                    usb_handle_StandardInterfaceRequest(bdp);
                    break;
                case USB_bmRequestType_Endpoint:
                    usb_handle_StandardEndpointRequest(bdp);
                    break;
                default:
                    usb_RequestError();
            }
            break;
        case USB_bmRequestType_Class:
            if (class_setup_handler) class_setup_handler();
            break;
        case USB_bmRequestType_Vendor:
            if (vendor_setup_handler) class_setup_handler();
            break;
        default:
            usb_RequestError();
    }
    /* Prepare endpoint for new reception */

    bdp->BDCNT = USB_EP0_BUFFER_SIZE; // Size  of EP0, should always be ep0? (JTR in practice YES)

    // JTR, is the next OUT transfer to be a setup packet (DAT0) or a DATA packet (DAT1)?
    // note that this is not an entirely robust way of doing things. See the microchip stack for
    // further comments and a better system as this does not account for errors and retries.

    // JTR Breakdown of what is happening here FYI.

    //bdp->BDSTAT = (!(bdp->BDADDR[USB_bmRequestType] & USB_bmRequestType_PhaseMask) &&
    // JTR meaning. If transfer was a CONTROL OUT then B7 (USB_bmRequestType_PhaseMask) of bmRequestType is '0'

    // &&

    //bdp->BDADDR[USB_wLength] || bdp->BDADDR[USB_wLengthHigh]))? UOWN + DTS + DTSEN : UOWN + DTSEN;
    // If there is a CONTROL OUT DATA PACKET to follow then its count in (int) wLength will be != 0
    // When both conditions are true then set for DAT1 (UOWN + DTS + DTSEN)
    // else set for DAT0 (UOWN + DTSEN)

    // See USB 2.0 spec 8.5.3

    bdp->BDSTAT = (!(bdp->BDADDR[USB_bmRequestType] & USB_bmRequestType_PhaseMask) &&
            (bdp->BDADDR[USB_wLength] || bdp->BDADDR[USB_wLengthHigh])) ? UOWN + DTS + DTSEN : UOWN + DTSEN;

    // JTR Note. For rbdp after a setup packet, the standard or class request handler will force the correct DTS state
    // JTR Note. that CONTROL IN and OUT DATA packet transfers do not come back here and there is no
    // univesal way and place of setting up EP0 after these DATA transfers in this stack.

    // JTR Note. that there is a PIC18 silicon errata issue that this does not address by being here.  See DS80220F-page 6
    EnablePacketTransfer();
}

// JTR comment on the standard device requests handler.
// This does not address that some requests need to be
// handled differently depending on what USB defined
// "STATE" of enumeration we are at. In fact this stack
// does not even track any of the different states.
// except that I have added basic tracking to determine
// if the device is configured or not. This is a minimum.
// Later the stack can be improved.

void usb_handle_StandardDeviceRequest(BDentry *bdp) {
    unsigned char *packet = bdp->BDADDR;
    int i;

    switch (packet[USB_bRequest]) {
        case USB_REQUEST_GET_STATUS:
            rbdp->BDADDR[0] = usb_device_status & 0xFF;
            rbdp->BDADDR[1] = usb_device_status >> 8;

            // JTR I added usb_ack_dat1() simply so that the handling
            // of STANDARD and CLASS requests (in cdc.c) were consistant
            // rather than have the same thing done in two different ways.

            usb_ack_dat1(rbdp, 2); // JTR common addition for STD and CLASS ACK
            break;
        case USB_REQUEST_CLEAR_FEATURE:
            if (0x01u == packet[USB_wValue]) { // TODO: Remove magic (REMOTE_WAKEUP_FEATURE)
                usb_device_status &= ~0x0002;
                usb_ack_dat1(rbdp, 0); // JTR common addition for STD and CLASS ACK
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_SET_FEATURE:
            if (0x01u == packet[USB_wValue]) { // TODO: Remove magic (REMOTE_WAKEUP_FEATURE)
                usb_device_status |= 0x0002;
                usb_ack_dat1(rbdp, 0); // JTR common addition for STD and CLASS ACK
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_SET_ADDRESS:
            if (0x00u == packet[USB_wValueHigh] && 0x7Fu >= packet[USB_wValue]) {
                usb_addr_pending = packet[USB_wValue];
                usb_ack_dat1(rbdp, 0); // JTR common addition for STD and CLASS ACK
                usb_set_in_handler(0, usb_set_address);
            } else
                usb_RequestError();
            break;


        case USB_REQUEST_GET_DESCRIPTOR:
            switch (packet[USB_bDescriptorType]) {
                case USB_DEVICE_DESCRIPTOR_TYPE:
                    usb_desc_ptr = usb_device_descriptor;
                    usb_desc_len = usb_device_descriptor[0];
                    if ((0 == packet[USB_wLengthHigh] && packet[USB_wLength] < usb_desc_ptr[0]))
                        usb_desc_len = packet[USB_wLength];
                    break;
                case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                    if (packet[USB_bDescriptorIndex] >= usb_device_descriptor[17]) // TODO: remove magic
                        usb_RequestError();
                    usb_desc_ptr = usb_config_descriptor;
                    usb_desc_len = usb_desc_ptr[2] + usb_desc_ptr[3] * 256;
                    for (i = 0; i < packet[USB_bDescriptorIndex]; i++) { // Implicit linked list traversal until requested configuration
                        usb_desc_ptr += usb_desc_len;
                        usb_desc_len = usb_desc_ptr[2] + usb_desc_ptr[3] * 256;
                    }
                    if ((packet[USB_wLengthHigh] < usb_desc_ptr[3]) ||
                            (packet[USB_wLengthHigh] == usb_desc_ptr[3] && packet[USB_wLength] < usb_desc_ptr[2]))
                        usb_desc_len = packet[USB_wLength] + packet[USB_wLengthHigh] * 256;
                    break;
                case USB_STRING_DESCRIPTOR_TYPE:
                    // TODO: Handle language request. For now return standard language.
                    if (packet[USB_bDescriptorIndex] >= usb_num_string_descriptors)
                        usb_RequestError();
                    usb_desc_ptr = usb_string_descriptor;
                    usb_desc_len = usb_desc_ptr[0];
                    for (i = 0; i < packet[USB_bDescriptorIndex]; i++) { // Implicit linked list traversal until requested configuration
                        usb_desc_ptr += usb_desc_len;
                        usb_desc_len = usb_desc_ptr[0];
                    }
                    if ((0 == packet[USB_wLengthHigh] && packet[USB_wLength] < usb_desc_ptr[0]))
                        usb_desc_len = packet[USB_wLength];
                    break;
                case USB_INTERFACE_DESCRIPTOR_TYPE:
                case USB_ENDPOINT_DESCRIPTOR_TYPE:
                default:
                    usb_RequestError();
            }
            usb_send_descriptor(); // Send first part of packet right away
            usb_set_in_handler(0, usb_send_descriptor);
            break;
        case USB_REQUEST_GET_CONFIGURATION:
            rbdp->BDADDR[0] = usb_current_cfg;
            usb_ack_dat1(rbdp, 1); // JTR common addition for STD and CLASS ACK
            //		rbdp->BDCNT = 1;
            //		rbdp->BDSTAT = UOWN + DTS + DTSEN;
            break;

        case USB_REQUEST_SET_CONFIGURATION:
            if (USB_NUM_CONFIGURATIONS >= packet[USB_wValue]) {
                // TODO: Support multiple configurations
                /* Configure endpoints (USB_UEPn - registers) */
                usb_current_cfg = packet[USB_wValue];
                if (usb_current_cfg != 0) {

                    // JTR user_configured_init major addition. This is a CALLBACK to the USER when the device is enumerated.
                    // This is when we setup non EP0 endpoints.
                    // TODO: This really could be a function pointer

                    usb_device_state = CONFIGURED_STATE;
                    user_configured_init();
                } else {
                    usb_device_state = ADDRESS_STATE;
                }

                usb_ack_dat1(rbdp, 0); // JTR common addition for STD and CLASS ACK

            } else
                usb_RequestError();
            break;

        case USB_REQUEST_SET_DESCRIPTOR:
        default:
            usb_RequestError();
    }
}

void usb_handle_StandardInterfaceRequest(BDentry *bdp) {
    unsigned char *packet = bdp->BDADDR;

    switch (packet[USB_bRequest]) {
        case USB_REQUEST_GET_STATUS:
            rbdp->BDADDR[0] = 0x00;
            rbdp->BDADDR[1] = 0x00;
            usb_ack_dat1(rbdp, 2); // JTR common addition for STD and CLASS ACK
            //		rbdp->BDCNT = 2;
            //		rbdp->BDSTAT = UOWN + DTS + DTSEN;
            break;
        case USB_REQUEST_GET_INTERFACE:
            if (USB_NUM_INTERFACES > packet[USB_bInterface]) {
                // TODO: Implement alternative interfaces, or move responsibility to class/vendor functions.
                rbdp->BDADDR[0] = 0;
                usb_ack_dat1(rbdp, 1); // JTR common addition for STD and CLASS ACK
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_SET_INTERFACE:
            if (USB_NUM_INTERFACES > packet[USB_bInterface] && 0u == packet[USB_wValue]) {
                // TODO: Implement alternative interfaces...
                usb_ack_dat1(rbdp, 0); // JTR common addition for STD and CLASS ACK
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_CLEAR_FEATURE: // JTR N/A for interface
        case USB_REQUEST_SET_FEATURE: // This is correct and finished code.
        default:
            usb_RequestError();
    }
}

void usb_handle_StandardEndpointRequest(BDentry *bdp) {
    unsigned char *packet;
    unsigned char epnum;
    unsigned char dir;
    BDentry *epbd;

    packet = bdp->BDADDR;

    switch (packet[USB_bRequest]) {
        case USB_REQUEST_GET_STATUS:
            rbdp->BDADDR[0] = 0x00; // Assume no stall
            rbdp->BDADDR[1] = 0x00; // Same for stall or not
            epnum = packet[USB_wIndex] & 0x0F;
            dir = packet[USB_wIndex] >> 7;
            epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_EVEN)];
            if (epbd->BDSTAT &= ~BSTALL)
                rbdp->BDADDR[0] = 0x01; // EVEN BD is stall flag set?
            epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_ODD)];
            if (epbd->BDSTAT &= ~BSTALL)
                rbdp->BDADDR[0] = 0x01; // ODD BD is stall flag set?
            usb_ack_dat1(rbdp, 2); // JTR common addition for STD and CLASS ACK
            break;

        case USB_REQUEST_CLEAR_FEATURE:
            epnum = packet[USB_wIndex] & 0x0F;
            dir = packet[USB_wIndex] >> 7;
            epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_EVEN)];
            epbd->BDSTAT &= ~BSTALL;
            if (dir) epbd->BDSTAT |= DTS; // JTR added IN EP set DTS as it will be toggled to zero next transfer
            if (0 == dir) epbd->BDSTAT &= ~DTS; // JTR added

            // JTR this pointless ATM. If ping-pong is enabled then you need to track PPBI
            // and set up ODD and EVEN BDs in respect to this. See complicated system in
            // microchip stack >= 2.8

            //		epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_ODD)];
            //		epbd->BDSTAT &= ~BSTALL;
            //		if (dir) epbd->BDSTAT |= DTS;		// JTR added
            //		if (0 == dir) epbd->BDSTAT &= ~DTS;	// JTR added


            usb_ack_dat1(rbdp, 0); // JTR common addition for STD and CLASS ACK
            //		rbdp->BDCNT = 0;
            //		rbdp->BDSTAT = UOWN + DTS + DTSEN;
            break;


        case USB_REQUEST_SET_FEATURE:
            epnum = packet[USB_wIndex] & 0x0F;
            dir = packet[USB_wIndex] >> 7;
            epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_EVEN)];
            epbd->BDSTAT |= BSTALL;
            epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_ODD)];
            epbd->BDSTAT |= BSTALL;
            usb_ack_dat1(rbdp, 0); // JTR common addition for STD and CLASS ACK
            //		rbdp->BDCNT = 0;
            //		rbdp->BDSTAT = UOWN + DTS + DTSEN;
            break;
        case USB_REQUEST_SYNCH_FRAME:
        default:
            usb_RequestError();
    }
}

void usb_handle_in(void) {
    if (endpoints[USB_STAT2EP(trn_status)].in_handler) {
        endpoints[USB_STAT2EP(trn_status)].in_handler();
    }
}

void usb_handle_out(void) {
    if (endpoints[USB_STAT2EP(trn_status)].out_handler) {
        endpoints[USB_STAT2EP(trn_status)].out_handler();
    }
}

void usb_register_sof_handler(usb_handler_t handler) {
    sof_handler = handler;
}

void usb_register_class_setup_handler(usb_handler_t handler) {
    class_setup_handler = handler;
}

void usb_register_vendor_setup_handler(usb_handler_t handler) {
    vendor_setup_handler = handler;
}

void usb_set_in_handler(int ep, usb_handler_t in_handler) {
    endpoints[ep].in_handler = in_handler;
}

void usb_set_out_handler(int ep, usb_handler_t out_handler) {
    endpoints[ep].out_handler = out_handler;
}

// JTR not actually using these two helper functions below
// but they are still useful and could be put back to use
// as they aught to result in smaller code. For now I am
// using the usb_ack_dat1() as it more correctly forces a
// DAT1 transfer and passes a varible for the transfer count.

void usb_ack(BDentry *bd) {
    bd->BDSTAT = ((bd->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN; // TODO: Accomodate for >=256 byte buffers
}

void usb_ack_zero(BDentry *bd) {
    bd->BDCNT = 0;
    bd->BDSTAT = ((bd->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN;

}
// JTR New added helper function use extensively by
// the standard and class request handlers. All status
// IN packets are DAT1 as is the first DATA packet of
// a IN transfer. Currently with this CDC stack the
// only IN DATA transfers that are > 8 bytes is the
// descriptor transfer and these are transfered in
// usb_send_descriptor()

void usb_ack_dat1(BDentry *rbdp, int bdcnt) {
    rbdp->BDCNT = (bdcnt & 0xFF);
    rbdp->BDSTAT = (DTS | UOWN | DTSEN); // | ((bdcnt & 0x300) >> 8));
}

void usb_RequestError(void) {

    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDCNT = USB_EP0_BUFFER_SIZE;
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_ODD)].BDCNT = USB_EP0_BUFFER_SIZE;

    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = UOWN + BSTALL;
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDSTAT = UOWN + BSTALL;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_ODD)].BDSTAT = UOWN + BSTALL;
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_ODD)].BDSTAT = UOWN + BSTALL;

    // JTR TODO: Should also kill the IN and OUT handlers?

}

void usb_set_address(void) {
    //	if (0x00u <= usb_addr_pending && 0x80u > usb_addr_pending) {   // JTR changed. The USB specs allow for a device to be de-addressed
    if (0x80u > usb_addr_pending) {
        SetUsbAddress(usb_addr_pending);
        usb_addr_pending = 0xFF;
    }
    // JTR TODO: If we are going to bounds check the pending address
    // (again) and it is out of bounds we should return a
    // protocol stall here.
    usb_unset_in_handler(0); // Unregister handler
}

void usb_send_descriptor(void) {

    // JTR comment While this function is being used only for the descriptors
    // there is no reason why it cannot be a generic CONTROL IN transfer handler.
    // To that ends it needs a flag to denote if it is copying from a RAM or ROM
    // pointer

    unsigned int i;
    // JTR N/A	BDentry *bd;
    size_t packet_len;
    if (usb_desc_len) {
        packet_len = (usb_desc_len < USB_EP0_BUFFER_SIZE) ? usb_desc_len : USB_EP0_BUFFER_SIZE; // JTR changed from MAX_BUFFER_SIZE
        for (i = 0; i < packet_len; i++) {
            rbdp->BDADDR[i] = usb_desc_ptr[i];
        }
    } else {

        // JTR comment. So the transfer is now completed.
        packet_len = 0; // Send a last ack
        usb_unset_in_handler(0);
    }

    rbdp->BDCNT = (unsigned char) packet_len;
    rbdp->BDSTAT = ((rbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN; // Packet length always less then 256 on endpoint 0

    usb_desc_ptr += packet_len;
    usb_desc_len -= packet_len;
}

BYTE FAST_usb_handler(void) {
    if (TestGlobalUsbInterruptFlag()) {
        if (USB_RESET_FLAG) {
            usb_handle_reset();
            ClearUsbInterruptFlag(USB_URST);
            return 0xFF;
        }
        if (USB_TRANSACTION_FLAG) {
            trn_status = GetUsbTransaction();
            if (USB_STAT2EP(trn_status)) {
                usb_handle_transaction();
                ClearUsbInterruptFlag(USB_TRN); // non-EP0 only
                return 0xFF;
            } else {
                ClearUsbInterruptFlag(USB_TRN); // non-EP0 only
            }
        }
        ClearGlobalUsbInterruptFlag();
    }
    return 0;
}

