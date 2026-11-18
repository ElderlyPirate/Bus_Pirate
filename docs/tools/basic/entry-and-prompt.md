# BASIC — entry, prompt and program model

Part of the BASIC documentation set (`docs/tools/basic/`).  This page owns
entering the interpreter, the prompt, the line-entry and editing model, the
`RUN`/`LIST`/`NEW`/`EXIT` commands, and the lifetime of a stored program.  The
language is in [language.md](language.md); bus access is in
[bus-access.md](bus-access.md).

## Scope

How to get in and out of the interpreter and how a program is typed, stored,
listed and erased.  Nothing here covers what a statement does, the bus
statements or `MODE`, or the size limits in detail.

## Entering

`?` help lists the command as `s` with the label `Script engine`.  Type `s` at
any mode prompt and press Enter:

```
HiZ> s
HiZ(BASIC)>
```

- `s` does not change the active mode.  The interpreter runs in whatever mode
  was active when `s` was typed — `HiZ` in the example above.
- The prompt keeps the mode name and appends `(BASIC)` before `>`:
  `HiZ(BASIC)>`, `SPI(BASIC)>`, `I2C(BASIC)>`, and so on.
- `s` itself prints no acknowledgement.  The `(BASIC)` marker in the prompt is
  the only sign that you are in.  `Ready` is the interpreter's acknowledgement
  for each line it handles **once you are inside** — a stored line, `RUN`,
  `LIST` or `EXIT` — not for the `s` line that launched it.
- `s` does not run a mode setup and does not turn the supplies on.  If the
  mode's interactive setup is incomplete, either finish it at the terminal
  first (including `W` where the mode uses it) or let a `MODE` statement apply
  the mode's default setup and engage the bus.

Every line typed at a `<mode>(BASIC)>` prompt is handled by the interpreter,
not by the normal command set.  Typing `s` again, or `#`, `$`, `m`, `w` or
`?`, is a BASIC line, and most such lines answer `Syntax error`.

## The line model

The interpreter decides what a line means from its **first character**:

| First character | The line is |
|---|---|
| a digit `0`..`9` | a numbered program line — stored, replaced, or deleted |
| anything else | an interpreter command (`RUN`, `LIST`, `NEW`, `EXIT`) |

All input is folded to upper case before it is parsed, including text inside
quotes: `10 PRINT "hi"` prints `HI`.  Whitespace outside quotes and outside a
`REM` comment is dropped when the line is stored.

A line holds one statement.  `IF ... THEN` takes one statement, optionally
followed by `ELSE` and one more statement; that is the only two-statement form.

### Numbered lines

- `10 PRINT "HI"` stores line 10.  The terminal answers with `Ready`.
- Typing a line number that already exists **replaces** the old line, so a
  program is edited by retyping whole lines.
- Lines are kept sorted by number no matter in which order they are typed.
- A line number on its own, with no statement after it, **deletes** that line
  (for example `15`).  If the line does not exist, nothing happens.
- Line numbers are decimal, `0` to `65534`.  `65535` is reserved for the
  interpreter's end-of-program marker; typing it answers `Syntax error`.
- A line may hold up to 31 **statement bytes**.  A longer line is rejected with
  `Too long!`.  If you were replacing a line, the old line is **kept**; the new
  line is not stored.  Shorten the line and retype it.
- If the program area has no room for the line, the line is rejected with
  `Too long!` and the stored program is unchanged.  Delete lines or `NEW`.

A **statement byte** is one byte of the stored line.  A keyword is stored as a
single byte however it is spelled (`PRINT` is one byte, not five); every other
character — digit, punctuation or quoted text — counts as one byte.  Spaces
outside quotes and outside a `REM` are dropped before storage, so they do not
count; spaces inside a `REM` are kept and do count.

The interpreter does not echo a stored line beyond the `Ready`; the terminal
itself echoes what was typed.  While you are typing, the shared command-line
keys work as at any other prompt: Backspace, Delete, Left/Right/Home/End for
the cursor, and Up/Down to recall history.  There is no in-place editor for a
line that is already stored — retype it.

### Interpreter commands

These are not stored as program lines:

| Command | Action |
|---|---|
| `RUN` | execute the stored program |
| `LIST` | print the stored program |
| `NEW` | erase the stored program |
| `EXIT` | leave the BASIC interpreter and return to the normal prompt |

A command must be alone on the line (trailing spaces are allowed).  `NEWTON`,
`RUNAWAY`, `LISTING` or `EXITNOW` are not commands and answer `Syntax error`.
The command names are case-insensitive because the line is folded to upper
case.

`DEBUG` (a raw program dump) and the on-board EEPROM commands
`SAVE`/`LOAD`/`FORMAT` are not in the default image; see
[limits-and-variants.md](limits-and-variants.md).

### What `LIST` prints

`LIST` walks the stored program from the lowest line up and prints each line,
then a byte count:

```
HiZ(BASIC)> LIST

10  PRINT "HI"
20  END
65535  END
15 bytes.
Ready
```

- A stored keyword is printed with a space on either side, so a line number
  and its first keyword are separated by two spaces (`10  PRINT`).
- The blank line before the first listing is the line break `LIST` prints
  before the first stored line.
- The last printed line, `65535  END`, is the interpreter's own
  **end-of-program marker**; it is not a line you typed and cannot be deleted
  (typing `65535` is rejected).
- The number before `bytes.` is one less than the stored size.  Line 10 is
  8 bytes, line 20 is 4 bytes and the `65535  END` marker is 4, 16 in all;
  `LIST` prints `15` because the count stops at the terminating zero byte.
- An empty program lists only the marker and `3 bytes.`

## Running a program

`RUN` executes the stored program from its first line, clears all variables to
zero, and prints `Ready` when it finishes:

```
HiZ(BASIC)> RUN
HI

Ready
HiZ(BASIC)>
```

- A program that reaches `END`, or runs past its last line into the
  end-of-program marker, finishes quietly and prints `Ready`.
- A program that stops on an error prints an error line before `Ready`; see
  [language.md §Errors](language.md#errors).
- The stored program and the last variable values are kept after the run; the
  next `RUN` clears the variables again and starts from the top.
- **Ctrl-C** pressed while a program is running stops it between statements
  and prints `Error(10)`.  This is the way to break a runaway loop.  It is not
  serviced inside `INPUT`, `DELAY` or a mode macro: a program waiting there
  finishes that step first.  Other keys typed during a run are discarded.
- `RUN` does not reset the board.  A program that turns on `PSU`, enables
  `PULLUP`, drives `AUX` or starts PWM leaves the board in that state when it
  ends or errors.  Add cleanup statements, or reset the board afterwards.

## Program lifetime

A program lives in the interpreter's RAM; there is no persistence.

| Event | Stored program |
|---|---|
| `EXIT` | kept; type `s` again to resume |
| `NEW` | erased |
| changing mode at the terminal (`EXIT`, `m`, `s`) | kept; `RUN` starts from the top and its `MODE` statements run again |
| `#` on v4 (terminal reset) | kept in RAM; the BASIC flag is cleared, so type `s` again |
| SUMP exit, and the binary return on v4 | kept in RAM; the terminal reset exits BASIC |
| `#` on v3 (hardware reset) | cleared |
| binary return on v3 (hardware reset) | cleared |
| `$` (bootloader), power cycle | cleared |

Variables are cleared by the next `RUN`; `EXIT`, `NEW` and a terminal reset
leave them.

Changing mode is a terminal action: `EXIT`, change mode with `m` or `m<N>`,
then type `s` again.  The program is kept and `RUN` starts from the top.

## Leaving

`EXIT` returns to the normal prompt:

```
HiZ(BASIC)> EXIT
Ready
HiZ>
```

`EXIT` is the only command that leaves the interpreter at the BASIC prompt.  A
board reset also leaves it.

Two input sequences are handled by the terminal layer before the interpreter:

| Sequence | Effect |
|---|---|
| 20 consecutive NUL bytes | enter the binary bitbang layer |
| five to nineteen NULs, then the Left-arrow key (`0x02`) | enter the logic analyzer (SUMP) |

The two windows do not overlap: the twentieth NUL enters binary first, so a
longer NUL run never reaches SUMP and the next byte is a binary command (a
Left arrow there selects binary I2C mode).

Either one leaves BASIC and hands the board to that layer; see the
[generic commands page](../../generic/commands.md), the
[binary set](../../protocol/binary/README.md) and the
[SUMP set](../../protocol/sump/README.md).  They work at the BASIC prompt,
not during a `RUN`: while a program runs, the interpreter consumes any pending
byte except Ctrl-C.

## v3 / v4

Entry, prompt, line model, `RUN`/`LIST`/`NEW`/`EXIT` and the byte-count line
are the same in both builds.  The program-lifetime differences are in the
table above; the other build differences are in
[limits-and-variants.md](limits-and-variants.md).
