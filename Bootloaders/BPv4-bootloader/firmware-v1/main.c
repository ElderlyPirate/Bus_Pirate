/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has
 * waived all copyright and related or neighboring rights to Bus Pirate. This
 * work is published from United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#pragma config WPFP = WPFP511
#pragma config WPDIS = WPDIS
#pragma config WPCFG = WPCFGDIS
#pragma config WPEND = WPENDMEM
#pragma config POSCMOD = HS
#pragma config DISUVREG = OFF
#pragma config IOL1WAY = ON
#pragma config OSCIOFNC = ON
#pragma config FCKSM = CSDCMD
#pragma config FNOSC = PRIPLL
#pragma config PLL_96MHZ = ON
#pragma config PLLDIV = DIV3
#pragma config IESO = OFF
#pragma config WDTPS = PS32768
#pragma config FWPSA = PR128
#pragma config WINDIS = OFF
#pragma config FWDTEN = OFF
#pragma config ICS = PGx2
#pragma config GWRP = OFF
#pragma config GCP = OFF
#pragma config JTAGEN = OFF

#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

// Hardware 'NORMAL' button on BPv4 definitions
#define BP_BUTTON_DIR TRISCbits.TRISC14
#define BP_BUTTON PORTCbits.RC14
#define BP_BUTTON_IF IFS1bits.CNIF
#define BP_BUTTON_SETUP()                                                      \
  BP_BUTTON_DIR = 1;                                                           \
  CNPU1 |= 0b1;                                                                \
  CNEN1 |= 0b1;                                                                \
  IEC1bits.CNIE = 0;                                                           \
  BP_BUTTON_IF = 0;
#define BP_BUTTON_ISDOWN() (!BP_BUTTON) // 0=DOWN (PRESSED) / 1=UP (UNPRESSED)

#define FIRMWARE_SIGNATURE 0x31415926

bool __attribute__((address(0x47FA), persistent)) skip_pgc_pgd_check;
uint32_t __attribute__((address(0x47FC), persistent)) firmware_signature;

// BOOTLOADER PROJECT MAIN FILE

#include "bootloader.h"
#include "descriptors.h" // JTR Only included in main.c
#include "globals.h"

void usb_start(void);
void initCDC(void);
void usb_init(ROMPTR const unsigned char *, ROMPTR const unsigned char *,
              ROMPTR const unsigned char *, int);
void Initialize(void);
void bootloader(void);
extern volatile unsigned char usb_device_state;
void startpoint(void);

#pragma code

#define PGC_OUT LATBbits.LATB6
#define PGC_TRIS TRISBbits.TRISB6
#define PGC_PU CNPU2bits.CN24PUE
#define PGC_IN PORTBbits.RB6

#define PGD_IN PORTBbits.RB7
#define PGD_TRIS TRISBbits.TRISB7
#define PGD_PU CNPU2bits.CN25PUE

int main(void) {
  AD1PCFGL = 0xFFFF; // all digital

  // PGD is input with pullup
  // TRISB|=0b10000000; //
  PGD_TRIS = 1; // input
  // CNPU2|=0b1000000000; //
  PGD_PU = 1; // pullup on

  PGC_OUT = 0;
  PGC_TRIS = 0;

  LedSetup();

  BP_BUTTON_SETUP();

  if ((firmware_signature != FIRMWARE_SIGNATURE) || !skip_pgc_pgd_check || BP_BUTTON_ISDOWN()) {
    volatile int i;

    skip_pgc_pgd_check = false;

    int j = 35;
    while (j--) {
        i = 65000;
        while (i--);
    }

    vLedOn();
    mLedOn();
    uLedOn();

    for (i = 0; i < 20; i++) {
      if ((PGD_IN == 1 && !(BP_BUTTON_ISDOWN()))) { // go to user space on first mis-match
        // continue to bootloader, or exit
        asm(".equ BLJUMPADDRESS, 0x2000");
        asm volatile("mov #BLJUMPADDRESS, w1 \n" // bootloader location
                     "goto w1 \n");
      }
    }
  }

  skip_pgc_pgd_check = false;

  PGD_PU = 0;   // pullup off
  PGC_TRIS = 1; // input
  startpoint();
  return 0;
}

void startpoint() {
  Initialize(); // setup bus pirate
  LedSetup();
  uLedOff();
  mLedOff();
  vLedOn();

  usb_init(cdc_device_descriptor, cdc_config_descriptor, cdc_str_descs,
           USB_NUM_STRINGS); // TODO: Remove magic with macro
  usb_start();
  initCDC(); // Setup CDC defaults.
  vLedOff();
  // wait for the USB connection to enumerate
  do {
    // if (!TestUsbInterruptEnabled()) //JTR3 added

    usb_handler(); ////service USB tasks Guaranteed one pass in polling mode
                   ///even when usb_device_state == CONFIGURED_STATE
                   // if ((usb_device_state < DEFAULT_STATE)) { // JTR2 no suspendControl
    // available yet || (USBSuspendControl==1) ){
    // } else if (usb_device_state < CONFIGURED_STATE) {
    // }
  } while (usb_device_state < CONFIGURED_STATE); // JTR addition. Do not proceed
                                                 // until device is configured.
  vLedOn();
  bootloader();
}

void Initialize(void) {
  CORCONbits.PSV = 1;
  PSVPAG = 0;      //
  CLKDIV = 0x0000; // Set PLL prescaler (1:1)
  TBLPAG = 0;

  // all high-z to protect everything
  AD1PCFGL = 0xFFFF; // all digital
  AD1PCFGH = 0x2;
  // TRISA=0xFFFF;
  TRISB = 0xFFFF;
  TRISC = 0xFFFF;
  TRISD = 0xFFFF;
  TRISE = 0xFFFF;
  TRISF = 0xFFFF;
  TRISG = 0xFFFF;
  OSCCONbits.SOSCEN = 0;
}

#ifdef USB_INTERRUPT
#pragma interrupt _USB1Interrupt

void __attribute__((interrupt, no_auto_psv)) _USB1Interrupt() {

  // USB interrupt
  // IRQ enable IEC5bits.USB1IE
  // IRQ flag	IFS5bits.USB1IF
  // IRQ priority IPC21<10:8>
  //{
  usb_handler();
  IFS5bits.USB1IF = 0; //	PIR2bits.USBIF = 0;
                       //}
}
#endif
