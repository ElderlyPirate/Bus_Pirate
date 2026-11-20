/*
 * This file is part of the Bus Pirate project (buspirate.com).
 *
 * Originally written by hackaday.com <legal@hackaday.com>
 *
 * To the extent possible under law, hackaday.com <legal@hackaday.com> has
 * waived all copyright and related or neighboring rights to Bus Pirate. This
 * work is published from United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

//this hardware profile is for the second, surface mount Bus Pirate.
//http://hackaday.com/2009/01/22/how-to-bus-pirate-v1-improved-universal-serial-interface

#define BP_VERSION_STRING "Hack a Day Bus Pirate v1a"
#define BP_VERSION "v1a"

//this configures the placement of the serial port for 
//the PC side terminal interface
//the other modules should be similarly abstracted, but since v0A and v1a share pins, it doesn't mater
//when we move to a USB PIC, we'll need to do that....
#define BP_TERM_TX		U1TX_IO;
#define BP_TERM_TX_RP  	RPOR1bits.RP2R
#define BP_TERM_RX		RPINR18bits.U1RXR
#define BP_TERM_RX_RP	3

//these macros set the pin IO used in each module
#define BP_MOSI 		PORTBbits.RB9
#define BP_CLK 			PORTBbits.RB8
#define BP_MISO 		PORTBbits.RB7
#define BP_CS 			PORTBbits.RB6
#define BP_AUX 			PORTBbits.RB10 //moved pin5 (AUX) to RB10
#define BP_PIN6			PORTBbits.RB11
#define BP_LEDMODE		PORTBbits.RB4
#define BP_LEDPWR		PORTAbits.RA3
#define BP_EN50V		PORTAbits.RA4
#define BP_EN33V		PORTBbits.RB5

//these macros set the direction registers for each IO pin
#define BP_MOSI_DIR 	TRISBbits.TRISB9
#define BP_CLK_DIR 		TRISBbits.TRISB8
#define BP_MISO_DIR 	TRISBbits.TRISB7
#define BP_CS_DIR 		TRISBbits.TRISB6
#define BP_AUX_DIR 		TRISBbits.TRISB10
#define BP_PIN6_DIR 	TRISBbits.TRISB11
#define BP_LEDMODE_DIR		TRISBbits.TRISB4
#define BP_LEDPWR_DIR		TRISAbits.TRISA3
#define BP_EN50V_DIR	TRISAbits.TRISA4
#define BP_EN33V_DIR	TRISBbits.TRISB5

//Open drain/high impedance pin setup
#define BP_MOSI_ODC 	ODCBbits.ODB9
#define BP_CLK_ODC 		ODCBbits.ODB8
#define BP_MISO_ODC 	ODCBbits.ODB7
#define BP_CS_ODC 		ODCBbits.ODB6
#define BP_AUX_ODC 		ODCBbits.ODB10

//define the PPS input/output for the AUX pin
#define BP_AUX_RPIN 10	
#define BP_AUX_RPOUT RPOR5bits.RP10R
#define BP_CS_RPIN 6
#define BP_CS_RPOUT RPOR5bits.RP6R

//pseudo function to reset pins to hizstate
//pseudofunctions for voltage regulator switch
#define BP_VREG_ON() BP_EN50V_DIR=0; BP_EN50V=1; BP_EN33V_DIR=0; BP_EN33V=1
#define BP_VREG_OFF() BP_EN50V_DIR=0; BP_EN50V=0; BP_EN33V_DIR=0; BP_EN33V=0

// Peripheral Pin Select Outputs
#define NULL_IO		0
#define C1OUT_IO	1
#define C2OUT_IO	2
#define U1TX_IO		3
#define U1RTS_IO	4
#define U2TX_IO		5
#define U2RTS_IO	6
#define SDO1_IO		7
#define SCK1OUT_IO	8
#define SS1OUT_IO	9
#define SDO2_IO		10
#define SCK2OUT_IO	11
#define SS2OUT_IO	12
#define OC1_IO		18
#define OC2_IO		19
#define OC3_IO		20
#define OC4_IO		21
#define OC5_IO		22
