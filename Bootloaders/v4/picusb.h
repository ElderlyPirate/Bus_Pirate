/* PIC24F USB definitions derived from Honken/JTR, CC BY 3.0. */
#ifndef PICUSB_H
#define PICUSB_H
#include <xc.h>
#include <stdint.h>
#define UOWN 0x80u
#define DTS 0x40u
#define DTSEN 0x08u
#define BSTALL 0x04u
#define USB_EP_CONTROL 0x0Du
#define USB_EP_IN 0x15u
#define USB_EP_INOUT 0x1Du
#define USB_UEP ((volatile unsigned int *)&U1EP0)
#define USB_UEP1 U1EP1
#define USB_UEP2 U1EP2
/* DS39897C 18.2: byte count then flags then a 16-bit data RAM address. */
typedef struct {
    volatile uint8_t BDCNT;
    volatile uint8_t BDSTAT;
    uint8_t * volatile BDADDR;
} BDentry;
#endif
