; ds30 Loader, copyright 2009 Mikael Gustafsson; GPL (see COPYING.txt).
; Bus Pirate v3 only: PIC24FJ64GA002, UART1 RP5 RX / RP4 TX.
        .include "p24FJ64GA002.inc"
        .equ FCY, 16000000
        .equ BAUDRATE, 115200
        .equ UARTBR, 0x22 ; Retain silicon A3 UART erratum workaround.
        .equ BLDELAY, 17 ; About 560 ms, 8 cycles per empty receive-loop pass.
        .equ ROWSIZE, 64
        .equ STARTADDR, 0xA800
        .equ BLCHECKST, STARTADDR-ROWSIZE*2
        .equ DEVICEID, 212 ; ds30 ID, not the silicon DEVID.
        .equ BLVERSION, 0x0500
