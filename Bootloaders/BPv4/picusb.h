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

Revision:
1 15th March 2011 // Please track your changes!
// JTR v0.1a
Comments by JTR.

This file is a combined header for all the PIC18F and PIC24F
USB PICs for the Honken USB stack. It has been corrected for
the PIC24 parts and slightly rearranged to reduce duplicate
entries common to both PIC families.


 */

#ifndef picusb_h
#define picusb_h

// JTR TODO Add support for the PIC24E and dsPIC33E parts when they are released.

#if defined(__18F2450) || defined(__18F2455) || defined(__18F2458) || defined(__18F2550) || defined(__18F2553)|| defined(__18F4450) || defined(__18F4455) || defined(__18F4458) || defined(__18F4550) || defined(__18F4553)
#define PIC_18F
#define PIC18Fxx5x
#define BD_RAM = 0x400

#elif defined(__18F13K50) || defined(__18F14K50)  //JTR add
#define PIC_18F
#define PIC18FxxK
#define BD_RAM  0x200

#elif defined(__18F24J50) || defined(__18F25J50) || defined(__18F26J50) ||  defined(__18F44J50) || defined(__18F45J50) || defined(__18F46J50) 
#define PIC_18F
#define IS_18J
#define PLLEN_REQD
#define BD_RAM = 0x400

#elif defined(__18F26J53) || defined(__18F27J53) || defined(__18F46J53)  || defined(__18F47J53)
#define PIC_18F
#define IS_18J
#define PLLEN_REQD
#define BD_RAM = 0xd00

#elif defined(__18F65J50) || defined(__18F66J50) || defined(__18F66J55) || defined(__18F67J50) || defined(__18F85J50) || defined(__18F86J50) || defined(__18F66J55) || defined(__18F67J50)
#define PIC_18F
#define IS_18J
#define PLLEN_REQD
#define USE_ALT_ANCON
#define BD_RAM = 0x400

#elif defined(__PIC24FJ64GB106__) || defined(__PIC24FJ128GB106__) || defined(__PIC24FJ192GB106__) || defined(__PIC24FJ256GB106__) || defined(__PIC24FJ64GB108__) || defined(__PIC24FJ128GB108__) || defined(__PIC24FJ192GB108__) || defined(__PIC24FJ256GB108__) || defined(__PIC24FJ64GB110__) || defined(__PIC24FJ128GB110__) || defined(__PIC24FJ192GB110__) || defined(__PIC24FJ256GB110__) 
#define PIC_24F

#elif defined(__PIC24FJ32GB002__) || defined(__PIC24FJ32GB004__) || defined(__PIC24FJ64GB002__) || defined(__PIC24FJ64GB004__)
#define PIC_24F 
#define PLLEN_REQD

#elif defined(__PIC24FJ128GB210__) || defined(__PIC24FJ128GB206__) || defined(PIC24FJ256GB206__) || defined(PIC24FJ256GB210__)
#define PIC_24F 
#define USE_ANS

#elif defined(__PIC24FJ128DA106__) || defined(__PIC24FJ128DA110__) || defined(__PIC24FJ128DA206__) || defined(__PIC24FJ128DA210__) || defined(__PIC24FJ256DA106__) || defined(__PIC24FJ256DA110__) ||  defined(_PIC24FJ256DA206__) || defined(__PIC24FJ256DA210__)
#define PIC_24F 
#define PLLEN_REQD
#define USE_ANS
#endif

#if defined(__18F13K50) || defined(__18F14K50)  //JTR add
#define MAX_CHIP_EP (8u)
#else
#define MAX_CHIP_EP (16u)
#endif

/* COMMON PIC DEFINES TO BOTH PIC18 AND PIC24 */

#define UOWN    0x80
#define DTS     0x40
#define KEN     0x20
#define INCDIS  0x10
#define DTSEN   0x08
#define BSTALL  0x04
#define BC98    0x03


#define USB_DIR_OUT     0
#define USB_DIR_IN      1
#define USB_PP_EVEN     0
#define USB_PP_ODD      1


/* PIC DEFINES SPECIFIC TO PIC18 */

#if defined(PIC_18F)

#define MyProcessor     // JTR check that a PIC is defined
#include <p18cxxx.h>

#define USTAT_ODD_EVEN (2)      // JTR may be required for ping-pong BD* calculations and are different from PIC18 to PIC24
#define USTAT_ODD_EVEN_SHIFT (1)

// JTR removed not used and the values??? 
// #define USB_EP_INTERRUPT             (0)
// #define USB_EP_BULK                  (1)
// #define USB_EP_ISOCHRONOUS           (2)

#define USB_UEP_EPSTALL         (0x01)  // JTR Note. to self. these values are different between the PIC18 and PIC24
#define USB_UEP_EPINEN          (0x02)
#define USB_UEP_EPOUTEN         (0x04)
#define USB_UEP_EPCONDIS        (0x08)
#define USB_UEP_EPHSHK          (0x10)

#define USB_EP_INOUT            (USB_UEP_EPHSHK | USB_UEP_EPINEN | USB_UEP_EPOUTEN | USB_UEP_EPCONDIS)
#define USB_EP_CONTROL          (USB_UEP_EPHSHK | USB_UEP_EPINEN | USB_UEP_EPOUTEN)
#define USB_EP_OUT              (USB_UEP_EPHSHK | USB_UEP_EPOUTEN | USB_UEP_EPCONDIS)
#define USB_EP_IN               (USB_UEP_EPHSHK | USB_UEP_EPINEN  | USB_UEP_EPCONDIS)
#define USB_EP_NONE             (0x00)

typedef unsigned char usb_uep_t;
#define USB_UEP                         ((usb_uep_t*) (&UEP0))
#define USB_UEP0                        UEP0
#define USB_UEP1                        UEP1
#define USB_UEP2                        UEP2
#define USB_UEP3                        UEP3
#define USB_UEP4                        UEP4
#define USB_UEP5                        UEP5
#define USB_UEP6                        UEP6
#define USB_UEP7                        UEP7

#ifndef PIC18FxxK
#define USB_UEP8                        UEP8
#define USB_UEP9                        UEP9
#define USB_UEP10                       UEP10
#define USB_UEP11                       UEP11
#define USB_UEP12                       UEP12
#define USB_UEP13                       UEP13
#define USB_UEP14                       UEP14
#define USB_UEP15                       UEP15
#endif

/* Interrupt */
#define USB_SOF                         0x40
#define USB_STALL                       0x20
#define USB_IDLE                        0x10
#define USB_TRN                         0x08
#define USB_ACTIV                       0x04
#define USB_UERR                        0x02
#define USB_URST                        0x01

#define USB_RESET_FLAG                          UIRbits.URSTIF
#define USB_ERROR_FLAG                          UIRbits.UERRIF
#define USB_ACTIVITY_FLAG                       UIRbits.ACTVIF
#define USB_IDLE_FLAG                           UIRbits.IDLEIF
#define USB_STALL_FLAG                          UIRbits.STALLIF
#define USB_SOF_FLAG                            UIRbits.SOFIF
#define USB_TRANSACTION_FLAG                    UIRbits.TRNIF

#define UsbInterruptFlags()                     (UIR)
#define UsbErrorInterruptFlags()                (UEIR)

#define TestGlobalUsbInterruptFlag()            PIR2bits.USBIF
#define TestUsbTrfInterruptFlag()               UIR (x)
#define ClearGlobalUsbInterruptFlag()           PIR2bits.USBIF = 0
#define ClearUsbInterruptFlag(x)                UIR &= ~(x)
#define ClearAllUsbInterruptFlags()             UIR = 0
#define ClearUsbErrorInterruptFlag(x)           UEIR &= ~(x)
#define ClearAllUsbErrorInterruptFlags()        UEIR = 0
#define DisableUsbInterrupts()                  PIE2bits.USBIE = 0
#define DisableUsbInterrupt(x)                  UIE &= ~(x)
#define DisableAllUsbInterrupts()               UIE = 0
#define DisableUsbErrorInterrupt(x)             UEIE &= ~(x)
#define DisableAllUsbErrorInterrupts()          UEIE = 0
#define EnableUsbInterrupts()                   PIE2bits.USBIE = 1
#define EnableUsbInterrupt(x)                   UIE |= (x)
#define EnableAllUsbInterrupts()                UIE = 0xFF
#define EnableUsbErrorInterrupt(x)              UEIE |= (x)
#define EnableAllUsbErrorInterrupts()           UEIE = 0xFF
#define EnableUSBHighInterrupts()               do { RCONbits.IPEN = 1; IPR2bits.USBIP = 1; INTCONbits.GIEH = 1;} while(0) // JTR new
#define EnableUSBLowInterrupts()                do { RCONbits.IPEN = 1; IPR2bits.USBIP = 0; INTCONbits.GIEL = 1;} while(0)  // JTR new
// JTR TODO define for NO priority interrupt.

/* UCON */
#define ResetPPbuffers()                        do {UCONbits.PPBRST = 1; UCONbits.PPBRST=0;} while(0)
#define SingleEndedZeroIsSet()                  (UCONbits.SE0)
#define EnablePacketTransfer()                  UCONbits.PKTDIS = 0
#define EnableUsb()                             UCONbits.USBEN = 1
#define SignalResume()                          do {UCONbits.RESUME = 1; delay_ms(10); UCONbits.RESUME = 0;} while(0)
#define SuspendUsb()                            UCONbits.SUSPND = 1
#define WakeupUsb()                             do {UCONbits.SUSPND = 0; while(USB_ACTIVITY_FLAG){USB_ACTIVITY_FLAG = 0;}} while(0)

/* UADDR */
#define SetUsbAddress(x)                        (UADDR = (x))
#define GetUsbAddress()                         (UADDR)

/* USTAT */
// typedef unsigned char usb_status_t; 
#define GetUsbTransaction()                     (USTAT)
#define USB_STAT2EP(x)                          ((x>>3)&0x0F)
#define USB_STAT2DIR(x)                         ((x>>2)&0x01)
#define USB_STAT2ADDR(x)                        ((x>>2)&0x1F)
#define USB_STAT2PPI(x)                         ((x>>1)&0x01)
#define DIRBIT 0x4      // JTR addition. Different between PIC18 and PIC24
/* Hardware implementations */

#if defined USB_INTERNAL_PULLUPS
#define USB_UCFG_UPUEN_VALUE (1<<4)
#elif defined USB_EXTERNAL_PULLUPS
#define USB_UCFG_UPUEN_VALUE (0)
#else
#error "Neither internal nor external pullups defined"
#endif

#if defined USB_INTERNAL_TRANSCIEVER
#define USB_UCFG_UTRDIS_VALUE (0)
#elif defined USB_EXTERNAL_TRANSCIEVER
#define USB_UCFG_UTRDIS_VALUE (1<<3)
#else
#error "Neither internal nor external transciever defined"
#endif

#if defined USB_FULL_SPEED_DEVICE
#define USB_UCFG_FSEN_VALUE (1<<2)
#elif defined USB_LOW_SPEED_DEVICE
#define USB_UCFG_FSEN_VALUE (0)
#else
#error "Neither internal nor external pullups defined"
#endif

#if defined USB_BUS_POWERED
#ifndef usb_low_power_request
/* Default low power mode is DUD */
#define usb_low_power_request() Nop()
#endif
#ifndef usb_low_power_resume
#define usb_low_power_resume() Nop()
#endif
#elif defined USB_SELF_POWERED
#define usb_low_power_request() Nop()
#define usb_low_power_resume() Nop()
#else
#error "No source of device power defined"
#endif

#ifndef USB_INTERNAL_VREG
#warning "Use of internal voltage regulator not defined. User must supply 3.3V on Vusb pin."
#endif

//#define USB_DIR_OUT     0
//#define USB_DIR_IN      1
//#define USB_PP_EVEN     0
//#define USB_PP_ODD      1

/* PingPong buffer descriptor table index calculations */
#if USB_PP_BUF_MODE == 0
#define USB_USTAT2BD(X)                         ( (X)/4 )
#define USB_CALC_BD(ep, dir, sync)              ( 2*(ep)+(dir) )
#elif USB_PP_BUF_MODE == 1
#error "USB_PP_BUF_MODE outside scope."
#define USB_USTAT2BD(X)                         ( ((X)>2)? (X)/4+1 : (X)/2 )
#define USB_CALC_BD(ep, dir, sync)              ( ((ep)==0 && (dir)==0)? (sync) : 2*(ep)+(dir) )
#elif USB_PP_BUF_MODE == 2              
#error "USB_PP_BUF_MODE outside scope."
#define USB_USTAT2BD(X)                         ( (X)/2 )
#define USB_CALC_BD(ep, dir, sync)              ( 4*(ep)+2*(dir)+(sync) )
#elif USB_PP_BUF_MODE == 3
#error "USB_PP_BUF_MODE outside scope."

#define USB_USTAT2BD(X)                         ( ((X)>4)? (X)/2-2 : (X)/4 )
#define USB_CALC_BD(ep, dir, sync)              ( ((ep)==0)? (dir) : 4*(ep)+2*(dir)+(sync)-2 )


#else
#error "USB_PP_BUF_MODE outside scope."
#endif

#define USB_UCFG_REGISTER_VALUE ((USB_UCFG_UPUEN_VALUE) | \
                                                                 (USB_UCFG_UTRDIS_VALUE) | \
                                                                 (USB_UCFG_FSEN_VALUE) | \
                                                                 (USB_PP_BUF_MODE))

#define ConfigureUsbHardware()                  do {UCFG = USB_UCFG_REGISTER_VALUE;} while(0)

#define ROM far rom
#define ROMPTR far rom
#define ARCH_memcpy memcpypgm2ram

typedef struct BDENTRY {
    unsigned char
    /*      struct {
                    unsigned BCH:2;
                    unsigned BSTALL:1;
                    unsigned DTSEN:1;
                    unsigned INCDIS:1;
                    unsigned KEN:1;
                    unsigned DTS:1;
                    unsigned UOWN:1;
            }*/
    BDSTAT;
    unsigned char BDCNT;
    unsigned char *BDADDR;
} BDentry;
#endif  // End of specific PIC18 defines.


/* PIC DEFINES SPECIFIC TO PIC24 FOLLOW */

#if defined(PIC_24F)

#define MyProcessor
#include <p24Fxxxx.h>

#define USTAT_ODD_EVEN (4)              // JTR PIC24 fixup potentially ?? Only required when ping-pong buffering is enabled. 
#define USTAT_ODD_EVEN_SHIFT (2)        // JTR these are required for BD* calculations and are different for the PIC24

/* Bitmasks */
#define USB_UEP_EPHSHK          (0x01)
#define USB_UEP_EPSTALL         (0x02)
#define USB_UEP_EPINEN          (0x04)
#define USB_UEP_EPOUTEN         (0x08)
#define USB_UEP_EPCONDIS        (0x10)

#define USB_EP_INOUT            (USB_UEP_EPHSHK | USB_UEP_EPINEN | USB_UEP_EPOUTEN | USB_UEP_EPCONDIS)
#define USB_EP_CONTROL          (USB_UEP_EPHSHK | USB_UEP_EPINEN | USB_UEP_EPOUTEN)
#define USB_EP_OUT              (USB_UEP_EPHSHK | USB_UEP_EPOUTEN | USB_UEP_EPCONDIS)
#define USB_EP_IN               (USB_UEP_EPHSHK | USB_UEP_EPINEN  | USB_UEP_EPCONDIS)
#define USB_EP_NONE             (0x00)

// JTR removed not used and the values??
// #define USB_EP_INTERRUPT     (0)
// #define USB_EP_BULK                  (1)
// #define      USB_EP_ISOCHRONOUS      (2)

typedef unsigned int usb_uep_t; // JTR PIC24 fixup potentially ?? changed from char.
// See http://dangerousprototypes.com/forum/viewtopic.php?f=39&t=1651&start=150#p17703

// However the stack as modified by JTR does not actually use it. Then again it might as it is tested and works.

#define USB_UEP                                 ((usb_uep_t*)(&U1EP0))
#define USB_UEP0                                U1EP0
#define USB_UEP1                                U1EP1
#define USB_UEP2                                U1EP2
#define USB_UEP3                                U1EP3
#define USB_UEP4                                U1EP4
#define USB_UEP5                                U1EP5
#define USB_UEP6                                U1EP6
#define USB_UEP7                                U1EP7
#define USB_UEP8                                U1EP8
#define USB_UEP9                                U1EP9
#define USB_UEP10                               U1EP10
#define USB_UEP11                               U1EP11
#define USB_UEP12                               U1EP12
#define USB_UEP13                               U1EP13
#define USB_UEP14                               U1EP14
#define USB_UEP15                               U1EP15

/* Interrupt */
#define USB_STALL                               0x0080
#define USB_ACTIV                               0x0020
#define USB_IDLE                                0x0010
#define USB_TRN                                 0x0008
#define USB_SOF                                 0x0004
#define USB_UERR                                0x0002
#define USB_URST                                0x0001

#define USB_RESET_FLAG                          U1IRbits.URSTIF
#define USB_ERROR_FLAG                          U1IRbits.UERRIF
#define USB_ACTIVITY_FLAG                       U1IRbits.RESUMEIF
#define USB_IDLE_FLAG                           U1IRbits.IDLEIF
#define USB_STALL_FLAG                          U1IRbits.STALLIF
#define USB_SOF_FLAG                            U1IRbits.SOFIF
#define USB_TRANSACTION_FLAG                    U1IRbits.TRNIF

#define UsbInterruptFlags()                     (U1IR)
#define UsbErrorInterruptFlags()                (U1EIR)
#define TestGlobalUsbInterruptFlag()            IFS5bits.USB1IF
#define ClearGlobalUsbInterruptFlag()           IFS5bits.USB1IF = 0
#define ClearUsbInterruptFlag(x)                U1IR = x
#define ClearAllUsbInterruptFlags()             U1IR = 0xFF
#define ClearUsbErrorInterruptFlag(x)           U1EIR = x
#define ClearAllUsbErrorInterruptFlags()        U1EIR = 0xFF
#define DisableUsbInterrupts()                  IEC5bits.USB1IE = 0
#define DisableUsbInterrupt(x)                  U1IE &= ~(x)
#define DisableAllUsbInterrupts()               U1IE = 0
#define DisableUsbErrorInterrupt(x)             U1EIE &= ~(x)
#define DisableAllUsbErrorInterrupts()          U1EIE = 0       
#define TestUsbInterruptEnabled()               IEC5bits.USB1IE

#ifdef USB_INTERRUPT
#define EnableUsbInterrupts()                   IEC5bits.USB1IE=1
#define EnableUsbInterrupt(x)                   U1IE |= (x)
#define EnableAllUsbInterrupts()                U1IE = 0x00FF
#define EnableUsbErrorInterrupt(x)              U1EIE |= (x)
#define EnableAllUsbErrorInterrupts()           U1EIE = 0x00FF
#define EnableUSBHighInterrupts()
#else
#define EnableUsbInterrupts()
#define EnableUsbInterrupt(x)
#define EnableAllUsbInterrupts()            
#define EnableUsbErrorInterrupt(x)
#define EnableAllUsbErrorInterrupts()
#define EnableUSBHighInterrupts()
#endif

/* UCON */
#define ResetPPbuffers()                        do {U1CONbits.PPBRST = 1; U1CONbits.PPBRST=0;} while(0)
#define SingleEndedZeroIsSet()                  (U1CONbits.SE0)
#define EnablePacketTransfer()                  U1CONbits.PKTDIS = 0
#define EnableUsb()                             while(!U1CONbits.USBEN){U1CONbits.USBEN = 1;} //                U1CONbits.USBEN = 1
#define SignalResume()                          do {U1CONbits.RESUME = 1; delay_ms(10); U1CONbits.RESUME = 0;} while(0)
#define SuspendUsb()                            U1PWRCbits.USUSPND = 1
#define WakeupUsb()                             do {U1PWRCbits.USUSPND = 0; while(USB_ACTIVITY_FLAG){USB_ACTIVITY_FLAG = 0;}} while(0)


/* UADDR */
#define SetUsbAddress(x)                        (U1ADDR = (x))
#define GetUsbAddress()                         (U1ADDR)

#define GetUsbTransaction()                     (U1STAT)
#define USB_STAT2EP(x)                          ((x>>4)&0x0F) //((x>>3)&0x0F)   JTR PIC24 fixups
#define USB_STAT2DIR(x)                         ((x>>3)&0x01) //((x>>2)&0x01)   ""
#define USB_STAT2ADDR(x)                        ((x>>3)&0x1F) //((x>>2)&0x1F)   ""
#define USB_STAT2PPI(x)                         ((x>>2 &0x01) //((x>>1)&0x01)   ""
#define DIRBIT 0x8      // JTR add PIC24 fixup as the mask is 0x4 for the PIC18


/* Hardware implementations */

#if defined USB_INTERNAL_PULLUPS
#elif defined USB_EXTERNAL_PULLUPS
#define USB_U1OTGCON_UPUEN_VALUE (0)
#else
#error "Neither internal nor external pullups defined"
#endif

#if defined USB_INTERNAL_TRANSCIEVER
#define USB_U1CNFG2_UTRDIS_VALUE (0)
#elif defined USB_EXTERNAL_TRANSCIEVER
#define USB_U1CNFG2_UTRDIS_VALUE (1)
#else
#error "Neither internal nor external transciever defined"
#endif

#if defined USB_FULL_SPEED_DEVICE
#if defined USB_INTERNAL_PULLUPS
#define USB_U1OTGCON_DPPULUP_VALUE (1<<7)
#define USB_U1OTGCON_DMPULUP_VALUE (0)
#else
#define USB_U1OTGCON_DPPULUP_VALUE (0)
#define USB_U1OTGCON_DMPULUP_VALUE (0)
#endif
#elif defined USB_LOW_SPEED_DEVICE
#if defined USB_INTERNAL_PULLUPS
#define USB_U1OTGCON_DPPULUP_VALUE (0)
#define USB_U1OTGCON_DMPULUP_VALUE (1<<6)
#else
#define USB_U1OTGCON_DPPULUP_VALUE (0)
#define USB_U1OTGCON_DMPULUP_VALUE (0)
#endif
#else
#error "Neither internal nor external pullups defined"
#endif

#if defined USB_BUS_POWERED
#ifndef usb_low_power_request
/* Default low power mode is DUD */
#define usb_low_power_request() Nop()
#endif
#ifndef usb_low_power_resume
#define usb_low_power_resume() Nop()
#endif
#elif defined USB_SELF_POWERED
#define usb_low_power_request() Nop()
#define usb_low_power_resume() Nop()
#else
#error "No source of device power defined"
#endif

#ifndef USB_INTERNAL_VREG
#warning "Use of internal voltage regulator not implemented. User must supply 3.3V on Vusb pin."
#endif

/* PingPong buffer descriptor table index calculations */
#if USB_PP_BUF_MODE == 0
#define USB_USTAT2BD(X)                         ( (X)/8 )  //JTR PIC24 fixups
#define USB_CALC_BD(ep, dir, sync)              ( 2*(ep)+(dir) )

// JTR TODO these values may need to be changed for the PIC24
//#elif USB_PP_BUF_MODE == 1
//#define USB_USTAT2BD(X)                       ( ((X)>2)? (X)/4+1 : (X)/2 )
//#define USB_CALC_BD(ep, dir, sync)            ( ((ep)==0 && (dir)==0)? (sync) : 2*(ep)+(dir) )
//#elif USB_PP_BUF_MODE == 2              
//#define USB_USTAT2BD(X)                       ( (X)/2 )
//#define USB_CALC_BD(ep, dir, sync)            ( 4*(ep)+2*(dir)+(sync) )
//#elif USB_PP_BUF_MODE == 3
//#define USB_USTAT2BD(X)                       ( ((X)>4)? (X)/2-2 : (X)/4 )
//#define USB_CALC_BD(ep, dir, sync)            ( ((ep)==0)? (dir) : 4*(ep)+2*(dir)+(sync)-2 )

#else
#error "USB_PP_BUF_MODE outside scope."
#endif

// JTR PIC24 fixups some may not required but are "more correct." The added /256 is required!
#define ConfigureUsbHardware()          do { \
                                                U1CNFG1 = USB_PP_BUF_MODE; \
                                                U1CNFG2 = USB_U1CNFG2_UTRDIS_VALUE; \
                                                U1BDTP1 = (unsigned int) usb_bdt/256; \
                                                U1OTGCON = USB_U1OTGCON_DPPULUP_VALUE | \
                                                USB_U1OTGCON_DMPULUP_VALUE; \
                                                U1PWRCbits.USBPWR = 1; \
                                        } while(0)

#define ROM __attribute__((space(auto_psv)))
#define ROMPTR
#define ARCH_memcpy memcpy

typedef struct BDENTRY {
    unsigned char BDCNT; // JTR PIC24 fixup Note that the endianness is swapped from the PIC18
                         // This is probably what had people hitting their heads!
    unsigned char BDSTAT;
    unsigned char *BDADDR;
} BDentry;

#endif

#ifndef MyProcessor
#error "No PIC family defined"
#endif  

#endif
