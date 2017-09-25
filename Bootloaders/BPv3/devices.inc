;------------------------------------------------------------------------------
;
; Title:			ds30 loader for PIC24FJ
;
; File description:	Device specific constants
;
; Copyright: 		Copyright © 2009, Mikael Gustafsson
;
; Webpage: 			http://mrmackey.no-ip.org/elektronik/ds30loader/
;
; History:			1.0.1 Removed some incorrect include for ga002 and ga004
;					1.0.0 Removed PIC24FxxKAyyy stuff, se separate fw
;					0.9.1 Added pps equ
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
;    along with ds30 Loader. If not, see <http:;www.gnu.org/licenses/>.
;------------------------------------------------------------------------------ 


;------------------------------------------------------------------------------
; Devices PIC24F - 16kB
;------------------------------------------------------------------------------ 
.ifdef __24FJ16GA002 
	.include "p24FJ"
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x2C00
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	206
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ16GA004 
	.include "p24FJ"
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x2C00
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	207
	.equ	HAS_PPS,	1
.endif


;------------------------------------------------------------------------------
; Devices PIC24F - 32kB
;------------------------------------------------------------------------------ 
.ifdef __24FJ32GA002
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x5800
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	208
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ32GA004
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x5800
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	209
	.equ	HAS_PPS,	1
.endif


;------------------------------------------------------------------------------
; Devices PIC24F - 48kB
;------------------------------------------------------------------------------ 
.ifdef __24FJ48GA002 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x8400
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	210
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ48GA004
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x8400
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	211
	.equ	HAS_PPS,	1
.endif


;------------------------------------------------------------------------------
; Devices PIC24F - 64kB
;------------------------------------------------------------------------------ 
.ifdef __24FJ64GA002 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0xAC00
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	212
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ64GA004
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0xAC00
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	213.
	equ	HAS_PPS,	1
.endif

.ifdef __24FJ64GA006 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0xAC00
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	214
.endif

.ifdef __24FJ64GA008 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0xAC00
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	215
.endif

.ifdef __24FJ64GA010 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0xAC00
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	216
.endif

.ifdef __24FJ64GB106 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0xAC00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1
	.equ	DEVICEID,	217
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ64GB108 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0xAC00
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	218
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ64GB110 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0xAC00
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	219
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1
	.equ	HAS_PPS,	1
.endif


;------------------------------------------------------------------------------
; Devices PIC24F - 96kB
;------------------------------------------------------------------------------ 
.ifdef __24FJ96GA006
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x10000
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	220
.endif

.ifdef __24FJ96GA008	
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x10000
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	221
.endif

.ifdef __24FJ96GA010
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x10000
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	222
.endif


;------------------------------------------------------------------------------
; Devices PIC24F - 128kB
;------------------------------------------------------------------------------ 
.ifdef __24FJ128GA006 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x15800
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	223
.endif

.ifdef __24FJ128GA008 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x15800
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	224
.endif

.ifdef __24FJ128GA010 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x15800
	.equ	HAS_UART2,	1
	.equ	DEVICEID,	225
.endif

.ifdef __24FJ128GA106 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x15800
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1
	.equ	DEVICEID,	226
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ128GA108 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x15800
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1
	.equ	DEVICEID,	227
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ128GA110 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x15800
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1
	.equ	DEVICEID,	228
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ128GB106 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x15800
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	229
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ128GB108 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x15800
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	230
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ128GB110 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x15800
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	231
	.equ	HAS_PPS,	1
.endif


;------------------------------------------------------------------------------
; Devices PIC24F - 192kB
;------------------------------------------------------------------------------ 
.ifdef __24FJ192GA106 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x20C00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	232
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ192GA108 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x20C00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	233
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ192GA110 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x20C00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	234
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ192GB106 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x20C00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	235
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ192GB108 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x20C00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	236
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ192GB110 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x20C00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	237
	.equ	HAS_PPS,	1
.endif


;------------------------------------------------------------------------------
; Devices PIC24F - 256kB
;------------------------------------------------------------------------------ 
.ifdef __24FJ256GA106 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x2AC00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	238
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ256GA108 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x2AC00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	239
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ256GA110 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x2AC00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	240
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ256GB106 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x2AC00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1
	.equ	DEVICEID,	241
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ256GB108 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x2AC00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	242
	.equ	HAS_PPS,	1
.endif

.ifdef __24FJ256GB110 
	.equ	VALID_DEV, 1
	.equ	FLASHSIZE,	0x2AC00
	.equ	HAS_UART2,	1
	.equ	HAS_UART3,	1
	.equ	HAS_UART4,	1	
	.equ	DEVICEID,	243
	.equ	HAS_PPS,	1
.endif



;------------------------------------------------------------------------------
; 
;------------------------------------------------------------------------------ 
.ifndef VALID_DEV
	.error "Unknown device specified"
.endif
