# I2C

**Status:** documented for v4; v3 is build-only (unverified).

I2C is a two-wire bus (SDA data, SCL clock) plus ground.  This mode is a
master that drives start/stop conditions, reads and writes bytes, manages the
ACK/NACK bit, scans for device addresses, sniffs traffic, and has macros for
the on-board EEPROM.  It has two drivers: a software (bit-banged) driver and,
on v4, a hardware engine.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entry, driver and speed menus, the one-line form |
| [wiring.md](wiring.md) | pins, the required pull-up, open-drain, VPU, voltage |
| [bus-operations.md](bus-operations.md) | start/stop, read/write, delayed ACK, scanner |
| [clock-stretching.md](clock-stretching.md) | stretch wait, timeout, bus errors, recovery |
| [macros.md](macros.md) | the `(0)` list and the semantics of macros 1–4 |
| [sniffer.md](sniffer.md) | the sniffer, output format and limits |
| [eeprom.md](eeprom.md) | on-board EEPROM geometry and the Windows tool |
| [binary.md](binary.md) | the `I2C1` binary sub-protocol and a worked exchange |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter: `m` then `4`, or `m4`.  On v4 choose **1 Software** or
  **2 Hardware**, then a speed.  A default v3 build skips the sub-menu.
- Wiring: **SDA on MOSI, SCL on CLK, plus ground.  Pull-ups of
  2 kΩ–10 kΩ are mandatory** — without one the scanner reports a device at
  every address.
- Bring-up: `W` engages the bus; on v4 `e2`/`e3` selects the on-board 3.3 V
  or 5 V pull-up source, then `P` enables the pull-ups.  `[` or `{` sends a
  start, `]` or `}` a stop.
- `r` reads a byte (`r:N` reads N); `0xNN`, `0b…` or a decimal writes one and
  reports the slave ACK.  A read byte's ninth bit is sent with the **next bus
  operation**: a read/write sends ACK, a start/stop sends NACK.
- `(1)` scans, `(2)` sniffs, `(0)` lists macros.  Macros `(3)`/`(4)` touch the
  on-board EEPROM and need a mode entered with **2 Hardware**; from Software,
  `(3)` prints a refusal — leave and re-enter with `m4 2 <speed>`.
- **Clock stretching is supported** in both drivers with a bounded wait.  A
  timeout reports `I2C bus error`, releases both pins, and abandons the
  command — see [clock-stretching.md](clock-stretching.md).

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | present (build-only) | present (default image) |
| software driver | yes | yes |
| hardware engine | optional build, not the default | yes (always asked) |
| clock stretching | yes (software) | yes (software and hardware) |
| address scanner / sniffer | yes | yes |
| on-board EEPROM macros 3/4 | — | yes (Hardware driver) |
| binary `I2C1` sub-protocol | yes | yes (software driver only) |

Full per-item detail and exact wording are in the topic files.

## Limits & out of scope

- No device-specific command layer (no EEPROM page-write helper, no RTC
  decode).  I2C here is a manual master plus the scanner/sniffer macros.
- The hardware engine's `100kHz` / `400kHz` / `1MHz` labels are nominal: the
  actual SCL rate is lower and depends on bus rise time and load.  Measure at
  the target for fast modes; exact values for a given board are unverified.
- The on-board EEPROM macros (v4) are **not verified**.
- The sniffer's maximum rate depends on load and is unverified — see
  [sniffer.md](sniffer.md).
- Shared prompt grammar (`r`, `W`, `P`, `(n)`, wrappers, `e`, `v`, bit ops,
  safe reset `#`, bootloader `$`) lives in
  [docs/generic/](../../generic/README.md) and is not repeated here.
