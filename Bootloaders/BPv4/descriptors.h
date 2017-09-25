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

#ifndef __DESCRIPTORS_H__
#define __DESCRIPTORS_H__

// JTR v0.1a

#include "usb_stack.h"
#include "usb_lang.h"
#include "boot_config.h"



/* String identifiers */
#define USB_iManufacturer               1u
#define USB_iProduct                    2u
#define USB_iSerialNum                  3u


const unsigned char cdc_device_descriptor[] = {
        0x12,                                                           // bLength
        USB_DEVICE_DESCRIPTOR_TYPE,                     // bDescriptorType
        0x10,                                                           // bcdUSB (low byte)
        0x01,                                                           // bcdUSB (high byte)
        0x02,                                                           // bDeviceClass
        0x00,                                                           // bDeviceSubClass
        0x00,                                                           // bDeviceProtocol
// JTR  removed       USB_MAX_BUFFER_SIZE,                            // bMaxPacketSize
                USB_EP0_BUFFER_SIZE,

        LOWB(USB_VID),                                          // idVendor (low byte)
        HIGHB(USB_VID),                                         // idVendor (high byte)
        LOWB(USB_PID),                                          // idProduct (low byte)
        HIGHB(USB_PID),                                         // idProduct (high byte)
        LOWB(USB_DEV),                                          // bcdDevice (low byte)
        HIGHB(USB_DEV),                                         // bcdDevice (high byte)
        USB_iManufacturer,                                      // iManufacturer
        USB_iProduct,                                           // iProduct
        USB_iSerialNum,                                         // iSerialNumber (none)
        USB_NUM_CONFIGURATIONS                          // bNumConfigurations 
};

#define USB_CONFIG_DESC_TOT_LENGTH (9+9+5+4+5+5+7+9+7+7)
const unsigned char cdc_config_descriptor[] = {
        0x09,                                                           // bLength
        USB_CONFIGURATION_DESCRIPTOR_TYPE,      // bDescriptorType
        LOWB(USB_CONFIG_DESC_TOT_LENGTH),       // wTotalLength (low byte), TODO: Automatic calculation - sizeof doesn't work here
        HIGHB(USB_CONFIG_DESC_TOT_LENGTH),      // wTotalLength (high byte)
        USB_NUM_INTERFACES,                                     // bNumInterfaces
        0x01,                                                           // bConfigurationValue
        0x00,                                                           // iConfiguration (0=none)
        0x80,                                                           // bmAttributes (0x80 = bus powered)
        0x64,                                                           // bMaxPower (in 2 mA units, 50=100 mA)

        0x09,                                                           // bLength (Interface0 descriptor starts here)
        USB_INTERFACE_DESCRIPTOR_TYPE,          // bDescriptorType
        0x00,                                                           // bInterfaceNumber
        0x00,                                                           // bAlternateSetting
        0x01,                                                           // bNumEndpoints (excluding EP0)  
        0x02,                                                           // bInterfaceClass (0x00=per endpoint specified, 0xFF=vendor specific)
        0x02,                                                           // bInterfaceSubClass (0x00=per endpoint specified, 0xFF=vendor specific)
        0x01,                                                           // bInterfaceProtocol (0x00=no protocol, 0xFE=as by command set, 0xFF=vendor specific)
        0x00,                                                           // iInterface (none)

        0x05,                                                           // bFunctionLength
        0x24,                                                           // bDescriptorType
        0x00,                                                           // bDescriptorSubtype (CDC header descriptor)
        0x10,                                                           // bcdCDC (low byte)
        0x01,                                                           // bcdCDC (high byte)

        0x04,                                                           // bFunctionLength
        0x24,                                                           // bDescriptorType
        0x02,                                                           // bDescriptorSubtype (CDC abstract control management descriptor)
        0x02,                                                           // bmCapabilities

        0x05,                                                           // bFunctionLength
        0x24,                                                           // bDescriptorType
        0x06,                                                           // bDescriptorSubtype (CDC union descriptor)
        0x00,                                                           // bControlInterface
        0x01,                                                           // bSubordinateInterface0

        0x05,                                                           // bFunctionLength
        0x24,                                                           // bDescriptorType
        0x01,                                                           // bDescriptorSubtype (Call management descriptor)
        0x01,                                                           // bmCapabilities
        0x01,                                                           // bInterfaceNum

        0x07,                                                           // bLength (Endpoint1 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,           // bDescriptorType
        0x81,                                                           // bEndpointAddress
        0x03,                                                           // bmAttributes (0x03=intr)
        LOWB(CDC_NOTICE_BUFFER_SIZE),                           // wMaxPacketSize (low byte)
        HIGHB(CDC_NOTICE_BUFFER_SIZE),                          // wMaxPacketSize (high byte)
        0x40,                                                           // bInterval

        0x09,                                                           // bLength (Interface1 descriptor)
        USB_INTERFACE_DESCRIPTOR_TYPE,          // bDescriptorType
        0x01,                                                           // bInterfaceNumber
        0x00,                                                           // bAlternateSetting
        0x02,                                                           // bNumEndpoints
        0x0A,                                                           // bInterfaceClass
        0x00,                                                           // bInterfaceSubClass
        0x00,                                                           // bInterfaceProtocol (0x00=no protocol, 0xFE=functional unit, 0xFF=vendor specific)
        0x00,                                                           // iInterface

        0x07,                                                           // bLength (Enpoint2 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,           // bDescriptorType
        0x02,                                                           // bEndpointAddress
        0x02,                                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                         // wMaxPacketSize (high byte)
        0x40,                                                           // bInterval

        0x07,                                                           // bLength
        USB_ENDPOINT_DESCRIPTOR_TYPE,           // bDescriptorType
        0x82,                                                           // bEndpointAddress
        0x02,                                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                         // wMaxPacketSize (high byte)
        0x40                                                            // bInterval
/*
        0x09,                                                           // bLength (Interface2 descriptor)
        USB_INTERFACE_DESCRIPTOR_TYPE,          // bDescriptorType
        0x02,                                                           // bInterfaceNumber
        0x00,                                                           // bAlternateSetting
        0x00,                                                           // bNumEndpoints (None, only control pipe used)
        0xFE,                                                           // bInterfaceClass
        0x01,                                                           // bInterfaceSubClass
        0x01,                                                           // bInterfaceProtocol
        0x00,                                                           // iInterface (none)

        0x09,                                                           // bFunctionLength (DFU functional descriptor)
        0x21,                                                           // bDescriptorType
        0x03,                                                           // bmAttributes (!bitWillDetatch + !bitManifestationTolerant + bitCanUpload + bitCanDownload)
        0x80, 0x00,                                                     // wDetachTimeout (128 ms initial guess)
        0x20, 0x00,                                                     // wTransferSize (32 bytes)
        0x10, 0x01                                                      // bcdDFUVersion */
};

// TODO: Rework to ordinary datastructures.

unsigned char cdc_str_descs[] = {
        /* 0 */                  4, USB_STRING_DESCRIPTOR_TYPE, LOWB(USB_LANGID_English_United_States), HIGHB(USB_LANGID_English_United_States),
        /* USB_iManufacturer */ 42, USB_STRING_DESCRIPTOR_TYPE, 'D',0,'a',0,'n',0,'g',0,'e',0,'r',0,'o',0,'u',0,'s',0,' ',0,'P',0,'r',0,'o',0,'t',0,'o',0,'t',0,'y',0,'p',0,'e',0,'s',0,
        /* USB_iProduct */      18, USB_STRING_DESCRIPTOR_TYPE, 'C',0,'D',0,'C',0,' ',0,'T',0,'e',0,'s',0,'t',0,
        /* USB_iSerialNum */    18, USB_STRING_DESCRIPTOR_TYPE, '0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'1',0
};

/* TODO: Develop prebuild script for generating all complex structures */

#endif//__DESCRIPTORS_H__
