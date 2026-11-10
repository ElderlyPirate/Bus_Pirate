# PC keyboard (KEYB)

**Applies to:** v4 only; not built on v3.

KEYB is a PC/AT keyboard mode.  It drives a PS/2 (PC/AT) keyboard's clock/data
interface and offers a live input monitor that streams the raw scancodes as you
type, so you can read a keyboard — or exercise a keyboard controller —
without a host PC driving it.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entry (mode 8), no-prompt setup, defaults, `W`, `v` |
| [wiring.md](wiring.md) | DATA/CLOCK pins, 5 V supply and naming, pull-up, open drain |
| [usage.md](usage.md) | frame and handshake, input monitor, `r` reads, writes, macros |
| [edge-cases.md](edge-cases.md) | no response, frame errors, power/pull-up failures |

## The five-minute version

- Enter: `m` then `8`, or `m8`.  KEYB has **no setup prompts** — you go
  straight to the `KEYB>` prompt.
- Wiring headline: **DATA → MOSI, CLOCK → CLK, +5 V → 5.0V, GND.**  If the
  keyboard has no DATA/CLOCK pull-ups (`Requires verification`), enable the
  Bus Pirate's: `W` first (the 5 V rail is the pull-up source), then `e3`, then
  `P` — or add external 1–10 kΩ to 5 V; see [wiring.md](wiring.md).
- Send `W` to power the keyboard before you read.
- `(1)` starts the **live input monitor** (`Input monitor, any key exits`); it
  prints each scancode as you type on the keyboard.  **The exit key is on the
  terminal**, not the keyboard.
- `r` reads one scancode frame; reads are not acknowledged — the ACK slot
  belongs to host writes — as described in
  [usage.md](usage.md#frame-and-handshake); a number (`0xNN`, `0b…`, decimal)
  writes a byte to the keyboard and reports `ACK` / `NACK` / `TIMEOUT`.
- `(0)` lists the two macros.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | not built | present (default image) |
| hardware | — | a PC/AT keyboard on MOSI/CLK + 5 V + GND |
| live input monitor | — | present |

Full per-item detail and the exact wording are in the topic files.

## Limits & out of scope

[usage.md](usage.md#limits) owns the mode's limits — in short: raw scancodes
only (no key-to-character decoding, no make/break interpretation), a live
monitor with no buffer, and no binary (BBIO) sub-protocol.

## Related docs

- [the generic layer](../../generic/README.md) — the shared prompt: `r`, `:N`,
  `;N`, `W`, `P`, `e`, `(n)`, `?`, display format `o`, reset `#`, bootloader
  `$`.
- [the mode menu and `i` banner](../../generic/navigation.md) — how mode 8 is
  listed.
- [the SPEC index](../../SPEC.md) — the KEYB row.
- Topics in this set: [entry-and-setup.md](entry-and-setup.md) ·
  [wiring.md](wiring.md) · [usage.md](usage.md) ·
  [edge-cases.md](edge-cases.md).
