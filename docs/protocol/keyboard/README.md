# PC keyboard (KEYB)

**Status:** draft — **v4 only**; not built on v3.

KEYB is a PC/AT keyboard mode.  It drives a keyboard's two-wire clock/data
interface and offers a live input monitor that streams the raw scancodes as you
type, so you can read or decode a keyboard — or exercise a keyboard controller
— without a host PC driving it.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering KEYB (mode 8), the no-prompt setup, defaults, `W`, `v` |
| [wiring.md](wiring.md) | data/clock pins, the 5 V supply and its naming, when a pull-up is needed, open drain |
| [usage.md](usage.md) | the live input monitor, `r` reads, byte writes, the `(0)` / `(1)` macros |
| [edge-cases.md](edge-cases.md) | no response, frame errors, stuck keys, power/pull-up failures |

## The five-minute version

- Enter: `m` then `8`, or `m8`.  KEYB has **no setup prompts** — you go
  straight to the `KEYB>` prompt.
- Wiring headline: **DATA → MOSI, CLOCK → CLK, +5 V → 5.0V, GND.**  If the
  keyboard has no DATA/CLOCK pull-ups (`Requires verification`), enable the
  Bus Pirate's (`e3`, then `P`) or add external 1–10 kΩ to 5 V — see
  [wiring.md](wiring.md).
- Send `W` to power the keyboard and configure the pins before you read.
- `(1)` starts the **live input monitor** (`Input monitor, any key exits`); it
  prints each scancode as you type on the keyboard.  **The exit key is on the
  terminal**, not the keyboard.
- `r` reads one scancode; a number (`0xNN`, `0b…`, decimal) writes a byte to
  the keyboard and reports `ACK` / `NACK` / `TIMEOUT`.
- `(0)` lists the two macros.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | not built | present (default image) |
| hardware | — | a PC/AT keyboard on MOSI/CLK + 5 V + GND |
| live input monitor | — | present |

Full per-item detail and the exact wording are in the topic files.

## Limits & out of scope

- It shows **raw scancodes**, not decoded characters: there is no key-to-ASCII
  translation, and make/break (press/release) is not interpreted.
- No binary (BBIO) sub-protocol.
- Shared prompt grammar (`r`, `:N`, `;N`, `W`, `P`, `(n)`, `?`, display format
  `o`, safe reset `#`, bootloader `$`) lives in
  [the generic layer](../../generic/README.md) and is not repeated here.