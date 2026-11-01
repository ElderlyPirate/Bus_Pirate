# Edge cases & failure modes — base layer

Part of the generic-layer documentation set (`docs/generic/`).

## Edge cases & failure modes

| Trigger | Observable symptom | Behaviour / recovery |
| --- | --- | --- |
| Unknown character typed | `Syntax error at char <n>` + newline | the offending command line aborts; the prompt returns. `<n>` is the 1-based position |
| `rx` / `rd` / `rb` / `rw` typed | `READ: ...` then `Syntax error at char 1` (see below) | `r` peeks the display letter but does not consume it; the leftover letter errors the line |
| Command used where it has no effect (e.g. a write in HiZ) | `ERROR: command has no effect here` then a syntax error | the action has no handler in this mode and flags an error |
| `g` / `p` / `P` / `W` / `w` / `e` outside an active bus | `Command not used in this mode` | refused in HiZ; only `e` also raises a syntax error |
| `P` with no pull-up voltage | `Pull-up resistors ON` then `Warning: no voltage on Vpullup pin` | pull-ups stay enabled; supply the Vpullup pin (`e2`/`e3` on v4, external on v3) |
| `P` in push-pull mode | `WARNING: pins not open drain (HiZ)` before enabling | informational; switch to open-drain in the mode setup |
| `W` with a faulted rail | `VREG too low, is there a short?` then `POWER SUPPLIES OFF` | VREG disabled again |
| `e` while VPU already powered | `Warning: already a voltage on Vpullup pin` + syntax error | refuses to change source |
| `~` outside HiZ | `Self-test in HiZ mode only` | refused; `~` only runs in HiZ |
| `f` while PWM is running | `ERROR: PWM active, g to disable` | stop PWM with `g` first; `f` itself works in HiZ |
| Input line longer than the buffer | further characters beep | buffer size 128 (v3) / 256 (v4); must be a power of two |
| Over-long user-macro body | syntax error | body must fit 24/32 bytes |
| `;` width > 16 | syntax error | widths 1…16 only |
| `v` label row | pin-1 legend label garbled (stray byte) | cosmetic string defect; transcript shows `1.(Wt)*`; values unaffected |
| Any tool that resets the bus by byte | expects `HiZ>` after reset | v4 `#`/binary reset forces HiZ and prints the banner |
| `?` label `o  Set output type` | misleading label | it is the display format, not the per-mode output type |

### `rx` / `rd` / `rb` / `rw`

The `r` command peeks the character after it (`x`/`d`/`b`/`w`) to override the
display for that one read, but it does **not** consume the letter.  Typing `rx`
therefore performs the read and then reports a syntax error on the leftover
letter.  In HiZ:

```
rx
READ: ERROR: command has no effect here
0x00
Syntax error at char 1
```

In HiZ the read has no effect, so the read itself reports
`ERROR: command has no effect here`.  The exact behaviour in a mode where the
read succeeds (the expected leftover error would be at char 2) is not verified.
See [commands.md](commands.md) §Repetition / width grammar details.  The same
applies to `rd`, `rb` and `rw`.