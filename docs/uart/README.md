# UART

**Status:** draft — v3 is build-only (unverified).

UART is asynchronous serial: start/stop-framed bytes on a data line.  This
mode is a full-duplex UART (**TX** + **RX** + ground), a half-duplex
**single-wire** UART (HDUART), a transparent USB↔serial bridge, a live
monitor, and a binary (`ART1`) sub-protocol.  One engine and one command set
cover all of them.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering UART, the speed menu, custom baud / BRG, bits/parity/stop/polarity/output/duplex, the settings header, defaults |
| [wiring.md](wiring.md) | full-duplex pins, the HDUART single wire and its **required** pull-up, open-drain, VPU feeding, voltage limit, v3/v4 |
| [terminal-usage.md](terminal-usage.md) | sending values, `r` / `r:N` / `r;bits`, the live display `[`/`{` and `]`/`}`, buffer behaviour |
| [hduart.md](hduart.md) | half-duplex single-wire: selection, no self-echo, open-drain + pull-up, speeds |
| [macros.md](macros.md) | the `(0)` list, the live monitor `(2)`, auto-baud `(4)` |
| [bridge.md](bridge.md) | the transparent bridge `(1)` and the flow-control bridge `(3)` |
| [binary.md](binary.md) | the `ART1` binary sub-protocol and a worked exchange |
| [errors.md](errors.md) | parity, framing and overrun reporting |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter: `m` then `3`, or `m3`; answer the setup prompts (speed, data
  bits/parity, stop bits, receive polarity, output type, duplex).  The speed
  prompt defaults to `1` = 300 bps — pick the speed explicitly.
- Wiring headline: **full duplex — TX on MOSI, RX on MISO, plus ground.**
  For **half-duplex single wire** use MOSI only and fit a **pull-up to
  3.3 V (1 kΩ–10 kΩ) — it is required** (see [hduart.md](hduart.md)).
- `W` starts the supplies (v4 prints `Clutch engaged!!!`); `P` enables the
  on-board pull-ups and needs a voltage on VPU (`e2` on v4).
- Send with `0xNN`, `0b…`, a decimal, or `"text"`; `r` reads one byte;
  `r:N` reads N.  `[`/`{` open a live display, `]`/`}` stop it.
- `(0)` opens the macro menu — see [macros.md](macros.md) for the four
  macros.
- Failures are reported inline: `-f ` (framing), `-p ` (parity),
  `FAILED, NO DATA` (nothing to read) — see [errors.md](errors.md).

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | present (build-only, unverified) | present (default image) |
| full duplex | yes | yes |
| half-duplex single-wire (HDUART) | yes | yes |
| speed 300 … 115200 | yes | yes |
| custom baud | raw BRG value | baud rate in bps |
| entry-time auto-baud (speed option 11) | — | yes |
| auto-baud macro `(4)` | listed + handled | listed + handled |
| bridge `(1)` / live monitor `(2)` / flow bridge `(3)` | yes | yes |
| bridge exit | reset only | NORMAL button |
| flow-control signals (macro 3) | CTS on CS, RTS on CLK | not wired (macro 3 = macro 1) |
| binary `ART1` | yes | yes (pull-up feed `0x5x` is v4 only) |

Full per-item detail and the exact wording are in the topic files.

## Limits & out of scope

- No protocol above the byte layer: no MODBUS/AT command parsing, no
  address/checksum handling.  UART is a manual byte bus and a bridge.
- The half-duplex variant has **no collision detection**: bytes the peer
  sends while the Bus Pirate is transmitting are not kept.
- Overruns are absorbed by a software receive buffer (128 bytes) and
  counted, but are **not reported** to the terminal by this firmware — the
  old "*Bytes dropped*" line is not printed (see [errors.md](errors.md)).
- MIDI is not a menu entry; 31250 bps is reachable through the binary speed
  operand or a custom baud value — see
  [entry-and-setup.md](entry-and-setup.md).
- Shared prompt grammar (`r`, `W`, `P`, `n`, `(n)`, wrappers, bit ops, safe
  reset `#`, bootloader `$`) lives in `docs/generic/` and is not repeated
  here.