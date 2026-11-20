# DIO — macros

Part of the DIO documentation set (`docs/protocol/dio/`).  This page covers
the two macro families as they apply to direct I/O.  The shared storage,
limits and recall grammar are owned by [macros.md](../../generic/macros.md).

## Two families

| Form | Family | In direct I/O |
|---|---|---|
| `<n>`, `<n=body>`, `<0>` | user macros | available |
| `(n)`, `(0)` | mode macros | **none** |

Direct I/O has no mode macros.  The round-bracket form is routed to the empty
handler, so `(0)` and `(n)` report an error and do nothing:

```
DIO>(0)
ERROR: command has no effect here

Syntax error at char 3
DIO>
```

The blank line between the error and the syntax line is added by the shared
macro command.

## User macros for pin sequences

The angle-bracket macros are the shared RAM slots.  They are useful in direct
I/O for replaying a pin sequence:

```
DIO><1=0xC4 0x1B>
DIO><1>

DIO>0xC4 0x1B
WRITE: 0xC4
WRITE: 0x1B
DIO>
```

`<1=…>` stores the body; it does not run it.  `<1>` recalls the body onto the
prompt line, where it is an ordinary command line: edit it if needed, then
press Enter.  The blank line after `<1>` is the macro command's line break
before it loads the body.  A body may hold several values separated by spaces
or commas; here `0xC4` loads `CLK` high with the supply on, then `0x1B` makes
only `CLK` an output.

The body is literal text.  It can contain any value or command that fits, but
there are no loops, conditions or named variables, and the first `>` ends the
assignment.  A wrapper, a bit op or `(n)` inside the body still reports the
same error it would report at the prompt.

## Limits and persistence

| Item | Rule |
|---|---|
| slots | 3 on v3, 5 on v4 (default build) |
| slot size | 24 bytes on v3, 32 on v4 |
| body length | about 22 characters on v3, 30 on v4 |
| recall | copies the body to the prompt; Enter submits it |
| storage | RAM; a power cycle clears every slot |
| `w` | does not clear or change the slots |
| `#` | keeps the slots on v4; the v3 reboot clears them |
| mode change | keeps the slots on both boards |

A missing `>` or an out-of-range slot is a syntax error and stores nothing.
The exact line limits and the list form are in
[macros.md](../../generic/macros.md); the list is not saved to EEPROM.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode macros `(n)` | none | none |
| user macro slots | 3 | 5 |
| user macro body | about 22 characters | about 30 characters |
| `#` | hardware reset; slots cleared | terminal reset; slots kept |
