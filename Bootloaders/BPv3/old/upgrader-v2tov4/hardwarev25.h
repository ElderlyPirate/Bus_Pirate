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

//this profile is for the Bus Pirate v2go hardware
//http://

#define BP_VERSION_STRING "Bus Pirate v2go"
#define BP_VERSION "v2go"

//set the pin to use for POST detection
#define BP_POST 	PORTBbits.RB1
#define BP_POST_DIR TRISBbits.TRISB1

//this configures the placement of the serial port for 
//the PC side terminal interface
//the other modules should be similarly abstracted, but since v0A and v1a share pins, it doesn't mater
#define BP_TERM_TX		U1TX_IO;
#define BP_TERM_TX_RP  	RPOR2bits.RP4R
#define BP_TERM_RX		RPINR18bits.U1RXR
#define BP_TERM_RX_RP	5

//this is a different type of pin identifier used by the bitbang library
#define AUX		0b10000000000 //RB10 on v1, 2, 2go, 3 (not v0)
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
#define BP_AUX 			PORTBbits.RB10 //moved pin5 (AUX) to RB10
#define BP_LEDMODE		PORTAbits.RA1 //single MODE led on v2a
#define BP_VREGEN		PORTAbits.RA0 //single vreg enable pin on v2a
#define BP_PULLUP		PORTBbits.RB11 //Pull-up pin on V2a

//these macros set the direction registers for each IO pin
#define BP_MOSI_DIR 	TRISBbits.TRISB9
#define BP_CLK_DIR 		TRISBbits.TRISB8
#define BP_MISO_DIR 	TRISBbits.TRISB7
#define BP_CS_DIR 		TRISBbits.TRISB6
#define BP_AUX_DIR 		TRISBbits.TRISB10
#define BP_LEDMODE_DIR	TRISAbits.TRISA1
#define BP_VREGEN_DIR	TRISAbits.TRISA0
#define BP_PULLUP_DIR	TRISBbits.TRISB11

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

//pseudofunctions for pullup resistors
//for V2a we need B5/pullup to be output and gnd to turn OFF the pullups...
#define BP_PULLUP_ON() BP_PULLUP_DIR=1;BP_PULLUP=0
#define BP_PULLUP_OFF() BP_PULLUP=0;BP_PULLUP_DIR=0

//pseudofunctions for voltage regulator switch
#define BP_VREG_ON() BP_VREGEN_DIR=0; BP_VREGEN=1
#define BP_VREG_OFF() BP_VREGEN_DIR=0; BP_VREGEN=0

//define device ID and revision memory location
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
