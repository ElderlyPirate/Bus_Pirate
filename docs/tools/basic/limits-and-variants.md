# BASIC — limits & v3 / v4 variants

Part of the BASIC documentation set (`docs/tools/basic/`).  This page owns the
memory and language limits, the features that are not available in the current
build, and the differences between the two boards.

## Scope

How much a program can hold and how the two builds differ.  Failure behaviour
when a limit is hit is in [edge-cases.md](edge-cases.md).

## Limits

The values below are those of the default build.  The program area and the
loop/stack depths are build-time settings, so a custom build can differ.

| Item | Default |
|---|---|
| program area | 1024 bytes |
| statement bytes per line | up to 31 (a longer line is rejected) |
| variables | `A`..`Z`, 26 signed 16-bit values |
| nested `FOR` loops | 4 |
| `GOSUB` call depth | 10 |
| line numbers | decimal; `65535` is reserved for the end-of-program marker |
| `INPUT` value range | `0`..`32767` |
| `DATA` values | non-negative decimal numbers or variables; a variable is evaluated at `READ` time |
| `TIME` resolution | 64 µs per unit, wrapping about every 4.19 s |

The program area holds one program.  A program plus its line headers and end
marker must fit in the area; when no room is left, storing another line is
rejected with `Too long!`.  If the rejected line was a replacement, the old
line is removed as well.

A **statement byte** is one byte of the stored line.  A keyword is stored as a
single byte however it is spelled (`PRINT` is one byte, not five); every other
character — digit, punctuation or quoted text — counts as one byte.  The
count the `LIST` command prints is the number of bytes in use before the final
terminator: the sample `15 bytes.` is a 16-byte program (two stored lines plus
the four-byte `65535 END` marker).

## Not available in this build

| Feature | Status |
|---|---|
| `FREQ hz duty`, `DUTY duty` | not usable; the argument parser is broken and the program stops with an error.  Use the terminal `g`. |
| `DEBUG` | build-time option; off and does not build |
| on-board EEPROM `SAVE`/`LOAD`/`FORMAT` | v4 build-time option; off and does not build |

`DEBUG` dumps the raw program area.  The EEPROM commands were intended to
store and retrieve a program in an on-board EEPROM slot.  Both options are
disabled by default and enabling either one fails to compile in the current
tree, so there is no program persistence and no raw dump on either board.
Treat them as unavailable.

## v3 / v4 differences

| Feature | v3 | v4 |
|---|---|---|
| interpreter (`s`) | present | present |
| language, statements, limits, errors | same | same |
| mode numbers used by `MODE` | no `KEYB`; `PIC`=8, `DIO`=9 | adds `KEYB`=8; `PIC`=9, `DIO`=10 |
| on-board VPU source `e` | not available | select it at the terminal; `MODE` clears it |

The interpreters in the two builds share one source of behaviour; the v3
column is stated from the build only and is **not verified** against hardware.
