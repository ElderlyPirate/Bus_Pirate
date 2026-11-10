# Using the prompt — the generic (base) terminal layer

**Applies to** v4 (documented) and v3 (build-only, not hardware-verified).

This folder documents the shared CLI that every mode leans into: the prompt,
command grammar, the base-layer controls, and the entry sequences into the
binary modes.  The full capability matrix — which board supports what — is in
[../SPEC.md](../SPEC.md).

## Getting connected

- The Bus Pirate is a USB serial device.  A v4 presents a USB CDC virtual COM
  port; a v3 presents an FTDI USB-serial port (`0403:6001`) — the FTDI bridge
  is on the board on a standard v3, and an external FTDI adapter appears the
  same way.
- Open the port at **115200 baud, 8 data bits, no parity, 1 stop bit (8N1)**,
  no flow control.  On v4 the CDC rate is host metadata: the firmware does
  not reconfigure the link.  On v3 the firmware boots at 115200.
- Open the port and press Enter: the default prompt is `HiZ>`.  Commands end
  with Enter.
- Drivers, building and flashing are developer topics — see
  [../dev/README.md](../dev/README.md).

## Where the mode docs live

The generic layer is shared; per-mode behaviour lives in the mode's own set:

| Mode / function | Documentation |
|---|---|
| HiZ (idle) | the [Idle mode (HiZ)](#idle-mode-hiz) section below |
| 1-Wire, 2-Wire, 3-Wire | [1wire](../protocol/wire/1wire/README.md) · [2wire](../protocol/wire/2wire/README.md) · [3wire](../protocol/wire/3wire/README.md) |
| UART, I2C, SPI | [UART](../protocol/uart/README.md) · [I2C](../protocol/i2c/README.md) · [SPI](../protocol/spi/README.md) |
| KEYB, DIO | [KEYB](../protocol/keyboard/README.md) · [DIO](../protocol/dio/README.md) |
| PIC, AVRISP-MKII | [PIC](../programmers/pic/README.md) · [AVRISP](../programmers/avrisp/README.md) |
| JTAG, OpenOCD, XSVF | [JTAG](../protocol/jtag/README.md) · [OpenOCD](../tools/openocd/README.md) · [XSVF](../tools/xsvf/README.md) |
| Binary bitbang, SUMP | [binary](../protocol/binary/README.md) · [SUMP](../protocol/sump/README.md) |
| BASIC | [BASIC](../tools/basic/README.md) |
| LCD, SMPS | [LCD](../capabilities/lcd/README.md) · [SMPS](../capabilities/smps/README.md) |
| Self-test, ADC, AUX pin | [self-test](../capabilities/selftest/README.md) · [ADC](../capabilities/adc/README.md) · [AUX](../capabilities/auxiliary/README.md) |

## Glossary

- **Clutch** — the bus-engagement metaphor.  "Clutch disengaged" means the bus
  is parked: after a mode change the new mode is inert and its I/O is not driven
  until the supplies are started with `W`, which prints "Clutch engaged!!!".
  `w` (supplies off) parks the bus again.
- **VPU** — the pull-up supply.  The pull-up resistors tie the I/O lines to
  whatever voltage is present on the physical **Vpullup** pin (the pin marked
  `Vpu`, "input pull-up resistors 0-5 V", on the reference card).  On v4 the `e`
  command can select that source (external/none, on-board 3.3 V, on-board
  5.0 V); v3 has no `e`, so the Vpullup pin must be fed externally — see
  [commands.md](commands.md) §Wiring.
- **AUX** — the auxiliary I/O pin: `a` drives it low, `A` high, `@` reads it.
  On v4 it can be reassigned among `A0` / `CS` / `A1` / `A2` (`c`/`C`/`k`/`K`;
  the target pins are shown as `AUX` / `CS` / `AUX1` / `AUX2`).

This README is a **map, not the manual** — every detail lives in the files
below:

| File | Covers |
|---|---|
| [navigation.md](navigation.md) | prompt lifecycle, line editing, `i`, `?`, `m`, `b`, `o`; the v3/v4 delta table |
| [commands.md](commands.md) | wiring requirements, the complete reference tables (values, elements, macros, wrappers, bit ops, firmware-level), and leaving the CLI for binary/SUMP |
| [usage.md](usage.md) | worked terminal sessions (mode change, `W`/`P`, `v`, `$`, reset, wrappers) |
| [edge-cases.md](edge-cases.md) | failure modes, warnings and known quirks |

## The five-minute version

- The prompt shows the current mode (`HiZ>`, `UART>`, …); commands end with
  Enter.
- `?` help · `i` version/status · `m` mode menu · `o` display format ·
  `b` baud (v3; on v4 the CDC rate is host-controlled).
- Elements: `W`/`w` supplies · `P`/`p` pull-ups · `a`/`A`/`@` AUX pin ·
  `l`/`L` bit order · `v` volts/pin states.
- Macros: `<n>` user, `(n)` mode.  Wrappers `[` `{` `]` `}`; reads `r`; reads
  can take a one-shot display letter (`rx`/`rd`/`rb`/`rw`); bit ops
  `/ \ ^ - _ . !`.
- `#` reset · `$` bootloader · `s` BASIC · `~` self-test (HiZ only).

**Pull-up note:** `P` only pulls the I/O lines up if a voltage is on the
Vpullup pin; without one it still enables the resistors and prints
`Warning: no voltage on Vpullup pin`.  On v4, enter a non-HiZ mode first
(e.g. `m3`), choose open drain at its output prompt, then run `e2`/`e3`, then
`P`; on v3 feed the pin externally.  `e` is refused in HiZ, so the source
cannot be selected from the idle prompt.  See
[commands.md](commands.md) §Feeding the VPU pin.

A first session (abridged — the full transcript is in [usage.md](usage.md)).
After `m3` and its setup prompts, the bus commands are:

```
UART>W
POWER SUPPLIES ON
Clutch engaged!!!
UART>P
Pull-up resistors ON
Warning: no voltage on Vpullup pin
UART>o1
Display format set
UART>v
Pinstates:
...
UART>
```

`W` engages the bus, `P` enables the pull-ups (and warns when nothing is on
Vpullup), `o1` selects HEX, and `v` prints the pin/rail table.

## Idle mode (HiZ)

HiZ is the firmware's idle/default mode.  Every header I/O pin is an input
(high-impedance), the on-board supply is off, and the pull-up resistors are
off.  It is the mode at power-up and the state every reset returns to; the
prompt reads `HiZ>`.  HiZ has no bus operations, so start, stop, read, write,
clock and bit commands report `ERROR: command has no effect here` (see
[edge-cases.md](edge-cases.md)).

- **Nothing is driven** except the selected AUX pin: `a`/`A` can drive it from
  HiZ and `@` reads it.  This is the safe state to leave the board in.
- **The self-test runs here only:** `~`; in any other mode it prints
  `Self-test in HiZ mode only`.  Disconnect the target before running it.

`#` and `m1` both return to HiZ and reset the board state (pins to inputs,
pull-ups released, supplies off, AUX assignment back to `A0`).  `#`
additionally restores the HEX display format, leaves BASIC and prints the
banner; `m1` does none of those and prints `Ready`.  The authoritative reset
description, including the binary `0x0F` return and the SUMP exit, is in
[usage.md](usage.md#reset-and-the-idle-state).

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| command history `h` | 7 entries | 15 entries |
| user macros `<n>` | 3 slots × 24 B | 5 slots × 32 B |
| terminal command buffer | 128 B | 256 B |
| VPU pull-up source `e` | — | present |

Full delta table (banner, help text, pin table, reset): [navigation.md](navigation.md).

## Limits & out of scope

- Per-mode settings defaults, per-mode `r`/`[`/`]` semantics, per-mode pin
  tables and per-mode macros are **not** here — see the mode table above.
- No pull-**down** command exists — the pull-up elements are on/off only and
  the output matrix offers open-drain or push-pull (a per-mode setup prompt),
  never a pull-down.
- No command-line editing beyond the keys in [navigation.md](navigation.md)
  (no word-delete, no tab completion).
- BASIC internals, the binary protocol, SUMP and self-test detail live on
  their own pages: [../tools/basic/](../tools/basic/README.md),
  [../protocol/binary/](../protocol/binary/README.md),
  [../protocol/sump/](../protocol/sump/README.md),
  [../capabilities/selftest/](../capabilities/selftest/README.md).
- The `o` help label (`Set output type`) is a firmware string misnomer: `o`
  selects the display format, not the per-mode output type.
