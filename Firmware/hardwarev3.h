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

#ifndef BP_HARDWAREV3_H
#define BP_HARDWAREV3_H

#include "types.h"

/**
 * Set this to 1 if you want to build a v3 firmware that is fully backwards
 * compatible with v2, v2.5, and v2go.
 */
#define BP_VERSION2_SUPPORT 0

#define BP_VERSION_STRING "Bus Pirate v3"
#define BP_VERSION "v3"

#define NUM_OF_SUMP_CHANNELS 5

// this configures the placement of the serial port for
// the PC side terminal interface
// the other modules should be similarly abstracted, but since v0A and v1a share
// pins, it doesn't mater
#define BP_TERM_TX U1TX_IO;
#define BP_TERM_TX_RP RPOR2bits.RP4R
#define BP_TERM_RX RPINR18bits.U1RXR
#define BP_TERM_RX_RP 5

// this is a different type of pin identifier used by the bitbang library
#define AUX 0b0000010000000000  // RB10 on v1, 2, 2go, 3 (not v0)
#define MOSI 0b0000001000000000 // RB9
#define CLK 0b0000000100000000  // RB8
#define MISO 0b0000000010000000 // RB7
#define CS 0b0000000001000000   // RB6
#define IOPOR PORTB
#define IOLAT LATB
#define IODIR TRISB

// these macros set the pin IO used in each module
#define BP_MOSI PORTBbits.RB9
#define BP_CLK PORTBbits.RB8
#define BP_MISO PORTBbits.RB7
#define BP_CS PORTBbits.RB6
#define BP_AUX0 PORTBbits.RB10   // moved pin5 (AUX) to RB10
#define BP_LEDMODE PORTAbits.RA1 // single MODE led on v2a
#define BP_VREGEN PORTAbits.RA0  // single vreg enable pin on v2a
#define BP_PULLUP PORTBbits.RB11 // Pull-up pin on V2a
#define BP_PGD PORTBbits.RB0     // PGD pin on programming header

// these macros set the direction registers for each IO pin
#define BP_MOSI_DIR TRISBbits.TRISB9
#define BP_CLK_DIR TRISBbits.TRISB8
#define BP_MISO_DIR TRISBbits.TRISB7
#define BP_CS_DIR TRISBbits.TRISB6
#define BP_AUX0_DIR TRISBbits.TRISB10
#define BP_LEDMODE_DIR TRISAbits.TRISA1
#define BP_VREGEN_DIR TRISAbits.TRISA0
#define BP_PULLUP_DIR TRISBbits.TRISB11
#define BP_PGD_DIR TRISBbits.TRISB0

// Open drain/high impedance pin setup
#define BP_MOSI_ODC ODCBbits.ODB9
#define BP_CLK_ODC ODCBbits.ODB8
#define BP_MISO_ODC ODCBbits.ODB7
#define BP_CS_ODC ODCBbits.ODB6
#define BP_AUX0_ODC ODCBbits.ODB10

// Change notice assignment
#define BP_MOSI_CN CNEN2bits.CN21IE
#define BP_CLK_CN CNEN2bits.CN22IE

/*
 * Change notice assignments for the pins bp_aux_pin_read() can select. The
 * device pin table maps RB10 to CN16 and RB6 to CN24, and RB9/RB8 to
 * CN21/CN22 as used above.
 */
#define BP_AUX0_CN CNEN2bits.CN16IE
#define BP_CS_CN CNEN2bits.CN24IE

// ADC assignment
#define BP_ADC_PROBE 12
#define BP_ADC_3V3 10
#define BP_ADC_5V0 9
#define BP_ADC_VPU 11

// ADC pin configuration
#define BP_ADC_PROBE_CFG AD1PCFGbits.PCFG12 // B12/AN12/ADC1/EXT
#define BP_ADC_3V3_CFG AD1PCFGbits.PCFG10   // B14/AN10/ADC3/33V
#define BP_ADC_5V0_CFG AD1PCFGbits.PCFG9    // B15/AN9/ADC4/50V
#define BP_ADC_VPU_CFG AD1PCFGbits.PCFG11   // B13/AN11/ADC2/Vextpullup

#define BL_ADDR_VER 0xABFA

// flow control connections for FTDI chip
// used in uart brige
#define FTDI_RTS_DIR TRISAbits.TRISA4
#define FTDI_RTS PORTAbits.RA4
#define FTDI_CTS_DIR TRISAbits.TRISA3
#define FTDI_CTS PORTAbits.RA3

// defines the PPS input/output for MISO and MOSI pins
#define BP_MISO_RPIN 7
#define BP_MISO_RPOUT RPOR3bits.RP7R
#define BP_MOSI_RPIN 9
#define BP_MOSI_RPOUT RPOR4bits.RP9R

// define the PPS input/output for the AUX pin
#define BP_AUX_RPIN 10
#define BP_AUX_RPOUT RPOR5bits.RP10R
#define BP_CS_RPIN 6
#define BP_CS_RPOUT RPOR5bits.RP6R

// define the PPS input/output for CLK pin
#define BP_CLK_RPIN 8
#define BP_CLK_RPOUT RPOR4bits.RP8R

#define CFG_ADDR_UPPER 0x0000
#define CFG_ADDR_1 0xABFC
#define CFG_ADDR_2 0xABFE

#define PIC_REV_A3 0x3003
#define PIC_REV_B4 0x3042
#define PIC_REV_B5 0x3043
#define PIC_REV_B8 0x3046

static inline void bp_enable_3v3_pullup(void) {}

static inline void bp_disable_3v3_pullup(void) {}

static inline void bp_set_3v3_pullup_state(bool state __attribute__((unused))) {
}

static inline void bp_enable_5v0_pullup(void) {}

static inline void bp_disable_5v0_pullup(void) {}

static inline void bp_set_5v0_pullup_state(bool state __attribute__((unused))) {
}

static inline void bp_enable_usb_led(void) {}

static inline void bp_disable_usb_led(void) {}

static inline void bp_toggle_usb_led(void) {}

static inline void bp_adc_pin_setup(void) {
  BP_ADC_PROBE_CFG = LOW;
  BP_ADC_3V3_CFG = LOW;
  BP_ADC_5V0_CFG = LOW;
  BP_ADC_VPU_CFG = LOW;
}

#endif /* !BP_HARDWAREV3_H */
