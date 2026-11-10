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

// JTR v0.2a

/* String identifiers */
#define USB_iManufacturer               1u
#define USB_iProduct                    2u
#define USB_iSerialNum                  3u
#define USB_NUM_STRINGS                 4u

ROMPTR const unsigned char cdc_device_descriptor[] = {
        0x12,                                           // bLength
        USB_DEVICE_DESCRIPTOR_TYPE,                     // bDescriptorType
        0x00,                                           // bcdUSB (low byte)
        0x02,                                           // bcdUSB (high byte)
        0x02,                                           // bDeviceClass
        0x00,                                           // bDeviceSubClass
        0x00,                                           // bDeviceProtocol
        USB_EP0_BUFFER_SIZE,                            // bMaxPacketSize

        LOWB(USB_VID),                                  // idVendor (low byte)
        HIGHB(USB_VID),                                 // idVendor (high byte)
        LOWB(USB_PID),                                  // idProduct (low byte)
        HIGHB(USB_PID),                                 // idProduct (high byte)
        LOWB(USB_DEV),                                  // bcdDevice (low byte)
        HIGHB(USB_DEV),                                 // bcdDevice (high byte)
        USB_iManufacturer,                              // iManufacturer
        USB_iProduct,                                   // iProduct
        USB_iSerialNum,                                 // iSerialNumber (none)
        USB_NUM_CONFIGURATIONS                          // bNumConfigurations 
};

#define USB_CONFIG_DESC_TOT_LENGTH (9+9+5+4+5+5+7+9+7+7)
ROMPTR const unsigned char cdc_config_descriptor[] = {
        0x09,                                           // bLength
        USB_CONFIGURATION_DESCRIPTOR_TYPE,              // bDescriptorType
        LOWB(USB_CONFIG_DESC_TOT_LENGTH),               // wTotalLength (low byte)
        HIGHB(USB_CONFIG_DESC_TOT_LENGTH),              // wTotalLength (high byte)
        USB_NUM_INTERFACES,                             // bNumInterfaces
        0x01,                                           // bConfigurationValue
        0x00,                                           // iConfiguration (0=none)
        0x80,                                           // bmAttributes (0x80 = bus powered)
        0x64,                                           // bMaxPower (in 2 mA units, 50=100 mA)
              //Interface0 descriptor starts here
        0x09,                                           // bLength (Interface0 descriptor starts here)
        USB_INTERFACE_DESCRIPTOR_TYPE,                  // bDescriptorType
        0x00,                                           // bInterfaceNumber
        0x00,                                           // bAlternateSetting
        0x01,                                           // bNumEndpoints (excluding EP0)  
        0x02,                                           // bInterfaceClass 0x02=com interface 
        0x02,                                           // bInterfaceSubClass 0x02=ACM 
        0x01,                                           // bInterfaceProtocol 0x01=AT modem
        0x00,                                           // iInterface (none)
              // CDC header descriptor
        0x05,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x00,                                           // bDescriptorSubtype (CDC header descriptor)
        0x10,                                           // bcdCDC (low byte)
        0x01,                                           // bcdCDC (high byte)
              // CDC abstract control management descriptor 
        0x04,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x02,                                           // bDescriptorSubtype (CDC abstract control management descriptor)
        0x02,                                           // bmCapabilities
              // CDC union descriptor
        0x05,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x06,                                           // bDescriptorSubtype (CDC union descriptor)
        0x00,                                           // bControlInterface
        0x01,                                           // bSubordinateInterface0
              // Call management descriptor
        0x05,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x01,                                           // bDescriptorSubtype (Call management descriptor)
        0x01,                                           // bmCapabilities
        0x01,                                           // bInterfaceNum
             // CDC Endpoint 1 IN descriptor (INTERRUPT, Not used)
        0x07,                                           // bLength (Endpoint1 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,                   // bDescriptorType
        0x81,                                           // bEndpointAddress
        0x03,                                           // bmAttributes (0x03=intr)
        LOWB(CDC_NOTICE_BUFFER_SIZE),                   // wMaxPacketSize (low byte)
        HIGHB(CDC_NOTICE_BUFFER_SIZE),                  // wMaxPacketSize (high byte)
        0x40,                                           // bInterval
                //Interface1 descriptor
        0x09,                                           // bLength (Interface1 descriptor)
        USB_INTERFACE_DESCRIPTOR_TYPE,                  // bDescriptorType
        0x01,                                           // bInterfaceNumber
        0x00,                                           // bAlternateSetting
        0x02,                                           // bNumEndpoints
        0x0A,                                           // datainterface type                                            
        0x00,                                           // bInterfaceSubClass
        0x00,                                           // bInterfaceProtocol (0x00=no protocol, 0xFE=functional unit, 0xFF=vendor specific)
        0x00,                                           // iInterface
             // CDC Endpoint 2 OUT descriptor (BULK)
        0x07,                                           // bLength (Enpoint2 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,                   // bDescriptorType
        0x02,                                           // bEndpointAddress
        0x02,                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                         // wMaxPacketSize (high byte)
        0x00,                                           // bInterval
             // CDC Endpoint 2 IN descriptor (BULK)
        0x07,                                           // bLength
        USB_ENDPOINT_DESCRIPTOR_TYPE,                   // bDescriptorType
        0x82,                                           // bEndpointAddress
        0x02,                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                         // wMaxPacketSize (high byte)
        0x00                                            // bInterval
};

ROMPTR const unsigned char cdc_str_descs[] = {
         /* 0 */                  4, USB_STRING_DESCRIPTOR_TYPE, LOWB(USB_LANGID_English_United_States), HIGHB(USB_LANGID_English_United_States),
         /* USB_iManufacturer */ 42, USB_STRING_DESCRIPTOR_TYPE, 'D',0,'a',0,'n',0,'g',0,'e',0,'r',0,'o',0,'u',0,'s',0,' ',0,'P',0,'r',0,'o',0,'t',0,'o',0,'t',0,'y',0,'p',0,'e',0,'s',0,
         /* USB_iProduct */      30, USB_STRING_DESCRIPTOR_TYPE, 'B',0,'u',0,'s',0,' ',0,'P',0,'i',0,'r',0,'a',0,'t',0,'e',0,' ',0,'V',0,'4',0,' ',0,
         /* USB_iSerialNum */    18, USB_STRING_DESCRIPTOR_TYPE, '0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'1',0
};

#endif//__DESCRIPTORS_H__
