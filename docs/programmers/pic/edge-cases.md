# PIC — edge cases and failure modes

Part of the PIC documentation set (`docs/programmers/pic/`).  Each entry is a
concrete trigger, the observable symptom, and the recovery.  This page owns
the current limitations; the other pages state the behaviour in one sentence
and link here.

## The target does not answer

| Trigger | Symptom | Behaviour and recovery |
|---|---|---|
| Pull-up fitted, no target | `(1)` all ones: `DevID = 0x01FF`, `Rev = 0x1F`; `r` `0x3FFF;14` | the no-answer signature; check power, ground and entry |
| No pull-up on PGD/PGC | `(1)` and `r` return an undefined word | fit the 2–10 kΩ pull-up and feed VPU |
| PGD held low | `(1)` all zero: `DevID = 0x0000`, `Rev = 0x00` | check target power, ground, PGD/PGC wiring and shorts |
| Target unpowered but connected | a low, clamped or garbage word | power the target (its own supply or `W`) and retry |
| HVPP part, no VPP | entry has no effect; the read is all ones or garbage | apply VPP through the external switch; see [wiring.md](wiring.md) |
| Wrong part | a valid non-zero ID that is not the part's | the mode cannot detect it; the host tool checks |
| Read-protected part | a plausible or masked word | the mode cannot detect or clear protection |

All ones with the pull-up fitted is the "no answer" signature, not all zero.
The mode has no error reporting and cannot detect a wrong part.

## Pull-up, VPU and power

| Trigger | Symptom | Behaviour and recovery |
|---|---|---|
| `P` with no VPU voltage | `Pull-up resistors ON` then `Warning: no voltage on Vpullup pin` | `e2`/`e3` (v4) then `W`, or an external VPU feed; then `P` again |
| `e` while VPU is already powered | `Warning: already a voltage on Vpullup pin` | remove the external VPU feed; `e1` disables the on-board source |
| `e` in HiZ | `Command not used in this mode` | enter PIC first |
| Target VCC differs from the pull-up feed | low high level, unreliable reads | match the pull-up feed to the target's logic rail |
| `W` with a shorted rail | `VREG too low, is there a short?` then `POWER SUPPLIES OFF` | remove the short, check the wiring, retry `W` |
| Target self-powered and the BP rail connected | back-feed between supplies | leave the `3.3V`/`5.0V` pins open; `W` only for the on-board pull-up feed |

## Command and data state

| Trigger | Symptom | Behaviour and recovery |
|---|---|---|
| `r` in Command state | `READ: no read` then `0x00` | send `]` (`DTA`) first |
| A payload typed in Command state | only the low 6 (or 4) bits go out; the echo is byte-wide | check the banner; `[` = command, `]` = payload |
| A wide opcode in Command state | the echo shows the low 8 bits but only 6/4 reach the wire | keep opcodes within 6/4 bits |
| State after `(1)` | the macro ends in Data | the banner is not printed while the macro runs; send `[` to return to command |
| State after `w` | cleanup resets to Data | the mode is still active; a value typed after `w` still clocks out |
| `L` selected | the wire order becomes MSB-first; reads display reversed | leave the generic bit order at `l` (no reversal) |

Entry and cleanup reset the state to Data, so each session starts in Data.
Within a session, always check the banner.

## Entry and settings

| Trigger | Symptom | Behaviour and recovery |
|---|---|---|
| `m 9 2` (one number) | both prompts run; no settings header | nothing is applied; answer both, or give `m 9 2 1` |
| `m 9 0 1` (invalid width) | the two prompts run | width is `1`–`2` |
| `m 9 1 4` (invalid delay) | the two prompts run | delay is `0`–`3` |
| `(1)` in 4b/16b | `Not implemented (yet)` | use the host tool for a 4b/16b part |
| Wrong width for the part | entry or ID read fails | match the part's programming specification |
| Delay 0 on a slow part | command writes may be too fast | choose 1–3 ms; this is not the programming time |
| One-line form not applied | the prompts run instead | check both; a missing or invalid width or delay triggers it |

The incomplete-pair fallback in full:

```
HiZ>m 9 2
Commandmode?
1. 6b/14b
2. 4b/16b
(1)>1
Delay?
(1)>1
Ready
PIC>
```

Nothing from the typed `2` is applied.  After the two answers the mode runs
6b/14b at 1 ms; leaving both prompts at Enter gives the same defaults.  Give
both numbers (`m 9 2 1`) to skip the prompts.

## Configuration and fuse words

| Trigger | Symptom | Behaviour and recovery |
|---|---|---|
| Wrong configuration word | the part stops answering ICSP | restore with a high-voltage programmer or a bulk erase; some settings are unrecoverable |
| Code protection set | reads look plausible or masked | the mode cannot detect or clear it |
| LVP disabled | the LVP entry no longer works | use HVPP or erase the part |
| Writing config by hand | an unverified word is latched | read it first, keep a copy, use the host tool |

The mode has no configuration command; the host builds the sequence.  See
[config-words.md](config-words.md).

## Binary paths

| Trigger | Symptom | Behaviour and recovery |
|---|---|---|
| `PIC1` with no VPU feed | board pull-ups on, lines not high | binary entry turns the v4 source off; feed VPU externally |
| `0xA4`/`0xA5` in the default engine | `0x00` failure | send `0xA0 1` (416) or `0xA0 2` (424) first |
| `0xA5` expected a result byte | the next byte is data | `0xA5` has no result byte; 416 returns one byte per count, 424 six |
| `0xA7` read ordering | the first byte after `0x01` is data | with read records the success byte precedes the data |
| `0xC0` read with no target | `0x3F 0xFF` (6-bit) or `0xFF 0xFF` (4-bit) | all ones; check the pull-up and target |
| `0x07` on a build without PIC | `BBIO1` printed, no `PIC1` | the build omitted PIC support |
| `0x00` from `PIC1` | `BBIO1` printed | that is the normal exit; send `0x0F` for the terminal |

## Leaving and parking

| Trigger | Symptom | Behaviour and recovery |
|---|---|---|
| `Please exit PIC programming mode` | informational | change mode (`m`/`m1`) before other use |
| `w` used to "leave" | the prompt is still `PIC>`; a value still clocks out | `w` powers down and runs cleanup only; use `m1` |
| `m1` | `Ready` then `HiZ>`; the rails and pull-ups are off | the normal way back |

## Unverified paths

- The binary `PIC1` loop and the raw-wire PIC group are **not verified** on
  hardware.
- The missing-VPP and missing-pull-up symptoms are derived from the
  open-drain design; they are not verified on specific parts.
