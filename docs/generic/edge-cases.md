# Edge cases & failure modes — base layer

Part of the generic-layer documentation set (`docs/generic/`).  Each entry is
concrete: trigger → observable symptom → behaviour → recovery.  Long
explanations live in [commands.md](commands.md) and [usage.md](usage.md).

## Idle mode (HiZ)

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `W`, `w`, `P`, `p`, `g` or `S` in HiZ | `Command not used in this mode` | refused, no action; switch to a bus mode with `m`, then use the command (`P`/`g`/`S` work as soon as the mode is entered, before `W`) |
| `e` (v4, any form) in HiZ | `Command not used in this mode` then `Syntax error at char 1` | refused; `e` needs a non-HiZ mode with open-drain pins and no voltage on VPU |
| `e` on v3 | `Syntax error at char 1` | there is no `e` command on v3 |
| `O` outside 1-Wire | `Command not used in this mode` | `O` is the 1-Wire speed toggle; it is refused in HiZ and in every other non-1-Wire mode |
| `~` in any non-HiZ mode | `Self-test in HiZ mode only` | refused; return to HiZ (`m1` or `#`) and run `~` again |
| a bus operation in HiZ (`r`, `[`, `]`, `^`, `!`, a value, `(n)`) | `ERROR: command has no effect here` plus a syntax error | nothing is sent; use a bus mode for bus traffic |
| `a` / `A` in HiZ | `AUX LOW` / `AUX HIGH`, and the selected AUX pin is driven | expected; `@` returns it to input, and `m1` / `#` re-impedes `AUX` and `CS` (a driven `AUX1` / `AUX2` keeps its direction and level — see [AUX](../capabilities/auxiliary/README.md)) |
| `v` in HiZ with nothing connected | direction `I` on every I/O pin, with a mix of `H`/`L` states | floating inputs, not driven levels — see [usage.md](usage.md#v-in-hiz) |

## Power and pull-ups

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `P` with no pull-up voltage | `Pull-up resistors ON` then `Warning: no voltage on Vpullup pin` | pull-ups stay enabled; supply Vpullup (`e2`/`e3` in a non-HiZ open-drain mode on v4, external on v3) |
| `P` in push-pull mode | `WARNING: pins not open drain (HiZ)` before enabling | informational; choose open drain in the mode setup — [DIO](../protocol/dio/README.md) has no output-type choice (push-pull only), so the advice does not apply there |
| `W` with a faulted rail | `VREG too low, is there a short?` then `POWER SUPPLIES OFF` | VREG disabled again; check the target and the rails |
| `e` while VPU already powered | `Warning: already a voltage on Vpullup pin` plus a syntax error | refuses to change source; remove the external voltage or keep the current source |
| `f` while PWM is running | `ERROR: PWM active, g to disable` | stop PWM with `g` first; `f` itself works in HiZ |

## Parser and buffer

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| unknown character typed | `Syntax error at char <n>` + newline | the offending line aborts; `<n>` is the 1-based position |
| input line longer than the buffer | further characters beep | excess is **discarded**; the truncated line still runs on Enter |
| over-long user-macro body | syntax error | the whole `<n=…>` line must fit, so the body is about 22 bytes (v3) / 30 bytes (v4) |
| `;` width > 16 | syntax error | widths 1…16 only |
| `(n)` in a mode with no mode macros | `ERROR: command has no effect here` plus a syntax error | HiZ, 3WIRE and DIO have no mode macros; use a mode that does |

Input buffer: 128 bytes (v3) / 256 bytes (v4); long sequences belong in user
macros.

## Firmware display strings

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `?` label `o  Set output type` | misleading label | `o` sets the display format, not the per-mode output type |
| any tool that resets the bus by byte | expects `HiZ>` after the reset | v4 `#`, the binary `0x0F` return and the SUMP exit run the terminal reset and print the banner; v3 `#` reboots |

### A bus command in HiZ

The bus operations have no handler in HiZ.  `(0)` asks for the mode macros,
which HiZ does not have, so the command reports the no-effect error and then
the syntax error for the rest of the line:

```
(0)
ERROR: command has no effect here

Syntax error at char 3
HiZ>
```

The blank line is part of the output: the no-effect message is a complete
line, then the command adds its own line break.  The same applies to `r`,
`[`, `]`, `^`, `!` and a bare value.  In a bus mode the same command reaches
the mode's handler instead.
