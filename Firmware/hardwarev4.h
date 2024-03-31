/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has waived all copyright and
 * related or neighboring rights to Bus Pirate. This work is published from
 * United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef BP_HARDWAREV4_H
#define BP_HARDWAREV4_H

#include "types.h"

#define BP_VERSION_STRING "Bus Pirate v4"
#define BP_VERSION "v4"

// If you want to route AUX1 to channel 7 of the JTR SUMP mode in firmware
// then uncomment this line. Otherwise you can "GREEN WIRE" pins 42 to 55
// on the PIC24FJ256GB106 or just have 6 channels that do not include AUX1

#define BPv4_SUMP_SOFT_WIRE
#define NUM_OF_SUMP_CHANNELS 8

// this configures the placement of the serial port for
// the PC side terminal interface
// the other modules should be similarly abstracted, but since v0A and v1a share
// pins, it doesn't mater
#define BP_TERM_TX U1TX_IO;
#define BP_TERM_TX_RP RPOR1bits.RP2R
#define BP_TERM_RX RPINR18bits.U1RXR
#define BP_TERM_RX_RP 11

// this is a different type of pin identifier used by the bitbang library
#define AUX2 0b0000000000000001        // B0
#define MOSI 0b0000000000000010        // B1
#define CLK 0b0000000000000100         // B2
#define MISO 0b0000000000001000        // B3
#define CS 0b0000000000010000          // B4
#define AUX0 0b0000000000100000        // B5
#define SUMP_SPARE6 0b0000000001000000 // B6
#define SUMP_SPARE7 0b0000000010000000 // B7
#define AUX1 0b0000000100000000        // B8

#define IOPOR PORTD
#define IOLAT LATD
#define IODIR TRISD

#define AUX AUX0

// these macros set the pin IO used in each module
#define BP_MOSI PORTDbits.RD1
#define BP_CLK PORTDbits.RD2
#define BP_MISO PORTDbits.RD3
#define BP_CS PORTDbits.RD4
#define BP_AUX0 PORTDbits.RD5    // moved pin5 (AUX) to RB10
#define BP_LEDMODE PORTBbits.RB8 // single MODE led on v2a
#define BP_VREGEN PORTBbits.RB9  // single vreg enable pin on v2a
#define BP_PULLUP PORTEbits.RE4  // Pull-up pin on V2a
#define BP_PGD PORTBbits.RB7     // PGD pin on programming header

// new in v4
#define BP_LEDUSB PORTBbits.RB10
#define BP_BUTTON PORTCbits.RC14
#define BP_AUX1 PORTDbits.RD8
#define BP_AUX2 PORTDbits.RD0
#define BP_EE_SDA PORTDbits.RD9
#define BP_EE_SCL PORTDbits.RD10
#define BP_EE_WP PORTDbits.RD11
#define BP_PUVSEL33 PORTFbits.RF4
#define BP_PUVSEL50 PORTFbits.RF5

// these macros set the direction registers for each IO pin
#define BP_MOSI_DIR TRISDbits.TRISD1
#define BP_CLK_DIR TRISDbits.TRISD2
#define BP_MISO_DIR TRISDbits.TRISD3
#define BP_CS_DIR TRISDbits.TRISD4
#define BP_AUX0_DIR TRISDbits.TRISD5
#define BP_LEDMODE_DIR TRISBbits.TRISB8
#define BP_VREGEN_DIR TRISBbits.TRISB9
#define BP_PULLUP_DIR TRISEbits.TRISE4
#define BP_PGD_DIR TRISBbits.TRISB7

// new in v4
#define BP_LEDUSB_DIR TRISBbits.TRISB10
#define BP_BUTTON_DIR TRISCbits.TRISC14
#define BP_AUX1_DIR TRISDbits.TRISD8
#define BP_AUX2_DIR TRISDbits.TRISD0
#define BP_EE_SDA_DIR TRISDbits.TRISD9
#define BP_EE_SCL_DIR TRISDbits.TRISD10
#define BP_EE_WP_DIR TRISDbits.TRISD11
#define BP_PUVSEL33_DIR TRISFbits.TRISF4
#define BP_PUVSEL50_DIR TRISFbits.TRISF5

// secondary MISO/CLK/MOSI connection setup (input, low)
#define BPV4_HWI2CPINS_SETUP()                                                 \
  TRISE |= 0b11100000;                                                         \
  LATE &= (~0b11100000)

// Hardware 'NORMAL' button on BPv4 definitions
#define BP_BUTTON_IF IFS1bits.CNIF
#define BP_BUTTON_SETUP()                                                      \
  BP_BUTTON_DIR = 1;                                                           \
  CNPU1 |= 0b1;                                                                \
  CNEN1 |= 0b1;                                                                \
  IEC1bits.CNIE = 0;                                                           \
  BP_BUTTON_IF = 0;
#define BP_BUTTON_ISDOWN() (!BP_BUTTON) // 0=DOWN (PRESSED) / 1=UP (UNPRESSED)

// Open drain/high impedance pin setup
#define BP_MOSI_ODC ODCDbits.ODD1
#define BP_CLK_ODC ODCDbits.ODD2
#define BP_MISO_ODC ODCDbits.ODD3
#define BP_CS_ODC ODCDbits.ODD4
#define BP_AUX_ODC ODCDbits.ODD5

// Change notice assignment
#define BP_MOSI_CN CNEN4bits.CN50IE
#define BP_CLK_CN CNEN4bits.CN51IE

// ADC assignment
#define BP_ADC_PROBE 1
#define BP_ADC_3V3 0
#define BP_ADC_5V0 2
#define BP_ADC_VPU 5
#define BP_ADC_USB 15

// ADC pin configuration
#define BP_ADC_PROBE_CFG AD1PCFGbits.PCFG2 // AN1/ADC1/EXT
#define BP_ADC_3V3_CFG AD1PCFGbits.PCFG1   // AN0/ADC3/33V
#define BP_ADC_5V0_CFG AD1PCFGbits.PCFG0   // AN2/ADC4/50V
#define BP_ADC_VPU_CFG AD1PCFGbits.PCFG5   // AN5/ADC2/Vextpullup
#define BP_ADC_USB_CFG AD1PCFGbits.PCFG15  // AN15/ADC5/Vusb

// error fixing defines (find with keyword #BPV4
#define OC5CON OC5CON1

//////////// BPv4 Peripheral Pin Select (PPS) Definitions
// Input/Output PPS Definitions
// MISO / MOSI / CS / CLK
#define BP_MISO_RPIN 22
#define BP_MISO_RPOUT RPOR11bits.RP22R
#define BP_MOSI_RPIN 24
#define BP_MOSI_RPOUT RPOR12bits.RP24R
#define BP_CS_RPIN 25
#define BP_CS_RPOUT RPOR12bits.RP25R
#define BP_CLK_RPIN 23
#define BP_CLK_RPOUT RPOR11bits.RP23R
// AUX0:2 (AUX0 = AUX / AUX = AUX0)
#define BP_AUX0_RPIN 20
#define BP_AUX0_RPOUT RPOR10bits.RP20R
#define BP_AUX1_RPIN 2
#define BP_AUX1_RPOUT RPOR1bits.RP2R
#define BP_AUX2_RPIN 11
#define BP_AUX2_RPOUT RPOR5bits.RP11R
// OTHER
#define BP_ADC_RPIN 1
#define BP_ADC_RPOUT RPOR1bits.RP1R

// PPS Compatability
#define BP_AUX_RPIN BP_AUX0_RPIN
#define BP_AUX_RPOUT BP_AUX0_RPOUT

#define CFG_ADDR_UPPER 0x0001
#define CFG_ADDR_0 0x57FA
#define CFG_ADDR_1 0x57FC
#define CFG_ADDR_2 0x57FE

#define PIC_REV_A3 0x0001
#define PIC_REV_A5 0x0003
#define PIC_REV_A6 0x0004

static inline void bp_enable_usb_led(void) {
  BP_LEDUSB_DIR = OUTPUT;
  BP_LEDUSB = ON;
}

static inline void bp_disable_usb_led(void) {
  BP_LEDUSB_DIR = OUTPUT;
  BP_LEDUSB = OFF;
}

static inline void bp_toggle_usb_led(void) {
  BP_LEDUSB_DIR = OUTPUT;
  BP_LEDUSB ^= ON;
}

static inline void bp_adc_pin_setup(void) {
  BP_ADC_PROBE_CFG = LOW;
  BP_ADC_3V3_CFG = LOW;
  BP_ADC_5V0_CFG = LOW;
  BP_ADC_VPU_CFG = LOW;
  BP_ADC_USB_CFG = LOW;
}

static inline void bp_enable_3v3_pullup(void) {
  BP_PUVSEL50_DIR = INPUT;
  BP_PUVSEL33 = LOW;
  BP_PUVSEL33_DIR = OUTPUT;
}

static inline void bp_disable_3v3_pullup(void) {
  BP_PUVSEL50_DIR = INPUT;
  BP_PUVSEL33 = LOW;
  BP_PUVSEL33_DIR = INPUT;
}

static inline void bp_set_3v3_pullup_state(bool state) {
  if (state == ON) {
    bp_enable_3v3_pullup();
  } else {
    bp_disable_3v3_pullup();
  }
}

static inline void bp_enable_5v0_pullup(void) {
  BP_PUVSEL33_DIR = INPUT;
  BP_PUVSEL50 = LOW;
  BP_PUVSEL50_DIR = OUTPUT;
}

static inline void bp_disable_5v0_pullup(void) {
  BP_PUVSEL33_DIR = INPUT;
  BP_PUVSEL50_DIR = INPUT;
}

static inline void bp_set_5v0_pullup_state(bool state) {
  if (state == ON) {
    bp_enable_5v0_pullup();
  } else {
    bp_disable_5v0_pullup();
  }
}

#endif /* !BP_HARDWAREV4_H */
