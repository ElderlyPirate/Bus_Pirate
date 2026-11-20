; ds30 Loader for Bus Pirate v3 / PIC24FJ64GA002.
; Copyright 2009 Mikael Gustafsson. GPL; see COPYING.txt.
; Reworked for bounded framing, strict RTSP geometry and reset-safe entry.
; DS39881E 4.3, 5.2, 5.5, 7.2, 9.4 and 16; DS70000157G TBLRD/TBLWT,
; DISI, CP and BTSS/BTSC. Addresses below are PIC program addresses.
        .include "settings.inc"
        .global __reset, __DefaultInterrupt, loader_end
        .equ DOERASE, W1
        .equ WBUFPTR, W2
        .equ WCNT, W3
        .equ WADDR2, W4
        .equ WADDR, W5
        .equ ERASEPAGE, W6
        .equ RESIDENT, W8
        .equ LENGTH, W9
        .equ WDEL1, W10
        .equ WDEL2, W11
        .equ STORED, W12
        .equ WCMD, W13
        .equ WCRC, W14

        .bss
buffer: .space 193
        .section .entry_flags, bss, address(0x27FA)
skip_pgc_pgd_check: .space 2
firmware_signature: .space 4

        .section .app_entry, code, address(STARTADDR-4)
usrapp: .pword 0xFFFFFF, 0xFFFFFF ; Filled by pirate-loader fixJumps().
        .section .version, code, address(0xABF8)
        bra firmwarejump ; Fixed legacy firmware entry ABI.
        .pword BLVERSION

        .section .loader, code, address(STARTADDR)
__reset:
        mov #__SP_init, W15
        mov #__SPLIM_init, W0
        mov W0, SPLIM
        nop
        clr RESIDENT
        clr DOERASE
        clr TBLPAG
        bclr OSCCON, #SOSCEN
        clr CLKDIV
waitPLL:btss OSCCON, #LOCK
        bra waitPLL
        mov #0xFFFF, W0
        mov W0, AD1PCFG
        mov #firmware_signature, W0
        mov [W0++], W1
        mov #0x5926, W2
        cp W1, W2
        bra nz, jumper
        mov [W0], W1
        mov #0x3141, W2
        cp W1, W2
        bra nz, jumper
        mov #skip_pgc_pgd_check, W0
        mov.b [W0], RESIDENT
jumper: mov #skip_pgc_pgd_check, W0
        clr.b [W0]
        clr DOERASE
        bclr LATB, #0
        bclr TRISB, #0
        bset TRISB, #1
        bset CNPU1, #CN5PUE
        repeat #31
        nop
        cp0.b RESIDENT
        bra nz, setup
        btss PORTB, #1
        bra setup
        bra quit

setup:  clr DOERASE
        mov #5, W0
        mov W0, RPINR18
        mov #3, W0
        mov W0, RPOR2
        bset LATA, #1
        bclr TRISA, #1
        clr U1MODE
        clr U1STA
        mov #UARTBR, W0
        mov W0, U1BRG
        bset U1MODE, #BRGH
        bset U1MODE, #UARTEN
        bset U1STA, #UTXEN
hello:  rcall Receive
        mov #0xC1, W3
        cp W0, W3
        bra z, helloOK
        mov #'#', W3
        cp W0, W3
        bra z, quit_uart
        mov #'B', W0
        rcall Send
        mov #'L', W0
        rcall Send
        mov #'4', W0
        rcall Send
        mov #'+', W0
        rcall Send
        bra hello
helloOK:mov #DEVICEID, W0
        rcall Send
        mov #1, W0 ; ds30 protocol version 1.0.2, distinct from BLVERSION.
        rcall Send
        mov #2, W0
        rcall Send
ok:     mov #'K', W0
reply:  rcall Send
main:   clr WCRC
        rcall Receive
        mov W0, TBLPAG
        rcall Receive
        sl W0, #8, WADDR
        rcall Receive
        ior WADDR, W0, WADDR
        mov WADDR, WADDR2
        rcall Receive
        mov W0, WCMD
        rcall Receive
        mov W0, LENGTH
        mov W0, WCNT
        cp0 WCNT
        bra z, malformed
        mov #buffer, WBUFPTR
        clr STORED
rcvdata:rcall Receive
        mov #193, W7
        cp STORED, W7
        bra geu, drain
        mov.b W0, [WBUFPTR++]
        inc STORED, STORED
drain:  dec WCNT, WCNT
        bra nz, rcvdata
        cp0.b WCRC
        bra nz, badcrc
        cp WCMD, #1
        bra z, erasecmd
        cp WCMD, #2
        bra nz, malformed
        mov #193, W0
        cp LENGTH, W0
        bra nz, malformed
        bra addresscheck
erasecmd:
        cp LENGTH, #1
        bra nz, malformed
        mov #0x3FF, W0
        and WADDR, W0, W0
        bra nz, protected
addresscheck:
        mov TBLPAG, W0
        cp0 W0 ; GA002 has no user flash in any other table page.
        bra nz, protected
        mov #0x7F, W0
        and WADDR, W0, W0
        bra nz, protected
        mov #BLCHECKST, W0
        cp WADDR, W0
        bra gtu, protected
        cp WCMD, #1
        bra nz, writerow
        mov WADDR, ERASEPAGE
        mov #1, DOERASE
        bra ok
writerow:
        cp0 DOERASE
        bra z, vector
        mov #0xFC00, W0
        and WADDR, W0, W0
        cp W0, ERASEPAGE
        bra nz, protected
vector: cp0 WADDR
        bra nz, erase
        mov #buffer, WBUFPTR
        mov #4, W0
        mov.b W0, [WBUFPTR++]
        clr.b [WBUFPTR++]
        mov #0xA8, W0
        mov.b W0, [WBUFPTR++]
        clr.b [WBUFPTR++]
        clr.b [WBUFPTR++]
        clr.b [WBUFPTR++]
erase:  cp0 DOERASE
        bra z, program
        clr DOERASE
        tblwtl WADDR, [WADDR]
        mov #0x4042, W0
        rcall Write
        btsc NVMCON, #WRERR
        bra verifyfail
program:mov #buffer, WBUFPTR
        mov #ROWSIZE, WCNT
latlo:  tblwth.b [WBUFPTR++], [WADDR]
        tblwtl.b [WBUFPTR++], [WADDR++]
        tblwtl.b [WBUFPTR++], [WADDR++]
        dec WCNT, WCNT
        bra nz, latlo
        mov #0x4001, W0
        rcall Write
        btsc NVMCON, #WRERR
        bra verifyfail
        mov #buffer, WBUFPTR
        mov #ROWSIZE, WCNT
verify: tblrdh.b [WADDR2], W0
        cp.b W0, [WBUFPTR++]
        bra nz, verifyfail
        tblrdl.b [WADDR2++], W0
        cp.b W0, [WBUFPTR++]
        bra nz, verifyfail
        tblrdl.b [WADDR2++], W0
        cp.b W0, [WBUFPTR++]
        bra nz, verifyfail
        dec WCNT, WCNT
        bra nz, verify
        bra ok
badcrc: mov #'N', W0
        bra error
malformed:
        mov #'U', W0
        bra error
protected:
        mov #'P', W0
        bra error
verifyfail:
        mov #'V', W0
error:  clr DOERASE
        bra reply

Write:  mov W0, NVMCON
        disi #5
        mov #0x55, W0
        mov W0, NVMKEY
        mov #0xAA, W0
        mov W0, NVMKEY
        bset NVMCON, #WR
        nop
        nop
busy:   btsc NVMCON, #WR
        bra busy
        bclr NVMCON, #WREN
        return
Send:   btsc U1STA, #UTXBF
        bra Send
        mov W0, U1TXREG
        return
Receive:mov #BLDELAY, WDEL1
outer:  clr WDEL2
poll:   btsc U1STA, #OERR
        bra receive_error
        btss U1STA, #URXDA
        bra notrcv
        mov U1STA, W0
        and #0xC, W0 ; Framing/parity flags describe the next RX FIFO entry.
        bra nz, receive_error
        mov U1RXREG, W0
        and #0xFF, W0
        add WCRC, W0, WCRC
        return
notrcv: dec WDEL2, WDEL2
        bra nz, poll
        dec WDEL1, WDEL1
        bra nz, outer
        bra timedout
receive_error:
        bclr U1STA, #OERR
flush:  btss U1STA, #URXDA
        bra timedout
        mov U1RXREG, W0
        bra flush
timedout:
        mov #__SP_init, W15 ; Discard the outstanding Receive call frame.
        clr DOERASE
        cp0.b RESIDENT
        bra nz, setup
        btss PORTB, #1
        bra setup
quit_uart:
        btsc U1STA, #TRMT
        bra txdone
        bra quit_uart
txdone: clr U1MODE
        clr U1STA
        bclr IFS0, #U1RXIF
        bclr IFS0, #U1TXIF
        mov #0x1F, W0
        mov W0, RPINR18
        clr RPOR2
        bclr LATA, #1
        bset TRISA, #1
quit:   clr TBLPAG
        mov #tbloffset(usrapp), W0
        tblrdl [W0], W1
        inc W1, W1
        bra nz, app_present
        tblrdh [W0], W1
        mov #0xFF, W2
        cp.b W1, W2
        bra z, setup ; Never execute an erased relocated application vector.
app_present:
        bclr CNPU1, #CN5PUE
        bset TRISB, #0
        clr AD1PCFG
        bra usrapp
firmwarejump:
        ; Legacy direct entry must not inherit application peripherals/IRQs.
        mov #firmware_signature, W0
        mov #0x5926, W1
        mov W1, [W0++]
        mov #0x3141, W1
        mov W1, [W0]
        mov #skip_pgc_pgd_check, W0
        mov #1, W1
        mov.b W1, [W0]
        reset
__DefaultInterrupt:
        reset
loader_end:
        .end
