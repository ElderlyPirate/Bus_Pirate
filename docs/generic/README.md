# The prompt — shared terminal layer

**Status:** current · the shared command layer.  Examples use v4 output; v3
differences are noted inline and in the delta table in
[navigation.md](navigation.md#v3--v4-delta).  The v3 build is included; its
hardware behaviour is not verified.

This folder documents what you talk to before any bus mode is active: the
prompt, line editing, command grammar, value entry and display, macros, the
shared element commands, reset, and the bootloader entry.  Per-mode behaviour
belongs to the mode's own set; the capability matrix is [SPEC.md](../SPEC.md).

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [navigation.md](navigation.md) | prompt, line editing, history, `i`, `?`, the mode menu, entering and leaving a mode, terminal baud `b` |
| [values.md](values.md) | numeric input, `:` repeat, `;` width, output formats `o`, bit order, `=` / `\|` conversion |
| [macros.md](macros.md) | user macros `<n>` and mode macros `(n)` |
| [commands.md](commands.md) | wiring, element commands, AUX targets, wrappers and bit ops, HiZ rules, binary/SUMP entry |
| [usage.md](usage.md) | example sessions: first contact, mode change, wrappers and reads, `v`, reset, `$` |
| [edge-cases.md](edge-cases.md) | failure modes, warnings and known traps |

## Getting connected

- The Bus Pirate is a USB serial device.  A v4 presents a USB CDC virtual COM
  port.  A v3 presents an FTDI USB-serial port (`0403:6001`); the bridge is on
  the board on a standard v3, and an external adapter appears the same way.
- Open the port at **115200 baud, 8 data bits, no parity, 1 stop bit (8N1)**,
  no flow control.  On v4 the rate is host metadata: the firmware does not
  reconfigure the link.  On v3 the firmware boots at 115200 and `b` changes it.
- Press Enter: the prompt shows the current mode name and `>`.  At power-up
  that is `HiZ>`.
- Drivers, building and flashing are developer topics — see
  [../dev/README.md](../dev/README.md).

## The five-minute version

- `?` help · `i` version/status · `m` mode menu · `o` display format ·
  `h` history · `b` terminal baud (v3).
- Elements: `W`/`w` supplies · `P`/`p` pull-ups · `e` VPU source (v4) ·
  `a`/`A`/`@` AUX pin · `l`/`L` bit order · `v` volts and pin states.
- Values: `123`, `0x1F`, `0b101`, `"abc"`; `:N` repeats; `;N` sets the width.
- Wrappers `[` `{` `]` `}`; reads `r`; bit ops `/ \ ^ - _ . !`.
- `#` reset · `$` bootloader · `s` BASIC · `~` self-test (HiZ only).
- Most modes stay parked until `W` prints `Clutch engaged!!!`.

The base layer also covers the ADC (`d`/`D`), frequency (`f`), PWM and servo
(`g`/`S`), the elapsed timer (`t`/`T`) and the self-test (`~`).  Their wiring
and limits are on their own pages: [ADC](../capabilities/adc/README.md),
[AUX](../capabilities/auxiliary/README.md),
[self-test](../capabilities/selftest/README.md).

## Glossary

- **Clutch** — the bus-engagement metaphor.  "Clutch disengaged" means the
  mode's I/O is parked; `W` starts the supplies, runs the mode's bus setup and
  prints `Clutch engaged!!!`.  It does not repeat the setup prompts.  `w` parks
  it again.
- **Vpullup** — the pin marked `Vpu` on the reference card ("input pull-up
  resistors 0-5 V"); the `v` table calls the column `VPU`.  `P` ties the I/O
  lines to whatever voltage is on it.
- **AUX** — the auxiliary pin.  `a`/`A`/`@` act on the selected target;
  `c`/`C` and v4 `k`/`K` choose among `AUX`, `CS`, `AUX1`, `AUX2`.  The
  token-to-pin mapping is in
  [commands.md](commands.md#aux-targets-and-pin-names).
- **Width** — the number of bits per read or write, 8 by default and 1–16
  with `;N` ([values.md](values.md)).

## Idle mode (HiZ)

HiZ is the firmware's idle mode.  Every header I/O pin is an input
(high-impedance), the on-board supply is off, and the pull-up resistors are
off.  It is the mode at power-up and the state every board reset returns to;
the prompt reads `HiZ>`.  Nothing is driven except the selected AUX pin:
`a`/`A` can drive it from HiZ and `@` reads it.  This is the safe state to
leave the board in.

The self-test runs here only: `~`.  In any other mode it prints
`Self-test in HiZ mode only`; disconnect the target before running it.  HiZ has
no bus operations, so start, stop, read, write, clock and bit commands report
`ERROR: command has no effect here` and a syntax error (see
[edge-cases.md](edge-cases.md)).

`#` and `m1` both return to HiZ and reset the board state (pins to inputs,
pull-ups released, supplies off, AUX assignment back to `A0` — the `AUX` pin).
`#` also restores HEX and prints the banner; `m1` does neither and prints
`Ready`.  Both are unavailable inside BASIC: the interpreter receives every
line until `EXIT`.  The full reset description is in
[usage.md](usage.md#reset-and-the-idle-state).

## Modes, layers and where their docs live

The mode menu is dynamic: it lists only the modes compiled into the build and
numbers them in a fixed order.  The default v4 menu is 1 `HiZ` … 10 `DIO`; the
default v3 build omits `KEYB`, so `PIC` and `DIO` are one number lower.  The
`HiZ` through `DIO` rows below are the default menu modes; `LCD` joins the
menu when built in, and the rest are entered another way (binary NULs, `s`, a
USB personality, or base-layer commands).  `SMPS` is a binary capability, not
a menu mode.  The exact lists are in
[navigation.md](navigation.md).

| Mode or entry | v3 | v4 | Documentation |
|---|---|---|---|
| `HiZ` | yes | yes | this page, [Idle mode](#idle-mode-hiz) |
| `1WIRE` | yes | yes | [1-Wire](../protocol/wire/1wire/README.md) |
| `UART` | yes | yes | [UART](../protocol/uart/README.md) |
| `I2C` | yes | yes | [I2C](../protocol/i2c/README.md) |
| `SPI` | yes | yes | [SPI](../protocol/spi/README.md) |
| `2WIRE` | yes | yes | [2-Wire](../protocol/wire/2wire/README.md) |
| `3WIRE` | yes | yes | [3-Wire](../protocol/wire/3wire/README.md) |
| `KEYB` | — | yes | [KEYB](../protocol/keyboard/README.md) |
| `PIC` | yes | yes | [PIC](../programmers/pic/README.md) |
| `DIO` | yes | yes | [DIO](../protocol/dio/README.md) |
| `LCD` | optional | optional | [LCD](../capabilities/lcd/README.md) |
| `SMPS` (binary `0xF0`) | — | optional | [SMPS](../capabilities/smps/README.md) |
| Binary, SUMP | yes | yes | [binary](../protocol/binary/README.md) · [SUMP](../protocol/sump/README.md) |
| AVRISP-MKII | — | yes | [AVRISP](../programmers/avrisp/README.md) |
| JTAG, OpenOCD | binary only | binary only | [JTAG](../protocol/jtag/README.md) · [OpenOCD](../tools/openocd/README.md) |
| XSVF | — | binary only | [XSVF](../tools/xsvf/README.md) |
| BASIC | yes | yes | [BASIC](../tools/basic/README.md) |
| Self-test, ADC, AUX | yes | yes | [self-test](../capabilities/selftest/README.md) · [ADC](../capabilities/adc/README.md) · [AUX](../capabilities/auxiliary/README.md) |

**Name note —** the device prints the mode string `1WIRE` in the menu, banner
and prompt, while its bus messages say `1-WIRE speed: …`; this set writes
`1-Wire` in prose and quotes the device string in the terminal output.
`2WIRE` reuses the firmware's `I2C START BIT` / `I2C STOP BIT` strings for its
start and stop wrappers, and its direct-form settings header is prefixed
`R2W`; the prompt and menu still say `2WIRE`.

## Availability (summary)

| Aspect | v3 | v4 |
|---|---|---|
| terminal modes in the default build | 9 (no `KEYB`) | 10 |
| command history `h` | 7 entries | 15 entries |
| user macros `<n>` | 3 slots × 24 B | 5 slots × 32 B |
| terminal command buffer | 128 B | 256 B |
| VPU pull-up source `e` | — | present |
| terminal baud `b` | rate menu | one-line notice |
| reset `#` | hardware reset | terminal reset to HiZ + banner |
| `v` pin table | 10 columns | 12 columns |

The full per-board delta table is in [navigation.md](navigation.md).

## Wiring headline

- The prompt itself needs only the USB/serial link — no target wiring.
- `W` enables the on-board regulator and puts the 3.3 V and 5.0 V rails on
  their header pins; keep the target ground common.
- `P` connects the I/O lines to the **Vpullup** pin through the on-board
  pull-up resistors.  A 0–5 V source must be on that pin.  On v4, `e2`/`e3`
  route the on-board 3.3 V / 5.0 V rails to it, but only in a non-HiZ mode
  whose outputs are open drain; on v3 the pin must be fed externally.
- No pull-up is needed to use the prompt.  Per-command wiring is in
  [commands.md](commands.md).

## Limits & out of scope

- Per-mode settings defaults, per-mode `r` / `[` / `]` semantics, per-mode pin
  names and per-mode macros are **not** here — see the mode table above.
- No pull-**down** command exists.  `P` / `p` switch the pull-ups on and off;
  the output type (open drain or push-pull) is a per-mode setup answer.
- There is no line editing beyond the keys in [navigation.md](navigation.md):
  no kill-line and no tab completion.
- The binary protocol, SUMP command set, BASIC language and self-test checks
  live on their own pages.
