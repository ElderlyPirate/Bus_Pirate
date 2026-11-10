# Binary bitbang layer (BBIO1)

**Status:** draft — v3 is build-only (not verified).

The binary bitbang layer is the Bus Pirate's scriptable, byte-oriented
protocol. A host program drives the Bus Pirate with single-byte requests
instead of terminal text: the same hardware is controlled, but the replies
are raw bytes rather than human-readable lines. The layer is entered from the
terminal and is how all the binary sub-protocols are reached — SPI, I2C,
UART, 1-Wire and raw wire, plus the PIC, OpenOCD, XSVF, AVRISP and self-test
paths.

This README is a map, not the manual; the details live in the files below.

| File | Covers |
|---|---|
| [entering-and-framing.md](entering-and-framing.md) | terminal entry, wiring, exit, reply conventions, framing, settling |
| [commands.md](commands.md) | the complete BBIO1 byte map: pin state and direction bytes, core commands, PWM/ADC/frequency, self-test, wait-for-edge |
| [sub-protocols.md](sub-protocols.md) | sub-protocol entry bytes and identifiers, the shared configuration groups, per-mode links |
| [examples.md](examples.md) | worked entry → configure → transfer → exit exchanges |
| [edge-cases.md](edge-cases.md) | failure modes: trigger → symptom → recovery |

## The five-minute version

- Enter from the terminal by sending **20 × `0x00`**; the Bus Pirate replies
  `BBIO1`. There is no prompt.
- Reply convention: **`0x01` = success/ack · `0x00` = failure**; commands that
  read return data bytes instead.
- `0x00` re-prints `BBIO1`; `0x0F` resets and returns to the terminal.
- Sub-protocol entry bytes: `0x01` SPI, `0x02` I2C, `0x03` UART, `0x04`
  1-Wire, `0x05` raw wire, `0x06` OpenOCD, `0x07` PIC, `0x08`/`0x09` AVRISP,
  `0x18` XSVF.  Each prints its own identifier (`SPI1`, `I2C1`, `ART1`, `1W01`,
  `RAW1`, `OCD1`, `PIC1`, `XSV1`) except the AVRISP entries, which print none.
- In BBIO1, the pins are driven by one **state byte** (`1xxxxxxx`) and set to
  input/output by a **direction byte** (`010xxxxx`); the `0x70` command waits
  for an edge on the AUX pin.
- Wiring headline: the layer drives the five header lines **AUX, MOSI, CLK,
  MISO, CS** plus ground.  Whether a **pull-up is required** depends on the
  selected sub-protocol and its output type — open-drain output needs a
  pull-up, push-pull does not; see the wiring page of the active mode and
  [entering-and-framing.md](entering-and-framing.md) §Wiring.
- Per-mode command maps (speed tables, transfers, sniffers) live in each
  mode's own `binary.md`, linked from
  [sub-protocols.md](sub-protocols.md).

## Availability (summary)

| Group | v3 | v4 |
|---|---|---|
| Core (pin I/O, reset, self-test, PWM, ADC, frequency, `0x70`) | present (build-only) | present |
| SPI / I2C / UART / 1-Wire / raw-wire / OpenOCD / PIC entries | present (build-only) | present |
| XSVF player (`0x18`) | — (answered with a failure) | present |
| AVRISP entries (`0x08` / `0x09`) | — (answered with a failure) | present |
| Pull-up feed (`0x5x`) | — (answered `0x00`) | present |

The v3 column is **not verified** (v3 is build-only here).

## Limits & out of scope

- This set documents the **binary layer itself**: entry, framing, the reply
  conventions, and the BBIO1 core command map. The per-protocol command maps
  are owned by each mode's `binary.md`.
- The SUMP logic analyzer is a separate mode with its own entry sequence — see
  `docs/sump/`.
- Building and flashing the firmware is a developer topic (`docs/dev/`).

## Related docs

- [../generic/README.md](../../generic/README.md) — the terminal prompt layer and
  the binary entry sequence.
- [../SPEC.md](../../SPEC.md) — the capability matrix.
- Mode sets: `docs/spi/`, `docs/i2c/`, `docs/uart/`, `docs/1wire/`,
  `docs/2wire/`, `docs/3wire/`, `docs/pic/`, `docs/jtag/`, `docs/avrisp/`,
  `docs/self-test/`, `docs/smps/`.