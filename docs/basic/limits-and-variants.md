# BASIC — limits & v3 / v4 variants

Part of the BASIC documentation set (`docs/basic/`).  This page owns the
memory and language limits and the differences between the two boards.

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
| `DATA` values | decimal numbers or variables; a variable is evaluated at `READ` time |
| PWM frequency (`FREQ`) | `0`..`4000` Hz (`0` = off) |
| PWM duty (`FREQ`/`DUTY`) | `1`..`100` % |
| `TIME` resolution | 64 µs per unit, wrapping about every 4.19 s |

The program area holds one program.  A program plus its line headers and end
marker must fit in the area; when no room is left, storing another line is
rejected with `Too long!`.

A **statement byte** is one byte of the stored line.  A keyword is stored as a
single byte however it is spelled (`PRINT` is one byte, not five); every other
character — digit, punctuation or quoted text — counts as one byte.  The
count the `LIST` command prints covers the whole program area in use,
including each line's three-byte header and the end marker.

## v3 / v4 differences

| Feature | v3 | v4 |
|---|---|---|
| interpreter (`s`) | present | present |
| language, statements, limits, errors | same | same |
| mode numbers used by `MODE` | no `KEYB`; `PIC`=8, `DIO`=9 | adds `KEYB`=8; `PIC`=9, `DIO`=10 |
| on-board EEPROM `SAVE`/`LOAD`/`FORMAT` | unsupported | build-time option, **off by default** |
| raw program `DEBUG` dump | build-time option, off by default | build-time option, off by default |

Notes:

- When enabled, `SAVE n` and `LOAD n` store and retrieve the program in
  on-board EEPROM slot `n`, and `FORMAT` erases that file system.  These are
  the only commands that would persist a program across a reset.  They are
  disabled in the default v4 build and are not available at all on v3, so in
  practice a program is RAM-only on both boards, and their use is out of scope
  for this set.
- `DEBUG` hex-dumps the raw program area; it is disabled in the default build
  and is a maintainer/inspection aid rather than a normal user command.
- The interpreters in the two builds share one source of behaviour; the v3
  column is stated from the build only and is **not verified** against
  hardware.