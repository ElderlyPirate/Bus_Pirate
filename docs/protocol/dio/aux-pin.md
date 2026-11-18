# DIO — the AUX-target commands

Part of the DIO documentation set (`docs/protocol/dio/`).  This page owns the
per-pin path: `a`, `A` and `@`, and the `c` / `C` (v4 `k` / `K`) target
selection.  The numeric direction/state values are in
[pin-control.md](pin-control.md).

## Two ways to a pin

Direct I/O has two independent ways to change a pin:

| Path | Reaches | Sets direction | Sets level | Touches supply / pull-ups |
|---|---|---|---|---|
| numeric value | the five signal pins | all five at once | all five at once | yes, a state value does |
| `a` / `A` / `@` | one selected AUX target | that pin only | that pin only | no |

The second path is the shared AUX-pin command set.  It is available in every
terminal mode, `HiZ` included.  In direct I/O it is the only way to reach
`AUX1` and `AUX2`, and it can drive one pin without rewriting the others.

## Selecting the target

`a` / `A` / `@` act on one selected target.  The selection is made with a
command, not with a value:

| Command | Confirmation line | Pin | `?`-help token |
|---|---|---|---|
| `c` | `a/A/@ controls AUX pin` | `AUX` | `A0` on v4, `aux` on v3 |
| `C` | `a/A/@ controls CS pin` | `CS` | `CS` |
| `k` (v4) | `a/A/@ controls AUX1 pin` | `AUX1` | `A1` |
| `K` (v4) | `a/A/@ controls AUX2 pin` | `AUX2` | `A2` |

On v4 the `?` help lists `A0` / `CS` / `A1` / `A2`; on v3 it prints `aux` for
`A0` and lists only `aux` / `CS`.  The confirmation line and the `v` table
use the pin names.  Both name the same pins, and this set uses the pin names
throughout; [generic/commands.md](../../generic/commands.md#aux-targets-and-pin-names)
owns the full mapping.  The default is `AUX` (`A0`), applied at mode entry and
by every board reset.  The selection is a mode setting: `w` does **not**
change it, while `#` and a mode change return it to `AUX`.

On v3 only `c` and `C` exist; `k` and `K` are syntax errors.

### Which target is selected?

Run `i`.  The status block prints `a/A/@ controls <pin> pin`, for example
`a/A/@ controls AUX1 pin`.  `v` shows the pin states, not the selection.  Do
not use `c` / `C` / `k` / `K` just to check them, because issuing a selection
command changes it.

## Driving and reading one pin

| Input | Action | Reply |
|---|---|---|
| `a` | selected target output low | `AUX LOW` |
| `A` | selected target output high | `AUX HIGH` |
| `@` | selected target input, then read | `AUX INPUT/HI-Z, READ: 1` (or `0`) |
| `@:4` | four samples | four reply lines |

The drive and read replies always say `AUX`, even when the selected target is
`CS`, `AUX1` or `AUX2`; the confirmation line from `c` / `C` / `k` / `K` is the
record of the real target.

The three commands affect only the selected pin: they do not change the other
pins, the supply or the pull-ups.  For `AUX` and `CS`, they update the same
direction and latch registers the numeric values use, so `r` and `v` both show
the result; for `AUX1` and `AUX2`, only `v` shows it, because `r` reads the
five signal pins.

## Worked session

Drive `CS` as the selected target, then return to `AUX`:

```
DIO>C
a/A/@ controls CS pin
DIO>A
AUX HIGH
DIO>a
AUX LOW
DIO>c
a/A/@ controls AUX pin
DIO>
```

With the target holding `AUX` high, `@` reads it and leaves the pin
as an input:

```
DIO>@
AUX INPUT/HI-Z, READ: 1
DIO>
```

`@` changes the pin to an input before sampling.  A pin that was just driven
by `A` is therefore released before the read; with nothing connected the
result floats.  Use `@` to read a level driven by the target, or to release
one pin.  To keep a pin driven, send `A` or `a` again afterwards.

On v4, `AUX1` and `AUX2` use the same commands:

```
DIO>k
a/A/@ controls AUX1 pin
DIO>A
AUX HIGH
DIO>K
a/A/@ controls AUX2 pin
DIO>a
AUX LOW
DIO>
```

`AUX1` stays driven high after the `K` / `a` sequence, because selecting
`AUX2` does not release the previous target.

## Interaction with the numeric values

The numeric bit map always targets the **pin**, not the selected AUX target:

| Command path | Pin affected |
|---|---|
| numeric bit 4 | `AUX` (`A0`), regardless of `c` / `C` / `k` / `K` |
| numeric bit 0 | `CS`, regardless of the selection |
| `a` / `A` / `@` after `C` | `CS` |
| `a` / `A` / `@` after `K` | `AUX2` |

A later direction value can override the direction set by `a` / `A` / `@` for
`AUX` or `CS`, and vice versa.  A numeric value cannot reach `AUX1` or
`AUX2`.  They are two ways to write the same registers for the five signal
pins, not two separate pin states.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `c` / `C` targets | `AUX`, `CS` | `AUX`, `CS` |
| `k` / `K` targets | — | `AUX1`, `AUX2` |
| confirmation lines | `AUX`, `CS` | `AUX`, `CS`, `AUX1`, `AUX2` |
| `@` | same | same |
| `w` releases a driven `AUX` / `CS` target | yes | yes |
| `w` releases a driven `AUX1` / `AUX2` target | — | no |
