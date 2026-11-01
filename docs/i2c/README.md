# I2C

**Status:** draft, 2026-11-01 — v4 bench captures from 2026-10-26/27
against an emulated EEPROM-style slave; v3 is build-only (no v3 hardware here).

I2C is a two-wire bus (SDA data, SCL clock) plus ground.  This mode is a
master that drives start/stop conditions, reads and writes bytes, manages the
ACK/NACK bit, scans for device addresses, sniffs traffic, and has macros for
the on-board EEPROM.  It has two drivers: a software (bit-banged) driver and,
on v4, a hardware engine.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering I2C, the Software/Hardware sub-menu, both speed menus, the one-line settings form, defaults |
| [wiring.md](wiring.md) | SDA/SCL pins, the **required** pull-up, open-drain, VPU feeding, voltage limit, v3/v4 |
| [bus-operations.md](bus-operations.md) | start/stop, byte read/write, the delayed ACK/NACK model, the address scanner, a worked session |
| [clock-stretching.md](clock-stretching.md) | per-bit stretch wait, the ~25 ms timeout, software vs hardware |
| [macros.md](macros.md) | the `(0)` list and macros 1–4, including the on-board EEPROM macros |
| [sniffer.md](sniffer.md) | the sniffer macro, output format and limits |
| [eeprom.md](eeprom.md) | the on-board EEPROM story and the Windows I2C EEPROM tool |
| [binary.md](binary.md) | the hidden `I2C1` binary sub-protocol and a worked exchange |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter: `m` then `4`, or `m4`.  On v4 you first choose **1 Software** or
  **2 Hardware**; then choose a speed.  A default v3 build skips the sub-menu.
- Wiring headline: **SDA on MOSI, SCL on CLK, plus ground.  Pull-ups of
  2 kΩ–10 kΩ to the VPU voltage are mandatory** — with no pull-up the
  scanner reports a device at every address.
- `W` engages the bus; put a voltage on VPU before `P` (on v4 `e2` selects the
  on-board 3.3 V source, `e3` is 5 V).  Then `[` or `{` sends a start, `]` or
  `}` a stop.
- `r` reads a byte (`r:N` reads N); `0xNN`, `0b…` or a decimal writes one and
  checks the slave ACK.
- The ACK/NACK for a read byte is sent with the **next** command (a following
  start or stop resolves it as NACK).
- `(1)` scans for addresses, `(2)` is the sniffer; `(0)` lists all macros.
- **Clock stretching is supported** in the software driver (per-bit wait,
  bounded timeout) — the old v3 "not supported" note is wrong.
- The sniffer is a software capture on the bus; its usable rate is not stated
  and it is not a substitute for a logic analyzer — see
  [sniffer.md](sniffer.md).
- **Troubleshooting** → [edge-cases.md](edge-cases.md): an every-address scan,
  a stuck line, a stuck clock, a pending ACK.

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | present (build-only here) | present (default image) |
| bench coverage | none — v3 not verified on this bench | v4 bench session (emulated slave) |
| software driver | yes | yes |
| hardware engine (`I2C mode` sub-menu) | not in a default build | yes (sub-menu always asked; Software is a valid choice) |
| clock stretching | yes (software) | yes (software and hardware) |
| address scanner / sniffer | yes | yes |
| on-board EEPROM macros 3/4 | — | yes |
| binary `I2C1` sub-protocol | yes | yes (software driver only) |

Full per-item detail and exact wording are in the topic files.

## Limits & out of scope

- No device-specific command layer (no EEPROM page-write helper, no RTC
  decode).  I2C here is a manual master plus the scanner/sniffer macros.
- The hardware engine's actual frequencies (100 kHz / 400 kHz / 1 MHz) are
  given as the firmware labels; they were **not measured with a scope** on
  this bench.
- The on-board EEPROM macros (v4) are documented from the firmware and were
  **not exercised** on this bench.
- The sniffer's maximum rate is not stated here — no clean current capture
  exists (see [sniffer.md](sniffer.md)).
- Shared prompt grammar (`r`, `W`, `P`, `(n)`, wrappers, `e`, `v`, bit ops,
  safe reset `#`, bootloader `$`) lives in `docs/generic/` and is not repeated
  here.