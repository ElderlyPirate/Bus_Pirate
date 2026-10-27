# Firmware-Alternates

Stand-alone firmware images that are not part of the main Bus Pirate
firmware build.

## firmware-STK500v2

Turns the Bus Pirate into a plain **STK500v2 / AVRISP** serial
programmer, so standard tooling (avrdude, AVR Studio) can use the board
without Bus Pirate-specific host support.  Based on Guido Socher's GPL
implementation, ported to the PIC24 by Ian Lesnet (2009).  See
`gpl.txt` inside the project.

This alternate is **not** merged into the main firmware, and cannot be
merged as-is: the Bus Pirate firmware is CC0-licensed while this
project is GPL.  A possible future task is porting its STK500**v2**
protocol support into the main firmware as an upgrade of the currently
active STK500**v1** AVR programming path (the avrdude `buspirate`
programmer), which would also bring it to BPv4.

## History

The following projects were removed (2026-10), as they are superseded:

- `firmware-XSVFplayer` — XSVF playback is integrated in the main
  firmware (`Firmware/jtag/`, BPv4) with host tools in `scripts/`; the
  alternate only targeted the v0a–v2.5 boards and carried MPLAB 8
  workspace files.
- `ADC2UART` — v3-era "ADC to UART responder" demo; superseded by the
  main firmware's ADC probe and binary ADC commands.
- `Blink_MODE_LED` — bring-up LED blinker; covered by the self-test.
- `BusPirateADCDemo` — ADC-reading-to-LED demo; superseded.
- `BusPirateUARTDEMOTEST` — UART echo demo; superseded by the main
  firmware's UART mode and bridge.

They remain available in the repository history if ever needed.