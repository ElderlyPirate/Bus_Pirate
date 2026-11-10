# I2C — edge cases & failure modes

Part of the I2C documentation set (`docs/protocol/i2c/`).  Each entry is a
trigger, the observable symptom, and the recovery.  Timing and bus-error
detail is owned by [clock-stretching.md](clock-stretching.md).

## Wiring and pull-ups

| Trigger | Symptom | Recovery |
|---|---|---|
| No pull-ups on SDA/SCL | scanner reports every address | fit 2 kΩ–10 kΩ |
| A line held low | scanner warns, does not scan | clear the short |
| Start/stop cannot complete | `I2C bus error` | see clock-stretching.md |
| `P` with no VPU voltage | `Warning: no voltage on Vpullup pin` | select `e2`/`e3` (v4) |
| `e` with VPU already powered | `Warning: already a voltage on Vpullup pin` | remove the external source |

The scanner's pre-check is the only place the `*Short or no pull-up` warning
is printed.  A bus operation that cannot complete reports `I2C bus error`
instead — it does not print that warning.

## Read / ACK handling

| Trigger | Symptom | Behaviour |
|---|---|---|
| Read then start/stop | `NACK` before the condition | the pending bit is NACKed |
| Two reads in a row | `READ:  ACK <value>` | the ACK is from the first byte |
| Write after a read | `WRITE: <value> ACK …` | first ACK is the read's |
| Read left unresolved | no warning | send `]` or `[` to NACK it |
| `*(N)ACK PENDING` banner | never appears | only the next line shows it |

## Clock stretching

A slave that holds SCL low is waited on per bit, up to a fixed bound.  On
timeout the command reports `I2C bus error` and releases both pins.  The
outcomes, the hardware bus-clear and the binary `0x0A` status are in
[clock-stretching.md](clock-stretching.md).

## Macros

| Trigger | Symptom | Recovery |
|---|---|---|
| `(4)` before `(3)` | nothing | run `(3)` first |
| `(3)` in Software mode | `Onboard EEPROM requires hardware I2C` | re-enter with `m4 2 <speed>` |
| `(3)`/`(4)` on v3 | `Unknown macro, try ? or (0) for help` | v4 only |
| Leave and re-enter after `(3)` | external bus again | redirect is per session |
| Macro number other than 0–4 | `Unknown macro, try ? or (0) for help` | use `(0)` |

The on-board write protect is separate from the redirect: `(4)` clears it,
and a reset or a mode change re-asserts it.  See [eeprom.md](eeprom.md).

## Setup and settings form

| Trigger | Symptom | Recovery |
|---|---|---|
| `mod 2` with `spd 4` | driver/speed prompts restart | use `spd 1`–`3` |
| Expecting an output-type prompt | it is not asked | fixed open-drain |
| Typing the zero-based header back | wrong driver/speed | type menu numbers |
| Old v3 PIC (rev A3 or earlier) | `WARNING: HARDWARE I2C is broken on this PIC! (REV A3)` | use Software |

## Binary interface

| Trigger | Symptom | Recovery |
|---|---|---|
| `0x08` zero write count | `0x00`, no payload read | send the address byte |
| `0x08` count over the buffer | `0x00`, no payload read | ≤ 4096 (v4) / 2048 (v3) |
| Slave NACK in a write phase | STOP sent; `0x00` | start again normally |
| Bus timeout in a write phase | pins released; no STOP; `0x0A` = `0x00` | start fresh |
| Sniffer `0x0F` exited | `0x01` after the exit byte | send the next command |
| Expecting binary to use the chosen driver | always software | — |

## Unverified / v3

- The on-board EEPROM macros, the hardware-driver branch, the address-scanner
  output and the sniffer output are **not verified**.
- All v3 behaviour in this set is build-only and **not verified**.
