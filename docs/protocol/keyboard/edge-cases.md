# PC keyboard (KEYB) — edge cases & failure modes

Part of the KEYB documentation set (`docs/protocol/keyboard/`).  Each table
gives one condition per row and the symptom it produces.

A silent or garbled monitor: check in this order — **power** (`W`, VCC, GND),
then **pull-up** (both lines idle high between exchanges), then
**clock/data** (DATA on MOSI, CLOCK on CLK).  Every failed read also prints
`0x00` after its error word (or appended, for `TIMEOUT`) — that value is a
sentinel, not a scancode ([usage.md](usage.md#reading-with-r)).

## Read and monitor

| Trigger | Symptom |
|---|---|
| Keyboard unpowered: no `W`, or VCC/GND open | `TIMEOUT` on every read; monitor silent |
| Keyboard powered but nothing to send: idle, or scanning off | `NONE` on each read; monitor silent |
| Lines held high, keyboard unpowered | `NONE` on each read; monitor silent |
| Typing on the keyboard does not end the monitor | scancodes keep printing |

- `TIMEOUT` — send `W` and check VCC and GND.
- `NONE` — normal when the keyboard is idle; press a key.  After `0xF5`
  (scanning off) writes still work but no scancodes arrive; `0xF4` re-enables
  scanning.  If the lines are high but the keyboard has no supply, check that
  its VCC is on the 5.0V pin.
- The monitor's exit key is on the terminal, not the keyboard.

## Frame errors

| Trigger | Symptom |
|---|---|
| No pull-up, or a weak one | `TIMEOUT` if the clock never goes high; else `*parity error` / `*stopbit error` |
| DATA on the wrong pin, or DATA held high | `*startbit error` |
| CLOCK held low by the keyboard or a short | `*startbit error` |
| One `*startbit error` that clears on retry | the read caught the keyboard's own clock-low window |
| A bit misread: noise, long wires, no ground | `*parity error` |
| The frame ends without a high stop bit | `*stopbit error` |
| Any other read result | `UNKNOWN ERROR` |

- `TIMEOUT`, `*parity error`, `*stopbit error` — both lines must idle high
  between exchanges: `P` after `W`, or external 1–10 kΩ to 5 V.  Then check
  the common ground and shorten the wiring.
- `*startbit error` — DATA must be on MOSI and driven by the keyboard; CLOCK
  must not be held low.  A single one that clears on retry is the keyboard's
  clock-low window before its frame.
- A disconnected DATA line reads as a fixed level: high fails the start-bit
  check, low fails parity.  Check DATA is on MOSI.
- `UNKNOWN ERROR` — a defensive label; the current read path cannot produce
  it.  Retry.

The `NONE`, `TIMEOUT`, `*startbit error`, `*parity error`, `*stopbit error` and
`UNKNOWN ERROR` words are the mode's own error labels; the meanings are in
[usage.md](usage.md#reading-with-r).

## Writing

| Trigger | Symptom |
|---|---|
| Write before `W`, or to an unpowered keyboard | `TIMEOUT` — send `W` |
| Scanning disabled (`0xF5`) | `ACK`; scancodes stay off — `0xF4` re-enables |
| Command not accepted, or a full buffer | `NACK` — check the byte; `0xFF` resets |
| Two-byte command sent without its data byte | the command byte `ACK`s, then waits — send the mask byte |

`TIMEOUT` on a write usually means the keyboard is unpowered (send `W`): the
write sets up the clock and data pins itself, so the keyboard has to be powered
for its clocks and acknowledge to arrive.  A stuck or shorted CLOCK, or a wrong
pin, also times out — check the wiring if the keyboard is powered.

## Power and pull-up

| Trigger | Symptom |
|---|---|
| Short on the 5 V rail | `VREG too low, is there a short?` + `POWER SUPPLIES OFF` |
| Keyboard over-loads the 5 V rail | rail sags, or `W` refuses — use external 5 V |
| `P` sent before the 5 V rail is up | `Warning: no voltage on Vpullup pin` |

- A short: clear it and retry `W`.
- Over-load: use an external 5 V supply with a common ground.
- `P` warning: the pull-up source has no voltage — run `W` and `e3` first,
  then `P`.  The warning is about the order, not a failed pull-up.

## Macros

| Trigger | Symptom |
|---|---|
| A number that is not a macro (`(2)` …) | `Unknown macro, try ? or (0) for help` |

## Unverified paths

- **Unverified:** whether a given keyboard keeps acknowledging host commands
  while its scanning is disabled.  That is standard PC/AT behaviour and the
  mode's write path does not depend on scanning, so the table above lists the
  normal case.
