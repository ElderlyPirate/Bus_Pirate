;
; OpenOCD_asm.s
;
; Optimized function for the OpenOCD mode
;
; Written by Sylvain Munaut <tnt@246tNt.com>
;
;
; Published in the public domain.
; For details see: http://creativecommons.org/publicdomain/zero/1.0/.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty o
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;

.ifdef __PIC24FJ256GB106__
	.error "This is only for Bus Pirate v3"
.endif ; __PIC24FJ256GB106__

.ifdef __PIC24FJ64GA002__
	.equ __24FJ64GA002, 1
	.include "p24FJ64GA002.inc"
.endif ; __PIC24FJ64GA002__

;
; Hardware configuration
;

;  Do we support delay ?
.equ HAS_DELAY, 1

;  Bus pirate v3 hardware
.equ IOPOR, PORTB

; .equ BP_MOSI_BIT, RB9
; .equ BP_CLK_BIT, RB8
; .equ BP_MISO_BIT, RB7
; .equ BP_CS_BIT, RB6
	
.equ BP_MOSI_BIT, #0x0009
.equ BP_CLK_BIT, #0x0008
.equ BP_MISO_BIT, #0x0007
.equ BP_CS_BIT, #0x0006

;  Pin usage by OpenOCD
.equ OOCD_TDO_BIT, BP_MISO_BIT
.equ OOCD_TMS_BIT, BP_CS_BIT
.equ OOCD_CLK_BIT, BP_CLK_BIT
.equ OOCD_TDI_BIT, BP_MOSI_BIT

;
; void binOpenOCDTapShiftFast(void *in_buf, void *out_buf, unsigned int bits, unsigned int delay)
;
; Parameters:
;  w0 : input buffer
;  w1 : output buffer
;  w2 : # of bits
;  w3 : delay (depends on config)
;
; Register usage:
;
;  w0  : #bytes processed in the inner loop (-1) / tmp register elsewhere
;  w1  : bit loop counter in the inner loop      / tmp register elsewhere
;  w2  : TDI data out
;  w3  : TMS data out
;  w4  : TDO data in
;
;  w5  : total bytes consumed
;  w6  : #bits_left - 1
;  w7  : src ptr
;  w8  : dst ptr
;
;  w9 :  constant IOPR
;  w10 : constant OOCD_TDI_BIT
;  w11 : constant OOCD_TMS_BIT
;  w12 : delay loop
;

	.text
	.global _binOpenOCDTapShiftFast

	.extern _UART1RXToRecv
	.extern _UART1RXRecvd
	.extern _UART1TXAvailable
	.extern _UART1TXSent
	.extern _UART1TXBuf
	.extern _UART1RXBuf
	
_binOpenOCDTapShiftFast:

		; Save registers
		push.d	w8			; save w8,w9
		push.d	w10			; save w10,w11
.ifdef HAS_DELAY
		push.w	w12			; save 12
.endif

		; Init consumed byte counter
		clr	w5			; w5 = 0

		; Save parameters
		dec.w	w2, w6			; w6 = w2 - 1;
		mov.w	w0, w7			; w7  = w0;
		mov.w	w1, w8			; w8  = w1;
.ifdef HAS_DELAY
		mov.w	w3, w12
.endif

		; Constants
		mov.w	#IOPOR, w9		; w9 = IOPOR;
		mov.w	#OOCD_TDI_BIT, w10	; w10 = OOCD_TDI_BIT;
		mov.w	#OOCD_TMS_BIT, w11	; w11 = OOCD_TMS_BIT;

		; Outer (word) loop
__loop_word:					; do {

		;   Wait until there are enough bytes RXed
		mov.w	_UART1RXToRecv, w0	;   w0 = UART1RXToRecv;
		add.w	w5, #4, w5		;   w5 += 4;
		cpsgt	w5, w0			;   w0 = min(w0, w5)
		mov.w	w5, w0
1:
		cp	_UART1RXRecvd		;   while (_UART1RXRecvd < w0);
		bra	lt, 1b

		;   Fetch and organize 2x16 bits
		mov.w	[w7++], w2		;   w2 = *((uint16_t*)w7++);
		mov.w	[w7++], w3		;   w3 = *((uint16_t*)w7++);

			; If memory had bytes 0xAA 0xBB 0xCC 0xDD,
			; we now have:
			;   w2 = 0xBBAA;
			;   w3 = 0xDDCC;
			;
			; and we want:
			;   w2 = 0xCCAA;	/* TDI bits (lsb out first) */
			;   w3 = 0xDDBB;	/* TMS bits (lsb out first) */
		swap.w	w2			;   w2 = swap_bytes(w2);
		xor.w	w2, w3, w4		;   w4 = w2 ^ w3;
		and.w	#0xff, w4		;   w4 &= 0xff;
		xor.w	w2, w4, w2		;   w2 = w2 ^ w4;
		xor.w	w3, w4, w3		;   w3 = w3 ^ w4;
		swap.w	w2			;   w2 = swap_bytes(w2);

		;   Compute how much bits to process
		mov.w	#15, w0			;   w0 = min(15, w6)
		cpsgt.w	w6, w0
		mov.w	w6, w0

		;   Inner (bit) loop
		mov	w0, w1			;   w1 = w0;
__loop_bit:					;   do {
 
 		;     Delay loop
.ifdef HAS_DELAY
		repeat	w12
		nop
.endif

		;     Clear TCK
		bclr.w	[w9], #OOCD_CLK_BIT

		;     Output TMS & TDI
		rrc.w	w2, w2
		bsw.c	[w9], w10
		rrc.w	w3, w3
		bsw.c	[w9], w11

 		;     Delay loop
.ifdef HAS_DELAY
		repeat	w12
		nop
.else
		nop				; /* still must pause a cycle because */
						; /* we wrote [w9] last cycle ...     */
.endif

		;     Set TCK
		bset.w	[w9], #OOCD_CLK_BIT

		;     Sample TDO
		btst.c	[w9], #OOCD_TDO_BIT
		rrc.w	w4, w4

		;     Inner (bit) loop branch condition
		dec.w	w1, w1			;   } while (--w1 >= 0);
		bra	c, __loop_bit

		;   Fix & Store the result
		subr	w0, #15, w1		;   w1 = 15 - w0;
		lsr	w4, w1, w4		;   w4 >>= w1;

		mov.w	w4, [w8++]		;   *((uint16_t*)w8++) = w4;

		;   Increment available bytes
		lsr.w	w0, #3, w0		;   w0 = w0 >> 3;
		inc.w	w0, w0			;   w0 = w0 + 1;
		add	_UART1TXAvailable	;   UART1TXAvailable += w0;

		;   Trigger TX if needed
		;   (we're in the loop, so use optimized version, also we can't
		;   loose register content -> no function call)
		
		; Referencing an .equiv symbol does not work anymore?
; 		btst	IEC0, U1TXIE
		btst	IEC0, #0x000C
		bra	z, 1f

		mov	_UART1TXSent, w0
		cp	_UART1TXAvailable
		bra	z, 1f

;		bclr	IFS0, U1TXIF
		bclr	IFS0, #0x000C
;		bset	IEC0, U1TXIE
		bset	IEC0, #0x000C
		add	_UART1TXBuf, wreg
		mov	[w0], w0
		mov	w0, U1TXREG
1:

		;   Outer (word) loop branch condition
		sub.w	w6, #16, w6		;   w6 -= 16;
		bra	c, __loop_word		; } while (w6>=0);

		; Restore registers
.ifdef HAS_DELAY
		pop.w	w12			; restore w12
.endif
		pop.d	w10			; restore w10,w11
		pop.d	w8			; restore w8,w9

		; If there was anything left
		goto	_user_serial_process_transmission_interrupt

		; (we used a goto, no need for return :)

