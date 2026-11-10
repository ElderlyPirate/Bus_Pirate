# PIC — edge cases & failure modes

Part of the PIC documentation set (`docs/programmers/pic/`).  Each entry is a
concrete trigger, the observable symptom, and the recovery.  **This page owns
the current limitations**; the other pages state the behaviour in one sentence
and link here.

## The target does not answer

There is no error reporting from this mode; the value you read is the only
signal, so compare it with the part's expected ID.

| Trigger | Symptom | Recovery |
|---|---|---|
| Pull-up fitted, target absent | `(1)` all ones (`DevID = 0x01FF`, `Rev = 0x1F`) | power, ground and programming entry |
| No pull-up on PGD/PGC | read value undefined | fit the 2–10 kΩ pull-up |
| PGD held low | `(1)` all zero (`DevID = 0x0000`, `Rev = 0x00`) | check power, ground, PGD/PGC wiring |
| High-voltage part, no VPP | entry has no effect | apply VPP (see [wiring.md](wiring.md)) |
| Wrong part | a non-zero ID that is not the part's | compare with the part's ID; the host tool checks |
| PGD/PGC open or swapped | no response or wrong word | check the wiring |

All ones with the pull-up fitted is the "no answer" signature, not all zero.
The mode has no error reporting and cannot detect a wrong part.

## Power, VPU and pull-ups

| Trigger | Symptom | Recovery |
|---|---|---|
| `P` with no VPU voltage | `Warning: no voltage on Vpullup pin` | `e2`/`e3` (v4) or external VPU (v3) first |
| VPU already powered, then `e` | `Warning: already a voltage on Vpullup pin` | drop the redundant `e` |
| `e` outside the mode | `Command not used in this mode` / pins not open-drain | use `e` only inside PIC mode |
| Target VCC ≠ pull-up voltage | unreliable reads or level stress | match the pull-up voltage to the target |
| Target short at `W` | `VREG too low, is there a short?` | remove the short, check wiring, retry `W` |

## Command / Data state

| Trigger | Symptom | Recovery |
|---|---|---|
| `r` in Command state | `no read` plus a zero result | `]` first, then `r` |
| Payload written in Command state | only the low 6 (or 4) bits go out | check the banner; `[` command / `]` payload |
| Re-entry left in Command | no `CMD` banner; value goes out as an opcode | setup does not reset state; `]` to `DTA` |
| State after `(1)` | the macro ends in Data | read the banner, or leave and re-enter |

## Entry and settings

| Trigger | Symptom | Recovery |
|---|---|---|
| One-line number out of range | the two prompts run instead | width `1`–`2`, delay `0`–`3` |
| One-line form with one number | missing delay is read as `0` | give both numbers |
| `(1)` in 4b/16b | `Not implemented (yet)` | the macro is 6b/14b only; use the host tool |
| Wrong width for the part | entry or ID read fails | match the part's programming protocol |
| Delay `0` on a slow part | command writes may be too fast | choose 1–3 ms |

## Code-protected parts

| Trigger | Symptom | Recovery |
|---|---|---|
| Read-protected part | reads may look valid but are masked | the mode cannot detect or clear protection |

The host tool owns erase and verify; a protected part can still return a
plausible-looking read.

## Display of values

| Trigger | Symptom | Recovery |
|---|---|---|
| Bare `r` | `READ: 0x00NN;8` — low 8 bits | the read is always 14/16 bits; use `r;14`/`r;16` |
| `;N` on a read | does not narrow the target read | sets only the display mask (and the `L` width) |
| `L` selected | wire pattern and read change | PIC clocks LSB-first; leave `l` |

## Leaving the mode

| Trigger | Symptom | Recovery |
|---|---|---|
| `Please exit PIC programming mode` | informational | change mode (`m`) before other use |

## Unverified paths

- The `(1)` device-ID macro is the only output shown in this set that comes
  from the mode's own logic; the 4b/16b path and the binary protocols are
  **unverified**.
- The failure rows for missing VPP and missing pull-up are derived from the
  mode's open-drain design; they are not independently confirmed.
- v3 behaviour throughout this set is build-only and **not verified**.
