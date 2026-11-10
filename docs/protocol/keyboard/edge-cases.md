# PC keyboard (KEYB) — edge cases & failure modes

Part of the KEYB documentation set (`docs/keyboard/`).  Each entry is a
concrete trigger, the observable symptom, and the recovery.

## Read and monitor

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| No `W` sent before reading | reads return `NONE`; the monitor stays silent | send `W` first — it powers the keyboard and configures the pins |
| Keyboard unpowered, or VCC/GND not connected | `NONE` on every read; monitor silent | connect 5 V and ground; then `W` |
| Keyboard not on a common ground | intermittent `NONE` / parity errors | share ground between keyboard and Bus Pirate |
| Monitor appears not to end when typing on the keyboard | scancodes keep printing | expected: the exit key is on the terminal; press a terminal key |
| A terminal key pressed to exit | monitor ends, prompt returns | — |

## Frame errors

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Clock line never changes (keyboard idle or unpowered) | `TIMEOUT` | power and connect the keyboard |
| Data line not low at frame start (missing/weak pull-up, wrong pin) | `*startbit error` | check DATA is on MOSI and the keyboard is powered; if the line never idles high, add a pull-up — Bus Pirate `e3` then `P`, or external 1–10 kΩ to 5 V |
| A bit misread (noise, long wires) | `*parity error` | shorten wiring, check ground, retry |
| Frame does not end high | `*stopbit error` | check wiring and power |
| Any other read result | `UNKNOWN ERROR` | retry; check wiring |

The `NONE`, `TIMEOUT`, `*startbit error`, `*parity error`, `*stopbit error` and
`UNKNOWN ERROR` words are the mode's own error labels.

## Writing

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Write to a keyboard that is not powered | `TIMEOUT` | power the keyboard (`W`) |
| Write to a keyboard that has scanning disabled | `NACK` or `TIMEOUT` | send `0xF4` (enable) or reset with `0xFF` |
| Write before `W` (bus parked) | unreliable — no configured pins | send `W` first |

## Power

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Short on the 5 V rail | `VREG too low, is there a short?` + `POWER SUPPLIES OFF` | clear the short; retry `W` |
| Keyboard draws more than the Bus Pirate 5 V rail supplies | rail sags, or `W` refuses | use an external 5 V supply sharing ground |

## Macros

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A number that is not a macro (`(2)` …) | `Unknown macro, try ? or (0) for help` | use `(0)` to list the valid macros |

## Unverified paths

- **Unverified:** the frame-error and acknowledge word combinations above.
- **Unverified:** the `VREG too low` path is a shared base-layer message; its
  behaviour in KEYB.