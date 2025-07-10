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

// JTR v0.2a Jan 26th 2012


#include "../dp_usb/usb_stack_globals.h"     // USB stack only defines Not function related.

#if USB_EP0_BUFFER_SIZE == 8u
#elif USB_EP0_BUFFER_SIZE == 16u
#elif USB_EP0_BUFFER_SIZE == 32u
#elif USB_EP0_BUFFER_SIZE == 64u
#else
#error "USB_EP0_BUFFER_SIZE needs to be 8, 16, 32 or 64 bytes"
#endif

static void usb_send_rom(void);

ROMPTR const BYTE *usb_device_descriptor;
ROMPTR const BYTE *usb_config_descriptor;
ROMPTR const BYTE *usb_string_descriptor;
int usb_num_string_descriptors;

usb_handler_t sof_handler;
usb_handler_t class_setup_handler, vendor_setup_handler;


usb_ep_t endpoints[MAX_CHIP_EP]; // JTR change. MAX_CHIP_EP is the number of hardware endpoints on the silicon. See picusb.h

/* Allocate buffers for buffer description table and the actual buffers */
// CvD: linkscript puts it in the right memory location


// JTR comments. This below goes part way to ridding us of the need for a linker script (PIC18).
// It floats the EP0 buffers to occupy space immediately behind the buffer descriptors.
// However there are problems. It only works for the EP0 buffers because they are the only
// buffers defined in this block. It is not possible to do arithmatic within a #pragma
// therefore there seems to be no way to cause EPs in other blocks to float.
// All the buffers would need to be defined here and this would break the universal nature
// of this module. As such efforts to create a custom linker script free stack have been
// put on hold for now but can be revisited at any stage.
// Please see http://dangerousprototypes.com/forum/viewtopic.php?f=39&t=1651&start=120#p17401

#if defined(PIC_18F)
#pragma udata usb_bdt
BDentry usb_bdt[2 + 2 * MAX_EPNUM_USED]; // JTR changed index from 32 to variable
#pragma udata usb_data
//* Only claim buffer for ep 0 */
#if USB_PP_BUF_MODE == 0
BYTE usb_ep0_out_buf[USB_EP0_BUFFER_SIZE];
BYTE usb_ep0_in_buf[USB_EP0_BUFFER_SIZE];
#else
#error "Ping pong buffer not implemented yet!"
#endif

#elif defined(PIC_24F)
#pragma udata usb_bdt
BDentry usb_bdt[2 + 2 * MAX_EPNUM_USED] __attribute__((aligned(512))); // JTR changed index from 32 to variable TODO: Dynamic allocation reflecting number of used endpoints. (How to do counting in preprocessor?)
#if USB_PP_BUF_MODE == 0
BYTE usb_ep0_out_buf[USB_EP0_BUFFER_SIZE];
BYTE usb_ep0_in_buf[USB_EP0_BUFFER_SIZE];
#else
#error "Ping pong buffer not implemented yet!"
#endif
#endif

#pragma udata
unsigned int usb_device_status;
unsigned int usb_current_cfg;
volatile BYTE usb_device_state;
BYTE usb_addr_pending;
BYTE trn_status; // Global since it is needed everywere
BDentry *EP0_Outbdp, *EP0_Inbdp; // Dito
BYTE IsSuspended = 0;
ROMPTR const BYTE *usb_rom_ptr;
size_t usb_rom_len;
volatile BYTE usbrequesterrorflag;

void usb_init(ROMPTR const BYTE *device_descriptor,
        ROMPTR const BYTE *config_descriptor,
        ROMPTR const BYTE *string_descriptor,
        int num_string_descriptors) {

    usb_device_descriptor = device_descriptor;
    usb_config_descriptor = config_descriptor;
    usb_string_descriptor = string_descriptor;
    usb_num_string_descriptors = num_string_descriptors;
    sof_handler = NULL;
    class_setup_handler = NULL;
    vendor_setup_handler = NULL;
    usb_unset_in_handler(0);
    usb_unset_out_handler(0);
    ClearUSBtoDefault();
    ConfigureUsbHardware();
    EnablePacketTransfer();
}

void usb_start(void) {
    EnableUsb(); // Enable USB-hardware
    usb_device_state = ATTACHED_STATE;
    while (SingleEndedZeroIsSet()); // Busywait for initial power-up
    usb_device_state = DEFAULT_STATE; //JTR2
}

void usb_handle_error(void) {
    /* No errorhandler for now, just clear offending flag*/
    ClearAllUsbErrorInterruptFlags();
}

void usb_handle_reset(void) {
    do {
        ClearUsbInterruptFlag(USB_TRN); // JTR corrected Must poll TRN Flag and clear, then wait 6 cycles. for next flag set.
        usb_current_cfg = 0;
        usb_device_state = DEFAULT_STATE; // This BLOCK creates the requied 6 cycle delay for TRNF to reassert.
        usb_addr_pending = 0x00;
    } while (USB_TRANSACTION_FLAG);

    ClearUSBtoDefault();
    EnablePacketTransfer();
}

void ClearUSBtoDefault(void) {
    int i;
    sof_handler = NULL;
    class_setup_handler = NULL;
    vendor_setup_handler = NULL;

    SetUsbAddress(0); // After reset we don't have an address
    ResetPPbuffers();
    ClearAllUsbErrorInterruptFlags();

    for (i = 0; i < MAX_CHIP_EP; i++) {
        endpoints[i].out_handler = NULL;
        endpoints[i].in_handler = NULL;
        USB_UEP[i] = 0;
    }

    for (i = 0; i < (2 + 2 * MAX_EPNUM_USED); i++) {
        usb_bdt[i].BDSTAT = 0;
    }

    USB_UEP0 = USB_EP_CONTROL; // Configure Only ep0 At this point.
    //usbrequesterrorflag = 0;

#ifdef USB_SELF_POWERED

    // JTR TODO this isn't actually 100% correct. "usb_device_status" is a runtime variable
    // In the case of a bus powered device it will always be 0x000 but in the case of
    // self powered with bus powered option it becames variable to the current powered
    // State. This is a minor thing and for now but it may need to be addressed if there is
    // any hardware that is dual powered.

    usb_device_status = 0x0001;
#else
    usb_device_status = 0x0000;
#endif
    usb_device_state = DETACHED_STATE; // JTR added flag byte for enumeration state
    usb_current_cfg = 0; // JTR formally usb_configured
    usb_addr_pending = 0x00;

#if USB_PP_BUF_MODE == NO_PINGPONG
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDCNT = USB_EP0_BUFFER_SIZE; // JTR endpoints[0].buffer_size; same thing done more obviously
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDADDR = usb_ep0_out_buf; //endpoints[0].out_buffer;
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDSTAT = UOWN + DTSEN;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDADDR = usb_ep0_in_buf; //endpoints[0].in_buffer;
    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTS + DTSEN; // Set DTS => First packet inverts, ie. is Data0
#else
#error "Invalid PING_PONG mode"
#endif
}

void usb_handler(void) {

    if (USB_IDLE_FLAG) {
        /* Idle - suspend */
        usb_suspend(); // // Must be defined in user code.
        ClearUsbInterruptFlag(USB_IDLE);
    }

    if (USB_RESET_FLAG) {
        usb_handle_reset();
        ClearUsbInterruptFlag(USB_URST);
    }
    if (USB_ERROR_FLAG) {
        //     usb_handle_error();
        ClearAllUsbErrorInterruptFlags();
        ClearUsbInterruptFlag(USB_UERR);
    }
    if (USB_STALL_FLAG) {
        ClearUsbInterruptFlag(USB_STALL);
    }
    if (USB_SOF_FLAG) {
        /* Start-of-frame */
        if (sof_handler) sof_handler();
        ClearUsbInterruptFlag(USB_SOF);
    }

    if (USB_TRANSACTION_FLAG) {
        if (!USB_STAT2EP(GetUsbTransaction()))
            usb_handle_transaction(); // Only handle EP0 transactions.
        ClearUsbInterruptFlag(USB_TRN); // JTR Missing! This is why Ian was only getting one interrupt??
    } // Side effect: advance USTAT Fifo
}

void usb_handle_transaction(void) {

    usbrequesterrorflag = 0;

    trn_status = GetUsbTransaction();
    EP0_Outbdp = &usb_bdt[USB_USTAT2BD(trn_status)];
    EP0_Inbdp = &usb_bdt[USB_USTAT2BD(trn_status | DIRBIT)]; // All replies in IN direction

    switch (EP0_Outbdp->BDSTAT & USB_TOKEN_Mask) {
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

    EP0_Inbdp->BDSTAT = DTSEN; // Reclaim reply buffer

    EnablePacketTransfer(); // JTR this is placed here to overcome a errate issue with early PIC18 USB pics.

    switch (EP0_Outbdp->BDADDR[USB_bmRequestType] & USB_bmRequestType_TypeMask) {
        case USB_bmRequestType_Standard:
            switch (EP0_Outbdp->BDADDR[USB_bmRequestType] & USB_bmRequestType_RecipientMask) {
                case USB_bmRequestType_Device:
                    usb_handle_StandardDeviceRequest(EP0_Outbdp);
                    break;
                case USB_bmRequestType_Interface:
                    usb_handle_StandardInterfaceRequest(EP0_Outbdp);
                    break;
                case USB_bmRequestType_Endpoint:
                    usb_handle_StandardEndpointRequest(EP0_Outbdp);
                    break;
                default:
                    usb_RequestError();
            }
            break;
        case USB_bmRequestType_Class:
            if (class_setup_handler) class_setup_handler();
            break;
        case USB_bmRequestType_Vendor:
			//ROBOTS FIX: http://dangerousprototypes.com/forum/viewtopic.php?f=39&t=3849&view=unread#unread
			// did call class_setup_handler();
            if (vendor_setup_handler) vendor_setup_handler();
            break;
        default:
            usb_RequestError();
    }
    /* Prepare endpoint for new reception */

    EP0_Outbdp->BDCNT = USB_EP0_BUFFER_SIZE; // Size  of EP0, should always be ep0?

    // JTR, is the next OUT transfer to be a setup packet (DAT0) or a DATA packet (DAT1)?
    // note that this is not an entirely robust way of doing things. See the microchip stack for
    // further comments and a better system as this does not account for errors and retries
    // and it results in the SIE not accepting the final out ZLP status packet for IN transfers
    // with a data stage. However it works but it is not anything to be proud of...

    EP0_Outbdp->BDSTAT = (!(EP0_Outbdp->BDADDR[USB_bmRequestType] & USB_bmRequestType_PhaseMask) &&
            (EP0_Outbdp->BDADDR[USB_wLength] || EP0_Outbdp->BDADDR[USB_wLengthHigh])) ? UOWN + DTS + DTSEN : UOWN + DTSEN;
}

void usb_handle_StandardDeviceRequest(BDentry *bdp) {
    BYTE *packet = bdp->BDADDR;
    int i;

    switch (packet[USB_bRequest]) {
        case USB_REQUEST_GET_STATUS:
            EP0_Inbdp->BDADDR[0] = usb_device_status & 0xFF;
            EP0_Inbdp->BDADDR[1] = usb_device_status >> 8;
            usb_ack_dat1(2);
            break;
        case USB_REQUEST_CLEAR_FEATURE:
            if (0x01u == packet[USB_wValue]) { // TODO: Remove magic (REMOTE_WAKEUP_FEATURE)
                usb_device_status &= ~0x0002;
                usb_ack_dat1(0);
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_SET_FEATURE:
            if (0x01u == packet[USB_wValue]) { // TODO: Remove magic (REMOTE_WAKEUP_FEATURE)
                usb_device_status |= 0x0002;
                usb_ack_dat1(0);
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_SET_ADDRESS:
            if (0x00u == packet[USB_wValueHigh] && 0x7Fu >= packet[USB_wValue]) {
                usb_addr_pending = packet[USB_wValue];
                usb_set_in_handler(0, usb_set_address);
                usb_ack_dat1(0);
            } else
                usb_RequestError();
            break;


        case USB_REQUEST_GET_DESCRIPTOR:
            switch (packet[USB_bDescriptorType]) {
                case USB_DEVICE_DESCRIPTOR_TYPE: // There is only every one in pratice.
                    usb_rom_ptr = usb_device_descriptor;
                    usb_rom_len = usb_device_descriptor[0]; // Get BYTE length from descriptor always at byte [0]
                    if ((0 == packet[USB_wLengthHigh] && packet[USB_wLength] < usb_rom_ptr[0]))
                        usb_rom_len = packet[USB_wLength]; // If the HOST asked for LESS then must adjust count to the smaller number
                    break;

                case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                    if (packet[USB_bDescriptorIndex] >= usb_device_descriptor[17]) {
                        flag_usb_RequestError();
                        break;
                    }

                    usb_rom_ptr = usb_config_descriptor;
                    usb_rom_len = usb_rom_ptr[2] + usb_rom_ptr[3] * 256; // Get WORD length from descriptor always at bytes 2&3 (Low-High)
                    for (i = 0; i < packet[USB_bDescriptorIndex]; i++) { // Implicit linked list traversal until requested configuration
                        usb_rom_ptr += usb_rom_len;
                        usb_rom_len = usb_rom_ptr[2] + usb_rom_ptr[3] * 256; // Get (next) WORD length from descriptor always at bytes 2&3 (Low-High)
                    }
                    if ((packet[USB_wLengthHigh] < usb_rom_ptr[3]) ||
                            (packet[USB_wLengthHigh] == usb_rom_ptr[3] && packet[USB_wLength] < usb_rom_ptr[2]))
                        usb_rom_len = packet[USB_wLength] + packet[USB_wLengthHigh] * 256; // If the HOST asked for LESS then must adjust count to the smaller number
                    break;
                case USB_STRING_DESCRIPTOR_TYPE:
                    // TODO: Handle language request. For now return standard language.
                    if (packet[USB_bDescriptorIndex] >= usb_num_string_descriptors) {
                        flag_usb_RequestError();
                        break;
                    }
                    usb_rom_ptr = usb_string_descriptor;
                    usb_rom_len = usb_rom_ptr[0]; // Get BYTE length from descriptor always at byte [0]
                    for (i = 0; i < packet[USB_bDescriptorIndex]; i++) { // Implicit linked list traversal until requested configuration
                        usb_rom_ptr += usb_rom_len;
                        usb_rom_len = usb_rom_ptr[0];
                    }
                    if ((0 == packet[USB_wLengthHigh] && packet[USB_wLength] < usb_rom_ptr[0]))
                        usb_rom_len = packet[USB_wLength];
                    break;
                case USB_INTERFACE_DESCRIPTOR_TYPE:
                case USB_ENDPOINT_DESCRIPTOR_TYPE:
                default:
                    flag_usb_RequestError();
            }
            if (0 == usbrequesterrorflag) {
                usb_send_rom(); // Send first part of packet right away, the rest is handled by the EP0 IN handler.
                usb_set_in_handler(0, usb_send_rom);
            } else {
                usb_RequestError();
            }

            break;
        case USB_REQUEST_GET_CONFIGURATION:
            EP0_Inbdp->BDADDR[0] = usb_current_cfg;
            usb_ack_dat1(1);
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

                usb_ack_dat1(0);

            } else
                usb_RequestError();
            break;

        case USB_REQUEST_SET_DESCRIPTOR:
        default:
            usb_RequestError();
    }
}

void usb_handle_StandardInterfaceRequest(BDentry *bdp) {
    BYTE *packet = bdp->BDADDR;

    switch (packet[USB_bRequest]) {
        case USB_REQUEST_GET_STATUS:
            EP0_Inbdp->BDADDR[0] = 0x00;
            EP0_Inbdp->BDADDR[1] = 0x00;
            usb_ack_dat1(2);
            break;
        case USB_REQUEST_GET_INTERFACE:
            if (USB_NUM_INTERFACES > packet[USB_bInterface]) {
                // TODO: Implement alternative interfaces, or move responsibility to class/vendor functions.
                EP0_Inbdp->BDADDR[0] = 0;
                usb_ack_dat1(1);
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_SET_INTERFACE:
            if (USB_NUM_INTERFACES > packet[USB_bInterface] && 0u == packet[USB_wValue]) {
                // TODO: Implement alternative interfaces...
                usb_ack_dat1(0);
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
    BYTE *packet;
    BYTE epnum;
    BYTE dir;
    BDentry *epbd;
    usb_uep_t *pUEP;

    packet = bdp->BDADDR;

    switch (packet[USB_bRequest]) {
        case USB_REQUEST_GET_STATUS:
            EP0_Inbdp->BDADDR[0] = 0x00; // Assume no stall
            EP0_Inbdp->BDADDR[1] = 0x00; // Same for stall or not
            epnum = packet[USB_wIndex] & 0x0F;
            dir = packet[USB_wIndex] >> 7;
            epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_EVEN)];
            if (epbd->BDSTAT &= ~BSTALL)
                EP0_Inbdp->BDADDR[0] = 0x01; // EVEN BD is stall flag set?
            //epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_ODD)];
            //if (epbd->BDSTAT &= ~BSTALL)
            //    rbdp->BDADDR[0] = 0x01; // ODD BD is stall flag set?
            usb_ack_dat1(2);
            break;

        case USB_REQUEST_CLEAR_FEATURE:
            // As this is really is an application event and there
            // should be a call back and protocol for handling the
            // possible lost of a data packet.
            // TODO: ping-ping support.

            epnum = packet[USB_wIndex] & 0x0F; // JTR Added V0.2 after microchip stuff up with their documentation.
            pUEP = USB_UEP;
            pUEP += epnum;
            *pUEP &= ~USB_UEP_EPSTALL;

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


            usb_ack_dat1(0);
            break;


        case USB_REQUEST_SET_FEATURE:
            epnum = packet[USB_wIndex] & 0x0F;
            dir = packet[USB_wIndex] >> 7;
            epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_EVEN)];
            epbd->BDSTAT |= BSTALL;
            //epbd = &usb_bdt[USB_CALC_BD(epnum, dir, USB_PP_ODD)];
            //epbd->BDSTAT |= BSTALL;
            usb_ack_dat1(0);
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

// JTR New added helper function use extensively by the standard and class
// request handlers. All status IN packets are DAT1 as is the first DATA packet
// of a IN transfer. Currently with this CDC stack the only IN DATA transfers
// that are > 8 bytes is the descriptor transfer and these are transfered in
// usb_send_rom()

void usb_ack_dat1(int bdcnt) {
    EP0_Inbdp->BDCNT = (bdcnt & 0xFF);
    EP0_Inbdp->BDSTAT = (DTS | UOWN | DTSEN); // | ((bdcnt & 0x300) >> 8));
}

void usb_RequestError(void) {

    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDCNT = USB_EP0_BUFFER_SIZE;
    //usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_ODD)].BDCNT = USB_EP0_BUFFER_SIZE;

    usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = UOWN + BSTALL;
    usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDSTAT = UOWN + BSTALL;
    //usb_bdt[USB_CALC_BD(0, USB_DIR_IN, USB_PP_ODD)].BDSTAT = UOWN + BSTALL;
    //usb_bdt[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_ODD)].BDSTAT = UOWN + BSTALL;

    // JTR TODO: Should also kill the IN and OUT handlers?

}

void usb_set_address(void) {
    if (0x00u == usb_addr_pending) {
        usb_device_state = DEFAULT_STATE;
    } else {
        usb_device_state = ADDRESS_STATE;
    }
    SetUsbAddress(usb_addr_pending);
    usb_addr_pending = 0xFF;
    usb_unset_in_handler(0); // Unregister handler
}

static void usb_send_rom(void) {

    unsigned int i;
    size_t packet_len;
    if (usb_rom_len) {
        packet_len = (usb_rom_len < USB_EP0_BUFFER_SIZE) ? usb_rom_len : USB_EP0_BUFFER_SIZE; // JTR changed from MAX_BUFFER_SIZE

        for (i = 0; i < packet_len; i++) {
            EP0_Inbdp->BDADDR[i] = usb_rom_ptr[i];
        }
    } else {
        packet_len = 0;
        usb_unset_in_handler(0);
    }

    EP0_Inbdp->BDCNT = (BYTE) packet_len;
    EP0_Inbdp->BDSTAT = ((EP0_Inbdp->BDSTAT ^ DTS) & DTS) | UOWN | DTSEN; // Packet length always less then 256 on endpoint 0

    usb_rom_ptr += packet_len;
    usb_rom_len -= packet_len;
}

