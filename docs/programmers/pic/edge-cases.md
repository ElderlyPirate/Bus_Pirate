# PIC — edge cases & failure modes

Part of the PIC documentation set (`docs/pic/`).  Each entry is a concrete
trigger, the observable symptom, and the recovery.

## The target does not answer

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Target unpowered, no common ground, or not in programming mode | `(1)` → `DevID = 0x0000 Rev = 0x00` | power the target, share ground, put it in programming mode; retry |
| No pull-up on PGD/PGC (open drain) | reads return all-zero / meaningless | fit the 2–10 kΩ pull-up; retry |
| High-voltage part with no VPP applied | entry sequence has no effect; ID read fails | apply VPP externally (13 V) or use the high-voltage adapter |
| PGD or PGC open or swapped | no response, or the wrong word | check the wiring against [wiring.md](wiring.md) |

An all-zero `DevID`/`Rev` is the mode's only "no answer" signal — there is no
other error reporting in this mode.

## Power, VPU and pull-ups

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `P` before a voltage is on VPU | `Warning: no voltage on Vpullup pin` | select the source first (`e2`/`e3` on v4), then `P`; on v3 feed VPU externally |
| VPU already powered, then `e` | warning / refusal | drop the redundant `e`; the external supply owns the source |
| `e` while the pins are not open-drain HiZ | command refused | `e` is accepted only in the parked open-drain state |
| Target voltage ≠ pull-up voltage | unreliable reads or level stress | fit the pull-up voltage to the target's VCC |

## Command / Data state

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `r` while in **command** state | the read is refused — `no read` plus a zero result | press `]` first to enter Data state, then `r` |
| A payload written while in **command** state | only the low 6 (or 4) bits are clocked as an opcode | check the banner; use `[` for commands and `]` for payloads |
| Forgetting the state after `(1)` | the macro leaves the mode in Data state | read the banner, or leave and re-enter the mode |

## Entry and settings

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| One-line form `m 9 <width> <delay>` | header prints `PIC(mod dly)=(6 0)` even when other numbers were typed | the one-line numbers are not applied in this build, so it runs at 6b/14b with a 0 ms delay; use the interactive prompts (`m9`/`m8`) |
| `(1)` in **4b/16b** mode | `Not implemented (yet)` | the devID macro is 6b/14b only; re-enter in 6b/14b to read the ID |
| Very short delay (`0` ms) on a slow part | command writes may be too fast | choose a larger delay (1–3 ms) via the `Delay?` prompt |
| `CMD` banner width digit | the appended digit does not track the selected width in this build | treat the banner as a state marker only; the width is what you answered at the `Commandmode?` prompt (or the `6b/14b` default) |

## Display of values

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `r` output width | the displayed width and any `;N` marker follow the generic read rules and the current display mode (`o`) | set the display with `o`, and the width with `;N` if a narrower read is wanted |

## Leaving the mode

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Macro ends with `Please exit PIC programming mode` | the message is informational | change mode (`m`) before using the Bus Pirate for anything else |

## Unverified paths

- The `(1)` device-ID macro is the only output shown in this set that comes
  from the mode's own logic; the 4b/16b path and the binary protocols are
  **unverified**.
- The failure rows for missing VPP and missing pull-up are derived from the
  mode's open-drain design; they are not independently confirmed.
- v3 behaviour throughout this set is build-only and **not verified**.