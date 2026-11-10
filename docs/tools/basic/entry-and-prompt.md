# BASIC — entry & prompt

Part of the BASIC documentation set (`docs/tools/basic/`).  This page owns
entering the interpreter, the prompt, and the line-entry/editing model.  The
language itself is in [language.md](language.md).

## Scope

How to get in and out of the interpreter and how programs are typed in,
stored, listed and erased.  Nothing here covers what a statement does
([language.md](language.md)), the bus statements or `MODE`
([bus-access.md](bus-access.md)), or the size limits
([limits-and-variants.md](limits-and-variants.md)).

## Entering

`?` help lists the command as `s Script engine`.  Type `s` at any mode prompt
and press Enter:

```
HiZ> s
HiZ(BASIC)>
```

- `s` does not change the active mode.  The interpreter runs in whatever mode
  was active when `s` was typed — `HiZ` in the example above.  Select the mode
  **first** at the terminal (with `m`), or switch later from inside the
  program with the `MODE` statement.
- The prompt keeps the mode name and appends `(BASIC)` before the `>`:
  `HiZ(BASIC)>`, `SPI(BASIC)>`, `I2C(BASIC)>`, and so on.
- Every line typed at the `(BASIC)>` prompt is handled by the interpreter,
  not by the normal command set.  Typing `s` again, `w`, `#`, `m` or `$` is
  not a prompt command here — it is a BASIC line, and most such lines answer
  `Syntax error`.
- The interpreter is not completely sealed off, though: the input layer still
  watches for the 20-NUL binary-entry sequence and the SUMP entry sequence
  (NUL bytes plus Left arrow).  Either one is handled before BASIC and leaves
  the interpreter; see §Leaving.

`s` itself prints no acknowledgement: the `(BASIC)>` prompt is the only sign
you are in.  The `Ready` line is the interpreter's acknowledgement for each
line it handles **once you are inside** — a stored line, `RUN`, `LIST` or
`EXIT` — not for the `s` line that launched it.

## The prompt and the line model

Terminal input at `(BASIC)>` is one line at a time.  The interpreter decides
what a line means from its **first character**:

| First character | The line is |
|---|---|
| a digit `0`..`9` | a numbered program line — stored, replaced, or deleted |
| anything else | an interpreter command (`RUN`, `LIST`, `NEW`, `EXIT`) |

### Numbered lines

- `10 PRINT "HI"` stores line 10.  The terminal answers with `Ready`.
- Typing a line number that already exists **replaces** the old line, so a
  program is edited by retyping whole lines.
- Lines are kept sorted by number no matter in which order they are typed.
- A line number on its own, with no statement after it, **deletes** that line
  (for example `15`).
- All input is folded to upper case **before** it is parsed, including text
  inside quotes: `10 PRINT "hi"` prints `HI`.
- One line holds a single statement.  `IF ... THEN` takes one statement,
  optionally followed by `ELSE` and one more statement.
- A line may hold up to 31 **statement bytes**; a longer line is rejected
  with `Too long!` and the new line is not stored.  If you were replacing an
  existing line, that old line is removed too, so shorten the line and retype
  it.  A statement byte is one byte of the stored line: a keyword is stored as
  a single byte however it is spelled (`PRINT` is one byte, not five), and
  every other character — digit, punctuation or quoted text — counts as one
  byte.

The interpreter does not echo a stored line beyond the `Ready`; the terminal
itself echoes what was typed.

While you are typing a line, the shared command-line keys work exactly as at
any other prompt: Backspace, Delete, Left/Right/Home/End for the cursor, and
Up/Down to recall history.  Only a line already stored is edited by retyping
it — the interpreter has no in-place editor for stored lines.

### Interpreter commands

These are not stored as program lines:

| Command | Action |
|---|---|
| `RUN` | execute the stored program |
| `LIST` | print the stored program |
| `NEW` | erase the stored program |
| `EXIT` | leave the BASIC interpreter and return to the normal prompt |

`DEBUG` (a raw program dump) and the on-board EEPROM commands
`SAVE`/`LOAD`/`FORMAT` are build-time options that are off by default and do
not build in the current tree.  Treat them as unavailable; see
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
  **end-of-program marker**; it is not a line you typed.
- The number before `bytes.` is the number of stored bytes before the final
  terminator.  The sample's `15` is a 16-byte program: two stored lines plus
  the four-byte `65535 END` marker.

### Running a program

`RUN` executes the stored program from its first line and prints `Ready` when
it finishes:

```
HiZ(BASIC)> RUN
HI

Ready
HiZ(BASIC)>
```

- Variables are cleared at the start of each `RUN`.
- A program that reaches `END`, or runs past its last line, finishes quietly
  and prints `Ready`.
- A program that stops on an error prints an error line before `Ready` — see
  [language.md](language.md) §Errors.
- **Ctrl-C** pressed while a program is running stops it between statements
  and prints an interrupted-status error line.  This is the way to break a
  runaway loop, because the interpreter otherwise ignores the terminal until
  the program ends.  It is not serviced inside `INPUT`, `DELAY` or a mode
  macro: a program waiting there finishes that step before the interruption
  takes effect.

## Program lifetime

- Programs and variables live in RAM only.  A board reset or power cycle
  clears them; on power-up the interpreter holds an empty program.
- `NEW` erases the stored program, leaving the interpreter with an empty
  program.
- `EXIT` leaves the interpreter but **keeps** the program; type `s` again to
  resume with the same program in memory.
- Changing mode is a terminal action: `EXIT`, change mode with `m`, then type
  `s` again.  The program is kept, and `RUN` starts from the top (its `MODE`
  statements, if any, run again).

## Leaving

`EXIT` returns to the normal prompt:

```
HiZ(BASIC)> EXIT
Ready
HiZ>
```

`Ready` is the normal end-of-line acknowledgement; after it the prompt is the
ordinary mode prompt again.

`EXIT` is the only command that leaves the interpreter at the `(BASIC)>`
prompt.  A board reset also leaves it.  `$` (bootloader), `#`, `w`, `m` and
`s` are not interpreted as commands while the `(BASIC)>` prompt is up — they
are BASIC lines and answer `Syntax error`.

Two input sequences are handled by the terminal layer before the interpreter:
20 consecutive NUL bytes enter the binary bitbang layer, and five or more NUL
bytes followed by the Left-arrow byte enter the logic-analyzer (SUMP) mode.
Either leaves BASIC and resets the board state; see
[the generic commands page](../../generic/commands.md), the
[binary set](../../protocol/binary/README.md) and the
[SUMP set](../../protocol/sump/README.md).

## v3 / v4

Entry, prompt, line model, `RUN`/`LIST`/`NEW`/`EXIT` and the byte-count line
are the same in both builds.  The build differences are tabulated in
[limits-and-variants.md](limits-and-variants.md).
