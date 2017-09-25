;------------------------------------------------------------------------------
;
; Title:			ds30 loader for PIC24FJ
;
; File description:	user settings and configuration bits
;
; Copyright: 		Copyright © 2009, Mikael Gustafsson
;
; Webpage: 			http://mrmackey.no-ip.org/elektronik/ds30loader/
;
; History:			1.0.1 -
;					1.0.0 Removed PIC24FxxKAyyy stuff, se separate fw
;					0.9.1 Only one clock option in this file
;					0.9.0 First version of this file
;                                                                             
;------------------------------------------------------------------------------

;-----------------------------------------------------------------------------
;    This file is part of ds30 Loader.
;
;    ds30 Loader is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation.
;
;    ds30 Loader is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with ds30 Loader. If not, see <http://www.gnu.org/licenses/>.
;------------------------------------------------------------------------------ 


;------------------------------------------------------------------------------
; Device
;------------------------------------------------------------------------------
        .equ 		__24FJ64GA002, 	1		;xxx
		.equ			BUSPIRATEV2, 1				;use pin config for v2go/v3
		;.equ			BUSPIRATEV1A, 1				;use pin config for v1a

;------------------------------------------------------------------------------
; Includes
;------------------------------------------------------------------------------
        .include	"devices.inc"
        .include 	"p24fxxxx.inc"


;------------------------------------------------------------------------------
; User preferences
;------------------------------------------------------------------------------
		.equ 	FCY, 		16000000			;xxx speed of internal cycle clock[Hz], used to calculate uart brg and delay
		.equ 	BAUDRATE, 	115200				;xxx baudrate
		.equ	BLTIME,		1					;xxx time in ms before bootloader loads the user application
        .equ    USE_UART1,	1					;xxx uncomment to use uart1
        ;.equ	USE_UART2,	1					;xxx uncomment to use uart2
        ;.equ	USE_UART3,	1					;xxx uncomment to use uart3
        ;.equ	USE_UART4,	1					;xxx uncomment to use uart4

;------------------------------------------------------------------------------
; Configuration bits, these macros can be found at the end of the inc-files located in 
; c:\Program files\Microchip\MPLAB ASM30 Suite\Support\dsPIC24x\inc
;
; These can also be set in MPLAB IDE instead, they are found in Configure->Configuration bits...
;------------------------------------------------------------------------------
		;use identical Bus Pirate config fuses
		config __CONFIG2, FNOSC_FRCPLL & OSCIOFNC_ON &POSCMOD_NONE & I2C1SEL_PRI	
		config __CONFIG1, JTAGEN_OFF & GCP_OFF & GWRP_OFF & COE_OFF & FWDTEN_OFF & ICS_PGx1

		;config	__CONFIG1, JTAGEN_OFF/*JTAG*/ & GCP_OFF/*code protect*/ & GWRP_OFF/*write protect*/ & BKBUG_OFF/*background debugger*/ & COE_OFF/*clip-on emulation mode*/ & ICS_PGx1/*icd pins select*/ & FWDTEN_OFF/*watchdog timer*/ & WINDIS_OFF/*windowed wdt*/ & FWPSA_PR32/*watchdog prescalar*/ & WDTPS_PS1/*watchdog postscalar*/
		;config	__CONFIG2, IESO_OFF/*two-speed startup*/ & FNOSC_FRC/*oscillator selection*/ & FCKSM_CSDCMD/*clock switching and clock monitor*/ & OSCIOFNC_OFF/*OSC0/RC15 function*/ & POSCMOD_XT/*oscillator selection*/
		;config 	__CONFIG3, WPEND_WPSTARTMEM/*write protect range selection*/ & WPCFG_WPCFGDIS/*write protection config page*/ & WPDIS_WPEN/*write protection*/ & WPFP_WPFP8/*Write Protection Flash Page*/
