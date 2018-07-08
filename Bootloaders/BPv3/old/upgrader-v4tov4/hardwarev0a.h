/*
 * This file is part of the Bus Pirate project (http://code.google.com/p/the-bus-pirate/).
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

//this hardware profile is for the first, through-hole Bus Pirate.
//http://hackaday.com/2008/11/19/how-to-the-bus-pirate-universal-serial-interface/

#define BP_VERSION_STRING "Pirate v0a"
#define BP_VERSION "v0a"

//this configures the proper placement of the serial port for 
//the PC side terminal interface
#define BP_TERM_TX		U1TX_IO;
#define BP_TERM_TX_RP  	RPOR1bits.RP3R
#define BP_TERM_RX		RPINR18bits.U1RXR
#define BP_TERM_RX_RP	4

//this is a different type of pin identifier used by the bitbang library
#define MOSI 	0b1000000000  //RB9
#define CLK 	0b100000000    //RB8
#define MISO 	0b10000000 //RB7
#define CS 		0b1000000 //RB6
#define IOPOR 	PORTB
#define IOLAT	LATB
#define IODIR	TRISB

//these macros set the pin IO used in each module
#define BP_MOSI 		PORTBbits.RB9
#define BP_CLK 			PORTBbits.RB8
#define BP_MISO 		PORTBbits.RB7
#define BP_CS 			PORTBbits.RB6
#define BP_AUX 			PORTBbits.RB5
#define BP_PULLUP1 		PORTBbits.RB11
#define BP_PULLUP2 		PORTBbits.RB10
#define BP_LEDMODE		PORTBbits.RB14

//these macros set the direction registers for each IO pin
#define BP_MOSI_DIR 	TRISBbits.TRISB9
#define BP_CLK_DIR 		TRISBbits.TRISB8
#define BP_MISO_DIR 	TRISBbits.TRISB7
#define BP_CS_DIR 		TRISBbits.TRISB6
#define BP_AUX_DIR 		TRISBbits.TRISB5
#define BP_PULLUP1_DIR 	TRISBbits.TRISB11 //SDA
#define BP_PULLUP2_DIR 	TRISBbits.TRISB10 //SCL
#define BP_LEDMODE_DIR	TRISBbits.TRISB14

//Open drain/high impedance pin setup
#define BP_MOSI_ODC 	ODCBbits.ODB9
#define BP_CLK_ODC 		ODCBbits.ODB8
#define BP_MISO_ODC 	ODCBbits.ODB7
#define BP_CS_ODC 		ODCBbits.ODB6
#define BP_AUX_ODC 		ODCBbits.ODB5

//define the PPS input/output for the AUX pin
#define BP_AUX_RPIN 	5	
#define BP_AUX_RPOUT 	RPOR2bits.RP5R
#define BP_CS_RPIN 6
#define BP_CS_RPOUT RPOR5bits.RP6R

//pseudo function to reset pins to hizstate
//pseudofunctions for pullup resistors
#define BP_PULLUP_ON() 	BP_PULLUP1=1;BP_PULLUP2=1;BP_PULLUP1_DIR=0;BP_PULLUP2_DIR=0
#define BP_PULLUP_OFF() BP_PULLUP1_DIR=1;BP_PULLUP2_DIR=1

//define device config bit, ID, and revision memory location
#define DEV_ADDR_UPPER	0x00FF
#define DEV_ADDR_TYPE	0x0000
#define DEV_ADDR_REV	0x0002
#define CFG_ADDR_UPPER 0x0000
#define CFG_ADDR_1     0xABFC
#define CFG_ADDR_2     0xABFE

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
