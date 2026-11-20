/* Honken/JTR-derived USB device support, CC BY 3.0. */
#ifndef USB_STACK_H
#define USB_STACK_H
#include "picusb.h"
#include "boot_config.h"
#define USB_DEVICE_DESCRIPTOR_TYPE 1u
#define USB_CONFIGURATION_DESCRIPTOR_TYPE 2u
#define USB_STRING_DESCRIPTOR_TYPE 3u
#define USB_INTERFACE_DESCRIPTOR_TYPE 4u
#define USB_ENDPOINT_DESCRIPTOR_TYPE 5u
#define LOWB(x) ((x) & 0xFFu)
#define HIGHB(x) ((x) >> 8)
extern BDentry usb_bdt[6];
void usb_init(void);
void usb_handler(void);
uint8_t usb_configured(void);
uint16_t usb_millis(void);
uint16_t usb_word(const uint8_t *p);
uint8_t usb_halted(uint8_t bd);
void usb_reply(const uint8_t *data, uint16_t length, uint16_t requested);
void usb_expect_line_coding(void);
#endif
