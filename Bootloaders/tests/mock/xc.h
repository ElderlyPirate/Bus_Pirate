/* Native-test SFR stand-ins. This is not a hardware/USB bus simulator. */
#ifndef TEST_XC_H
#define TEST_XC_H
#include <stdint.h>
static volatile unsigned int test_ep[16];
#define U1EP0 test_ep[0]
#define U1EP1 test_ep[1]
#define U1EP2 test_ep[2]
static volatile unsigned int U1CON, U1OTGCON, U1PWRC, U1IE, U1EIE, U1OTGIE;
static volatile unsigned int U1IR, U1EIR, U1OTGIR, U1CNFG1, U1CNFG2, U1BDTP1;
static volatile unsigned int U1ADDR, U1STAT, TMR1;
static struct { unsigned USB1IE; } IEC5bits;
static struct { unsigned PPBRST, PKTDIS, USBEN; } U1CONbits;
static struct { unsigned USBPWR; } U1PWRCbits;
static struct { unsigned URSTIF, TRNIF, UERRIF; } U1IRbits;
#endif
