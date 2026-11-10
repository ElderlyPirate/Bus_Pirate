# BASIC (script engine)

**Status:** draft — the on-board BASIC interpreter; available on v3 and v4.
v3 is included in the build but not verified on hardware.

BASIC lets the Bus Pirate run a small program by itself, so a bus sequence
can be replayed without a PC driving every step.  A program lives in the
Bus Pirate's own RAM and is executed from there; nothing survives a reset or
power cycle.

`?` help calls `s` the **Script engine**; the prompt shows the interpreter as
`(BASIC)`.  Both names refer to the same thing and both come from the
firmware — this set uses **BASIC interpreter** for the feature and quotes the
exact on-screen strings where they matter.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-prompt.md](entry-and-prompt.md) | entering with `s`, the prompt, line entry/editing, `RUN`/`LIST`/`NEW`/`EXIT` |
| [language.md](language.md) | statements, expressions, operators, variables, error codes |
| [bus-access.md](bus-access.md) | `MODE`, mode numbers, terminal mapping, bus statements |
| [limits-and-variants.md](limits-and-variants.md) | limits, unavailable features, v3/v4 differences |
| [examples.md](examples.md) | worked sessions |
| [edge-cases.md](edge-cases.md) | concrete failure modes and quirks |

## The five-minute version

- Enter: type `s` at any prompt; the prompt becomes `<mode>(BASIC)>`.  Leave
  with `EXIT`.
- A line that starts with a number is stored as a program line; retyping the
  number replaces it; a number on its own deletes it.  `RUN` executes, `LIST`
  prints, `NEW` erases.
- Language: `LET`, `IF/THEN/ELSE`, `GOTO`, `GOSUB/RETURN`, `FOR/NEXT`,
  `PRINT`, `INPUT`, `DATA/READ/RESTORE`, `REM`, `END` — single statement per
  line.  `INPUT` needs the comma: `INPUT "prompt", A` or `INPUT , A`.
- Bus: `MODE n` switches to mode-menu number `n`.  Protocol operations:
  `START`/`STARTR`, `STOP`/`STOPR`, `SEND`, `MACRO`, plus the
  `RECEIVE`/`BITREAD` read tokens; bit-bang lines: `CLK`, `DAT`.
  Board-level: `PSU`, `PULLUP`, `AUX`,
  `AUXPIN`, `DELAY`, plus the `ADC` and `TIME` read tokens.
- Ctrl-C stops a running program between statements.
- Limits: 1024-byte program area; up to 31 statement bytes per line;
  variables `A`..`Z` (signed 16-bit); 4 nested `FOR` loops; 10 `GOSUB` frames.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| interpreter (`s`) | present (build-only) | present |
| mode numbers in `MODE` | includes PIC/DIO, no KEYB | adds KEYB (see [bus-access.md](bus-access.md)) |

`DEBUG` and the on-board EEPROM `SAVE`/`LOAD`/`FORMAT` commands are build-time
options that are off by default and do not build in the current tree — treat
them as unavailable (see [limits-and-variants.md](limits-and-variants.md)).
The interpreter itself and all usable language statements are the same in both
builds.

## Wiring

BASIC adds **no wiring of its own**.  The protocol statements drive the bus of
whichever mode is active, so their wiring, pull-ups and supply requirements are
that mode's.  Select the mode first at the terminal, or from inside the program
with `MODE`, and follow that mode's page:

| Mode | Documentation |
|---|---|
| 1-Wire | [1wire](../../protocol/wire/1wire/README.md) |
| UART | [UART](../../protocol/uart/README.md) |
| I2C | [I2C](../../protocol/i2c/README.md) |
| SPI | [SPI](../../protocol/spi/README.md) |
| 2-Wire | [2wire](../../protocol/wire/2wire/README.md) |
| 3-Wire | [3wire](../../protocol/wire/3wire/README.md) |
| KEYB | [KEYB](../../protocol/keyboard/README.md) |
| PIC | [PIC](../../programmers/pic/README.md) |
| DIO | [DIO](../../protocol/dio/README.md) |

The **board-level operations** are not the mode's bus.  `PSU` switches the
on-board regulator (target VCC on the VCC pin), `PULLUP` ties the I/O lines to
the **Vpullup** pin and needs a voltage there, `AUX`/`AUXPIN` drive or read the
selected AUX/CS pin, and the `ADC` and `TIME` read tokens sample the probe pin
and read the on-board timer.  These work even in HiZ.  See
[bus-access.md](bus-access.md) and the
[AUX](../../capabilities/auxiliary/README.md) and
[ADC](../../capabilities/adc/README.md) pages.

Shared prompt grammar (`?`, `i`, `m`, `v`, `e`, `L`/`l`, `;N` and so on) lives
in [the generic layer](../../generic/README.md) and is not repeated here.

## Limits & out of scope

- No string variables and no string expressions; string literals are allowed
  in `PRINT` and `INPUT`.
- No arrays and no user-defined functions.
- No built-in numeric functions such as `ABS`, `INT` or `RND`.
- No `STEP` clause on `FOR`; `NEXT` always names its loop variable.
- No multi-statement lines; a `THEN`/`ELSE` branch is a single statement.
- `PRINT` has one separator, `;`.  A `,` is not a separator: it prints `0`
  (`PRINT 1,2` prints `102`).
- No hexadecimal or binary literals inside a stored program (decimal only).
- There is no in-place editor for a stored line — you retype the whole line.
  The shared command-line keys (Backspace, Delete, Left/Right, Home/End, and
  Up/Down history) still work on the line you are typing.
- No binary sub-protocol and no host API of its own.  The terminal's 20-NUL
  binary entry and the SUMP entry sequence are handled outside the interpreter
  and leave it.
- The program area holds one program; there is no file system.
- `FREQ` and `DUTY` are present but not usable in the current interpreter; see
  [bus-access.md](bus-access.md).

## Related docs

| Doc | Why |
|---|---|
| [../../SPEC.md](../../SPEC.md) | function and capability index |
| [../../generic/README.md](../../generic/README.md) | prompt, mode menu, supply/pull-up basics, `e`/`L`/`;N` |
| [../../generic/commands.md](../../generic/commands.md) | terminal command reference; binary/SUMP entry |
| [bus-access.md](bus-access.md) | the `MODE` contract, the works-in matrices, the terminal mapping |
| [../../protocol/binary/README.md](../../protocol/binary/README.md) | the 20-NUL binary entry |
| [../../protocol/sump/README.md](../../protocol/sump/README.md) | the SUMP entry sequence |
| [../../capabilities/auxiliary/README.md](../../capabilities/auxiliary/README.md) | AUX targets; terminal PWM/servo |
| [../../capabilities/adc/README.md](../../capabilities/adc/README.md) | the ADC probe and its scale |
