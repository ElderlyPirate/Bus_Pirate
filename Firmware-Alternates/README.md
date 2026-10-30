# Firmware-Alternates

The obsolete standalone projects in this directory have been removed.
Their replacements are features of the main firmware; no separate image
is needed for those functions.

## History

The following projects were removed (2026-10), as they are superseded:

- `firmware-STK500v2` — former standalone STK500v2/AVRISP serial
  programmer, based on Guido Socher's GPL implementation and ported to
  PIC24 by Ian Lesnet. Superseded on **BPv4** by the inlined AVRISP-MKII
  personality in the main firmware (ISP/PDI/TPI plus a CDC serial ISP
  route). The new personality is independently LUFA-derived; this is
  not a relicensing or direct incorporation of the removed GPL project.
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
