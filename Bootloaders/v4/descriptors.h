/* Derived from Honken/JTR descriptors, CC BY 3.0; see COPYING-USB.txt. */
#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H
#include "usb_stack.h"

static const uint8_t cdc_device_descriptor[] = {
    18, 1, 0x10, 0x01, 2, 0, 0, USB_EP0_BUFFER_SIZE,
    LOWB(USB_VID), HIGHB(USB_VID), LOWB(USB_PID), HIGHB(USB_PID),
    LOWB(USB_DEV), HIGHB(USB_DEV), 1, 2, 3, 1
};

static const uint8_t cdc_config_descriptor[] = {
    9, 2, 67, 0, 2, 1, 0, 0x80, 100, /* Bus powered; maximum 200 mA. */
    9, 4, 0, 0, 1, 2, 2, 1, 0,     /* ACM control interface. */
    5, 0x24, 0, 0x10, 0x01,         /* CDC 1.10 header. */
    4, 0x24, 2, 2,                  /* Line coding/control-line capabilities. */
    5, 0x24, 6, 0, 1,               /* Union: control 0, subordinate 1. */
    5, 0x24, 1, 0, 1,               /* No call management. */
    7, 5, 0x81, 3, 10, 0, 64,       /* EP1 IN: notification, ten bytes. */
    9, 4, 1, 0, 2, 0x0A, 0, 0, 0,  /* Data interface. */
    7, 5, 0x02, 2, 64, 0, 0,        /* EP2 OUT: bulk. */
    7, 5, 0x82, 2, 64, 0, 0         /* EP2 IN: bulk. */
};

static const uint8_t cdc_str_descs[] = {
    4, 3, 9, 4, /* English (United States), LANGID 0x0409. */
    42, 3, 'D',0,'a',0,'n',0,'g',0,'e',0,'r',0,'o',0,'u',0,'s',0,' ',0,
           'P',0,'r',0,'o',0,'t',0,'o',0,'t',0,'y',0,'p',0,'e',0,'s',0,
    50, 3, 'B',0,'u',0,'s',0,' ',0,'P',0,'i',0,'r',0,'a',0,'t',0,'e',0,
           ' ',0,'v',0,'4',0,' ',0,'B',0,'o',0,'o',0,'t',0,'l',0,'o',0,
           'a',0,'d',0,'e',0,'r',0,
    18, 3, '0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'1',0
};
#endif
