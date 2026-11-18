# Edge cases & failure modes — base layer

Part of the generic-layer documentation set (`docs/generic/`).  Each entry is
concrete: trigger → observable symptom → behaviour and recovery.  The full
explanations are in [navigation.md](navigation.md), [values.md](values.md),
[commands.md](commands.md) and [usage.md](usage.md).

## Idle mode (HiZ)

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `W`, `w`, `P`, `p`, `g`, `S` in HiZ | `Command not used in this mode` | refused, no syntax error.  Switch to a mode with `m`; `P` / `g` / `S` work as soon as the mode is entered, before `W` |
| `e` (v4) in HiZ | `Command not used in this mode`, then `Syntax error at char 1` | refused; `e` needs a non-HiZ mode with open-drain outputs and no voltage on VPU |
| `e` on v3 | `Syntax error at char 1` | there is no `e` command |
| `O` outside `1WIRE` | `Command not used in this mode` | `O` is the 1-Wire speed toggle; refused in HiZ and every other mode |
| `~` in a non-HiZ mode | `Self-test in HiZ mode only` | return to HiZ (`m1` or `#`) and run it there |
| a bus operation in HiZ (`[` `{` `]` `}` `r` `/` `\` `^` `-` `_` `.` `!`, a value, `(n)`) | `ERROR: command has no effect here` plus a syntax error | nothing is sent; use a bus mode.  A prefix can precede the error line: `READ: ERROR: …`, `WRITE: 0x05 ERROR: …` |
| `a` / `A` in HiZ | `AUX LOW` / `AUX HIGH` | expected; the selected AUX pin is driven.  `@` returns it to input |
| `v` in HiZ with nothing connected | every I/O pin is `I`, with a mix of `H` / `L` states | floating inputs, not driven levels |
| `d`, `D`, `P`, `e`, `g`, `S`, `v` or `~` while the optional SMPS feedback owns the ADC | `ADC busy: SMPS feedback active; command rejected.` plus a syntax error | stop the SMPS first ([SMPS](../capabilities/smps/README.md)); a normal `HiZ>` prompt means it is already stopped |

A HiZ `(0)` shows the extra blank line that the mode-macro command adds:

```
(0)
ERROR: command has no effect here

Syntax error at char 3
HiZ>
```

## Power, pull-ups and VPU

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `P` with no pull-up voltage | `Pull-up resistors ON`, then `Warning: no voltage on Vpullup pin` | the pull-ups stay enabled; supply Vpullup (`e2` / `e3` in a non-HiZ open-drain mode on v4, external on v3) |
| `P` in a push-pull mode | `WARNING: pins not open drain (HiZ)`, then `Pull-up resistors ON` | informational; the pull-ups still switch on.  `(HiZ)` here is the open-drain high state, not the HiZ mode.  Choose open drain in the mode setup |
| `W` with a faulted rail | `VREG too low, is there a short?`, then `POWER SUPPLIES OFF` | the regulator is disabled again; check the target and rails, then `W` again |
| `e` with more than about 1.6 V already on Vpullup | `Warning: already a voltage on Vpullup pin` plus a syntax error | the source is unchanged; remove the external voltage or keep it |
| `e` in a push-pull mode | `WARNING: pins not open drain (HiZ)` plus a syntax error | choose open drain in the mode setup |
| `f` while a generator is active | `ERROR: PWM active, g to disable` | stop it with `g` / `S`, then measure |
| `w` in DIO | `Clutch disengaged!!!` then `POWER SUPPLIES OFF`; the prompt stays `DIO>` | the mode's cleanup runs; the DIO page owns what that means for the pins ([DIO](../protocol/dio/README.md)) |

## Parser, values and buffer

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| unknown character | `Syntax error at char <n>` | the line aborts; `<n>` is the 1-based position, sometimes past the visible text |
| input line longer than the buffer | further characters beep | excess is **discarded**; the truncated line still runs on Enter.  128 B (v3) / 256 B (v4) |
| a letter directly after a number (bus mode) | the number is sent, then the letter runs as a command | `12a` writes 12 then pulls AUX low; separate commands with a space if that is not intended |
| `;0`, `;17` | syntax error | widths are 1–16 |
| `:0`, `:40000` | syntax error | repeat counts are 1–32767 |
| a value above 65535 | syntax error | values are 16-bit |
| `0x` or `0b` with no digits | syntax error | nothing is sent |
| `-5` (bus mode) | DATA HIGH, then the value 5 | there are no negative values; in HiZ the `-` aborts the line |
| space before `:` or `;` | the value is sent first, then a syntax error | the suffix must follow the value immediately |
| non-standard width | each printed value carries `;N` | `0x123;12` prints `0x0123;12` |
| width set with `;N`, then later commands | the width persists | it stays until changed, a mode change or a reset |
| `=0x1234` | `0x34 = 52 = 0b00110100` | the conversion prints one byte; wider input is truncated |
| `\|X` with a width above 8 | the printed byte can be 0 | the reversal uses the current width; set 8 bits for a byte result |
| `rW` (bus mode) | one read, then `W` enables the supplies | only lowercase `x` / `d` / `b` / `w` are read-display letters |
| `o4` (RAW) | the raw byte may be invisible or unprintable | use HEX/DEC/BIN when reading text |
| `h5`, dismissed without a selection | after the history prompt, `5` is executed | `h` takes no argument; type `h` alone |

## Macros

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `<n=…>` longer than the line limit | syntax error, nothing stored | the whole command must stay under 27 (v3) / 35 (v4) characters; body about 22 / 30 with a one-digit slot |
| missing `>` in an assignment | syntax error | the first `>` ends the body; there is no escape |
| slot outside the range | syntax error | 1–3 on v3, 1–5 on v4 |
| `<n>` seems not to run | the body appears on the prompt line, nothing executes | press Enter to submit it; `<n>` recalls, it does not run |
| `(0)` or `(n)` in HiZ, 3WIRE or DIO | `ERROR: command has no effect here` plus a syntax error | those modes have no mode macros |
| macro body contains `>` | the assignment ends early | store the rest in another slot |
| macros gone after a power cycle | `<0>` lists empty slots | macros live in RAM; on v4 `#` and mode changes keep them, on v3 `#` clears them |

## Mode entry, reset and firmware

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `m0` | the mode menu opens | zero counts as "no argument" |
| invalid number at the menu prompt | `Invalid choice, try again`, prompt repeats | answer within 1…N or `x` |
| invalid direct form, e.g. `m11` | `Nonexistent protocol!` | the mode is unchanged |
| non-numeric direct form, e.g. `mx` | syntax error | the line aborts |
| incomplete inline answers, e.g. `m3 5` | the mode's setup prompts start from the top | supply the full answer order from the mode page |
| direct `m<N>` | no `Clutch disengaged!!!` line | the mode starts parked; `W` runs its bus setup (DIO pin writes are excepted) |
| `#` with text after it | the rest of the line disappears | v4 discards it during the reset |
| `#`, `$`, `m`, `m1` in BASIC | `Syntax error` | every line goes to the interpreter, including these; type `EXIT` first, then use them |
| `?` label `o  Set output type` | misleading label | `o` is the display format; output type is a mode setup answer |
| `?` label `s  Script engine` | not the word BASIC | `s` enters BASIC; the prompt gains `(BASIC)` |
| `?` on v4 has no `#` row | `#` is still a working command | the help slot is used by `o`; `t` / `T` / `O` have no row either |

## Bootloader and layer entry

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `$`, then `no` and Enter | the bootloader prompt is cancelled and the `o` opens the display-format menu | answer with a single `y` or `n`; the prompt reads one byte |
| `$` on a v3 with an older bootloader | the board resets back to the application | only the 4.5 loader understands the flag; use the PGC/PGD jumper (not verified on v3) |
| `$` inside SUMP | returns to the terminal in HiZ | SUMP treats `0x0F`, `#` and `$` as exits; type `$` at the prompt for the bootloader |
| fewer than 20 NUL bytes | no `BBIO1` reply | send 20 consecutive `0x00` bytes |
| 20 NUL bytes then `0x02` | enters binary bitbang (`BBIO1`), not SUMP | the 20th NUL already switched layers; the `0x02` then selects the binary I2C sub-mode.  Exit with `0x0F` and retry with 5–19 NULs |
| 5–19 NULs, then any other byte | no SUMP entry | only a raw `0x02` enters; the count is reset by every other byte |
| a raw `0x02` alone | the cursor moves left | it is the left-arrow control byte without a NUL prefix |
| NULs followed by Enter | no entry | Enter resets the count; send the NULs immediately before the entry byte |
