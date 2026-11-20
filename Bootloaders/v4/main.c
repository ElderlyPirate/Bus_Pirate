/* Bus Pirate project, CC0. Board entry and reset contract. */
#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "bootloader.h"
#include "usb_stack.h"
#include "cdc.h"

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

#define FIRMWARE_SIGNATURE 0x31415926UL
bool __attribute__((address(0x47FA), persistent)) skip_pgc_pgd_check;
uint32_t __attribute__((address(0x47FC), persistent)) firmware_signature;

static void delay_ms(uint16_t count) {
    /* FCY = 16 MHz, Timer1 /64: 250 ticks/ms. */
    while (count--) {
        uint16_t start = TMR1;
        while ((uint16_t)(TMR1 - start) < 250u) {}
    }
}

int main(void) {
    uint8_t requested = firmware_signature == FIRMWARE_SIGNATURE && skip_pgc_pgd_check;
    uint16_t restart_time = 0;
    skip_pgc_pgd_check = false;
    CLKDIV = 0;
    OSCCONbits.SOSCEN = 0;
    AD1PCFGL = 0xFFFF;
    AD1PCFGH = 2;
    /* RB9 is the target supply enable, NOT an LED. Never turn it on. */
    LATBbits.LATB9 = 0;
    TRISBbits.TRISB9 = 0;
    LATBbits.LATB8 = LATBbits.LATB10 = 0;
    TRISBbits.TRISB8 = TRISBbits.TRISB10 = 0;
    TMR1 = 0;
    PR1 = 0xFFFF;
    T1CON = 0x8020; /* TON=1, TCKPS=10 (1:64), TCS=0. */
    TRISCbits.TRISC14 = 1;
    CNPU1bits.CN0PUE = 1;
    LATBbits.LATB6 = 0;
    TRISBbits.TRISB6 = 0;
    TRISBbits.TRISB7 = 1;
    CNPU2bits.CN25PUE = 1;
    delay_ms(100); /* Button/jumper inputs settle; not an entry timeout. */
    TBLPAG = 0;
    if (!requested && PORTCbits.RC14 && PORTBbits.RB7 &&
        !(__builtin_tblrdl(APPLICATION_ADDRESS) == 0xFFFF &&
          (__builtin_tblrdh(APPLICATION_ADDRESS) & 0xFF) == 0xFF)) {
        CNPU2bits.CN25PUE = 0;
        CNPU1bits.CN0PUE = 0;
        TRISBbits.TRISB6 = 1;
        TRISBbits.TRISB8 = TRISBbits.TRISB10 = TRISBbits.TRISB9 = 1;
        T1CON = 0;
        /* Table address == PC address (DS39897C 4.3.2), not twice it. */
        __asm__ volatile("goto 0x2000");
    }
    CNPU2bits.CN25PUE = 0;
    TRISBbits.TRISB6 = 1;
    usb_init();
    boot_init();
    for (;;) {
        usb_handler();
        boot_poll();
        LATBbits.LATB10 = usb_configured();
        if (boot_restart) {
            if (!restart_time) restart_time = usb_millis();
            if (cdc_tx_ready() || (uint16_t)(usb_millis() - restart_time) >= 250u) {
                U1CONbits.USBEN = 0;
                delay_ms(100); /* Make the detach visible before software reset. */
                __asm__ volatile("reset");
            }
        } else restart_time = 0;
    }
}

void __attribute__((interrupt, no_auto_psv)) _DefaultInterrupt(void) {
    __asm__ volatile("reset");
}
