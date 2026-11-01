# SMPS (switched-mode power supply board)

**Status:** stub — write from code only.  The external SMPS board and its 2010
documentation are **not available on this bench**; treat everything as
`Requires verification` unless confirmed in source.

Drives a legacy Dangerous Prototypes SMPS add-on board connected to a **BPv4**
via binary I/O commands (command group `0b1111xxxx`):

| Command | Meaning |
|---|---|
| `0xF0` | get the last output-voltage reading (big-endian 16-bit) |
| `0xF1` | stop the board |
| `0xF2` | start — the requested output voltage follows |

The firmware sets a PWM duty cycle and reads the output voltage back via ADC.
Not built in the default images (`BP_ENABLE_SMPS_SUPPORT` is off; the code is
additionally `#ifdef BUSPIRATEV4`).  The configuration comment notes that the
2010 board's compatibility with the current code is unclear.

- Source today: none — `Firmware/smps.c` / `smps.h`, `Firmware/binary_io.c`
  (`:88`, `:137`–`:140`, `:1320`–`:1348`, verified).
- Planned: `README.md` (protocol, commands, known limits).
- SPEC: `docs/SPEC.md` §2 (SMPS row).