;*
;* This file is part of the Bus Pirate project (http://code.google.com/p/the-bus-pirate/).
;*
;* Written and maintained by the Bus Pirate project.
;*
;* To the extent possible under law, the project has
;* waived all copyright and related or neighboring rights to Bus Pirate. 
;*
;* For details see: http://creativecommons.org/publicdomain/zero/1.0/.
;*
;*    This program is distributed in the hope that it will be useful,
;*    but WITHOUT ANY WARRANTY; without even the implied warranty of
;*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;* 
; CvD: mostly copied from the C30loader (http://mrmackey.no-ip.org/elektronik/ds30loader/)
;
; CvD: does the licenses match up with ours? or did i copy to less? the code is also more or less publices by microchip.

; some equ's
.equ	ROWSIZE,	64


; erase a page
; W0 contains page
; 
; void Ferase(int page);

.global _Ferase

_Ferase:tblwtl		W0, [W0]	; erase page 0x10
		mov			#0x4042, W0	; WREN=1, ERASE=1, NVMOP=mempageerase
		rcall		write
		return

; write a row
;
; void Fwrite(char *buf, int address);

.global _Fwrite

_Fwrite:push		W2
		mov			#ROWSIZE, W2
latlo:	tblwth.b	[W0++], [W1]			; transfer 3 bytes
		tblwtl.b	[W0++], [W1++]			; is the sequence ok? since we convert byte aray to programwords
		tblwtl.b	[W0++], [W1++]
		dec			W2, W2
		bra			nz, latlo
		mov			#0x4001, W0	; WREN=1, ERAsE=0, NVMOP=memrowprog
		rcall		write
		pop			W2
		return

; general write function
; unlocksequence+set wr bit and wait until operation completes.
; W0 = value to write to NVM
; 
write:	mov			W0, NVMCON
		mov			#0x55, W0
		mov			W0, NVMKEY
		mov			#0xAA, W0
		mov			W0, NVMKEY
		bset		NVMCON, #15	; bit15
		nop
		nop
chk:	btsc		NVMCON, #15	; bit15
		bra			chk
		return
		
