# BASIC (script engine)

**Status:** draft — the on-board BASIC interpreter; available on v3 and v4;
v3 is build-only and unverified.

BASIC lets the Bus Pirate run a small program by itself, so a bus sequence
can be replayed without a PC driving every step.  A program lives in the
Bus Pirate's own RAM and is executed from there; in the default build nothing
survives a reset or power cycle.

`?` help calls `s` the **Script engine**; the prompt shows the interpreter as
`(BASIC)`.  Both names refer to the same thing and both come from the
firmware — this set uses **BASIC interpreter** for the feature and quotes the
exact on-screen strings where they matter.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-prompt.md](entry-and-prompt.md) | entering with `s`, the prompt, the line-entry/editing model, `RUN`/`LIST`/`NEW`/`EXIT` |
| [language.md](language.md) | statements, expressions, operators, variables, error codes |
| [bus-access.md](bus-access.md) | `MODE` and the mode numbers, and every bus statement |
| [limits-and-variants.md](limits-and-variants.md) | memory/language limits and the v3/v4 differences |
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
  line.
- Bus: `MODE n` switches to mode-menu number `n`; the bus statements are
  `START`/`STARTR`, `STOP`/`STOPR`, `SEND`, `RECEIVE`, `CLK`, `DAT`,
  `BITREAD`, `AUX`, `AUXPIN`, `PSU`, `PULLUP`, `DELAY`, `FREQ`, `DUTY`,
  `MACRO`, `ADC`, `TIME`.
- Ctrl-C stops a running program.
- Limits: 1024-byte program area; up to 31 statement bytes per line;
  variables `A`..`Z` (signed 16-bit); 4 nested `FOR` loops; 10 `GOSUB` frames.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| interpreter (`s`) | present (build-only here) | present |
| mode numbers in `MODE` | includes PIC/DIO, no KEYB | adds KEYB (see [bus-access.md](bus-access.md)) |
| on-board EEPROM `SAVE`/`LOAD`/`FORMAT` | — | build-time option, off by default |
| raw program `DEBUG` dump | build-time option, off by default | build-time option, off by default |

The interpreter itself and all language statements are the same in both
builds; v3 behaviour is **not verified**.

## Wiring

BASIC adds **no wiring of its own**: it drives the bus of whichever mode is
active, so the wiring, pull-ups and supply requirements are that mode's.
Select the mode first (or with `MODE`) and follow that mode's page — see
[the generic layer](../../generic/README.md) and `docs/<mode>/`.

## Limits & out of scope

- No strings, no string variables, no arrays, no user-defined functions.
- No built-in numeric functions such as `ABS`, `INT` or `RND`.
- No `STEP` clause on `FOR`; `NEXT` always names its loop variable.
- No multi-statement lines; a `THEN`/`ELSE` branch is a single statement.
- `PRINT` has one separator, `;`; a `,` is not a `PRINT` separator.
- No hexadecimal or binary literals inside a stored program (decimal only).
- There is no in-place editor for a stored line — you retype the whole line.
  The shared command-line keys (Backspace, Delete, Left/Right, Home/End, and
  Up/Down history) still work on the line you are typing.
- No binary sub-protocol and no host API — BASIC is terminal-only.
- The program area holds one program; there is no file system in the default
  build.

Shared prompt grammar (`?`, `i`, `m`, `v` and so on) lives in
[the generic layer](../../generic/README.md) and is not repeated here.