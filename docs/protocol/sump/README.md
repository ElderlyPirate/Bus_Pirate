# SUMP logic analyzer

**Status:** draft — available on v3 and v4; v3 is build-only and unverified.

The Bus Pirate can act as a small SUMP-compatible logic analyzer: it samples
its signal pins into a buffer and streams the capture to a SUMP host program
(sigrok / PulseView). The analyzer is not on the mode menu — it is entered
by a hidden byte sequence that the host program sends on the serial port.
This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [capture.md](capture.md) | entry/exit, channels and pin mapping, client setup, sample rates, capture depth, trigger, the command reference, a worked exchange |
| [extended.md](extended.md) | the enhanced logic analyzer: 2k–32k samples, 2/4/8/16 MHz engines, trigger position |
| [edge-cases.md](edge-cases.md) | entry/exit mistakes, unsupported options, buffer limits, client mismatch |

## The five-minute version

- Enter: the host client sends a hidden byte sequence on the serial port —
  **at least five `0x00` (NUL) bytes**, fewer than twenty, then a
  **left-arrow** (`0x02`, or the `ESC [ D` a keyboard sends). A keyboard
  cannot type a NUL, so this is always sent by the client or a script.
- The device answers with its four-byte identification, `1ALS`, and then
  waits for the SUMP client.
- Open sigrok / PulseView, choose the SUMP / "Openbench Logic Sniffer & SUMP
  compatibles" driver, and point it at the Bus Pirate serial port — the
  client opens the port and drives both the entry and the capture. Pick a
  sample rate and a sample count, then capture.
- The analyzer records the signal pins as channels: v4 advertises eight probe
  slots (seven wired), v3 five. Samples stream back **newest-first**.
- **No pull-up is required** — the analyzer pins become inputs and the
  on-board pull-ups are switched off.
- Exit with `0x0F`, `#` or `$`; the board returns to the terminal in HiZ.

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | present (build-only, unverified) | present (default image) |
| sample memory | 2048 bytes | 4096 bytes |
| probes advertised | 5 | 8 (seven wired) |
| depth/speed tiers | unverified | present |

Full per-item detail and the exact wording are in the topic files.

## Limits & out of scope

- Flow control (XON/XOFF), the SUMP flags byte, trigger values/stages and the
  external clock are accepted or ignored, not implemented.
- Above 1 MHz the trigger always marks the start of the captured window.
- Larger captures trade channels for depth: 8k samples keep four channels,
  16k keep two and 32k keep one.
- No host client ships with the firmware; the SUMP client is external.
- Shared prompt grammar (`#`, `$`, `W`, `P`, `e`) lives in
  [the generic layer](../../generic/README.md) and is not repeated here.