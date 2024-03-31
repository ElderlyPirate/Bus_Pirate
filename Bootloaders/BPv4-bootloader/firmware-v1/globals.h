#ifndef GLOBALS_H
#define GLOBALS_H

// =================== GLOBAL TYPEDEFS
typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;
typedef unsigned char BYTE;

#define INVALID 0xFF

#define FALSE   0
#define TRUE    (!FALSE)

// =================== GLOBAL HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boot_config.h"
#include "usb_stack.h"
#include "cdc.h"

#define LedSetup() TRISBbits.TRISB8=0;TRISBbits.TRISB9=0;TRISBbits.TRISB10=0

#define uLedOn() LATBbits.LATB10 = 1
#define uLedOff() LATBbits.LATB10 = 0
#define uLedToggle() LATBbits.LATB10 ^= LATBbits.LATB10

#define mLedOn() LATBbits.LATB8 = 1
#define mLedOff() LATBbits.LATB8 = 0
#define mLedToggle() LATBbits.LATB8 ^= LATBbits.LATB8

#define vLedOn() LATBbits.LATB9 = 1
#define vLedOff() LATBbits.LATB9 = 0
#define vLedToggle() LATBbits.LATB9 ^= LATBbits.LATB9
#endif
