# Binary bitbang layer (BBIO1)

**Status:** stub — page to be written (scriptable binary protocols).

The binary layer replaces the terminal with a byte-oriented protocol: entry is
20× `0x00` → `BBIO1`; sub-modes cover SPI, I2C, UART, 1-Wire and raw wire,
plus PIC, OpenOCD JTAG, XSVF, the AVRISP personality, self-test, PWM, ADC,
frequency, pin I/O and the `0x70` wait-for-edge command.  The full byte map
lives in `Firmware/binary_io.c` / `binary_io.h`.

- Source today: `Documentation/bitbang.md` (partial — migrate + fix: `0x07`
  PIC and `0x18` XSVF are missing there).
- Planned files: `README.md` (core + byte map), per-protocol details or
  cross-links into the mode pages.
- SPEC: `docs/SPEC.md` §6 (binary-layer rows).