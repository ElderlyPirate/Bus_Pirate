# BASIC — entry & prompt

Part of the BASIC documentation set (`docs/basic/`).  This page owns entering
the interpreter, the prompt, and the line-entry/editing model.  The language
itself is in [language.md](language.md).

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
  was active when `s` was typed — `HiZ` in the example above.  Select the
  mode **first** (with `m`/`MODE`), or switch later from inside the program
  with the `MODE` statement.
- The prompt keeps the mode name and appends `(BASIC)` before the `>`:
  `HiZ(BASIC)>`, `SPI(BASIC)>`, `I2C(BASIC)>`, and so on.
- Every line typed at the `(BASIC)>` prompt is handled by the interpreter,
  not by the normal command set.  Typing `s` again, or `w`, or `#`, is not a
  prompt command here — it is a BASIC line, and most such lines answer
  `Syntax error`.

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
  with `Too long!` and is not stored.  A statement byte is one byte of the
  stored line: a keyword is stored as a single byte however it is spelled
  (`PRINT` is one byte, not five), and every other character — digit,
  punctuation or quoted text — counts as one byte.

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
| `DEBUG` | hex-dump the program area (only in a build that enables it; off by default) |

`FORMAT`, `SAVE` and `LOAD` exist only in a v4 build that enables the
interpreter's on-board EEPROM file system; they are off by default and
unsupported on v3, so their use is out of scope here — see
[limits-and-variants.md](limits-and-variants.md) for their syntax.

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
- The number before `bytes.` is how much of the program area the stored
  program uses.

### Running a program

`RUN` executes the stored program from its first line.  Each interpreter line
is followed by `Ready`:

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
  the program ends.

## Program lifetime

- Programs and variables live in RAM only.  A board reset or power cycle
  clears them; on power-up the interpreter holds an empty program.
- `NEW` erases the stored program, leaving the interpreter with an empty
  program.
- `EXIT` leaves the interpreter but **keeps** the program; type `s` again to
  resume with the same program in memory.
- Changing mode with the normal `m` command does not clear a stored program;
  a program loaded with `RUN` in one mode can be run again after a mode
  change (its `MODE` statements, if any, run on each `RUN`).

## Leaving

`EXIT` returns to the normal prompt:

```
HiZ(BASIC)> EXIT
Ready
HiZ>
```

`Ready` is the normal end-of-line acknowledgement; after it the prompt is the
ordinary mode prompt again.  A board reset also leaves the interpreter.

## v3 / v4

Entry, prompt, line model, `RUN`/`LIST`/`NEW`/`EXIT` and the byte-count line
are the same in both builds.  The differences that touch this page — mode
numbers used by `MODE`, the default-off `DEBUG` dump, and the v4-only EEPROM
file system — are tabulated in
[limits-and-variants.md](limits-and-variants.md).  v3 behaviour is **not
verified**.