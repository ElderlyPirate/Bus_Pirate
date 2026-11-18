# Macros — user macros and mode macros

Part of the generic-layer documentation set (`docs/generic/`).  This page owns
the two macro families: the user macros stored in RAM and the macros a mode
provides.  The value and command grammar is in [values.md](values.md) and
[commands.md](commands.md).

## Two families

| Form | Family | Owner |
|---|---|---|
| `<n>`, `<n=body>`, `<0>` | user macros | the shared prompt |
| `(n)`, `(0)` | mode macros | the active mode |

The angle-bracket family is yours; the round-bracket family belongs to the
mode.  The `?` help lists them as `<x>/<x= >/<0>  Usermacro x/assign x/list all`
and `(x)  Macro x`, with `(0)  List current macros`.

## User macros

| Input | Effect |
|---|---|
| `<0>` | list every slot, including empty ones |
| `<n>` | copy slot n onto the prompt line |
| `<n=body>` | store `body` in slot n; the first `>` ends the body |

Slots are 1-based.  The default build has 3 slots on v3 and 5 on v4; each slot
holds 24 bytes on v3 and 32 on v4.  The whole `<n=…>` command must stay under
27 characters (v3) or 35 (v4), so the practical body is at most about
**22 characters (v3)** or **30 characters (v4)** with a one-digit slot.  A
longer line, a missing `>` or an out-of-range slot is a syntax error and
nothing is stored.

Assignment is literal: spaces are kept, and the body is stored exactly as
typed between `=` and the first `>`.  Assignment does not run anything.

`<n>` does **not** run the macro immediately.  It prints a newline, reprints the
prompt and types the stored body after it; the body is now an ordinary command
line that you can edit and must submit with Enter:

```
UART><1=0x55:3>
UART><0>
1. <0x55:3>
2. <>
3. <>
4. <>
5. <>
UART><1>

UART>0x55:3
WRITE: 0x55 0x55 0x55
UART>
```

Because the body is replayed as typed text, it may contain any command or
value syntax — separators, wrappers, another macro call.  It has no quoting
rules of its own, so a `>` in the body would have ended the assignment.

Macros live in RAM.  On v4 they survive `#`, a mode change, the binary return
and a SUMP session; a power cycle clears them.  On v3 a hardware reset (`#` or
the binary return) clears them, while a mode change or a SUMP exit keeps them.
The macro list is not saved to EEPROM.

## Mode macros

`(0)` lists the active mode's macros; `(n)` runs macro n.  The mode owns the
list and the meaning of each number, so read its page for the actual entries.
The shared layer only parses the number and the closing `)`:

| Input | Effect |
|---|---|
| `(0)` | the mode prints its macro menu |
| `(n)` | the mode runs macro n |
| `(1 )` | whitespace before `)` is accepted |
| `(1x)` or a missing `)` | syntax error |

A mode with no macros at all — HiZ, 3WIRE and DIO in the default build —
sends `(0)` or `(n)` to a null handler: the reply is
`ERROR: command has no effect here` and the line ends with a syntax error.  In
HiZ the `(0)` form also leaves a blank line between the error and the syntax
line (see [edge-cases.md](edge-cases.md)).

Mode macros can be lost or reset by a mode change, exactly like any other mode
setting.

## Related docs

- [commands.md](commands.md) — the command table entries.
- [values.md](values.md) — numbers and separators inside a macro body.
- [edge-cases.md](edge-cases.md) — macro traps and recovery.
- The mode's page for the `(n)` list.
