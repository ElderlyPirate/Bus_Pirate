# 2-Wire (raw 2-wire)

**Status:** present in the v3 and v4 builds; the v3 hardware behaviour is not
verified.

Part of the [wire protocol set](../README.md).  The **raw 2-wire** library is a
generic two-wire bit-bus: it drives one clock line and one bidirectional data
line by hand and clocks bits in and out.  It is **not I2C** — it does not
handle acknowledge bits, addressing, or any protocol layer.  I2C could be
built from these primitives, but the mode itself is a bare bit bus.  Its
headline extra is a pair of **ISO 7816-3 ATR** macros for smart cards.

The mode displays as `2WIRE`; the library it implements is the "raw 2-wire"
library and the settings header abbreviates it `R2W`.  This set writes the mode
as `2-Wire` in prose.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering `2WIRE`, the speed and output-type menus, the one-line settings form, defaults |
| [wiring.md](wiring.md) | SDA/SCL pins, the output types, the pull-up requirement, voltage limit, v3/v4 |
| [bus-operations.md](bus-operations.md) | start/stop, byte read/write, clock edges, the bitwise controls, a worked session |
| [macros.md](macros.md) | the `(0)` macro list and the ISO 7816-3 ATR macros |
| [smartcard.md](smartcard.md) | smart-card / ISO 7816-3 context, card wiring, the ATR flow |
| [binary.md](binary.md) | the `RAW1` binary sub-protocol, 2-Wire specifics |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter with `m` then `6`, or `m6` — 2-Wire is mode number **6**.  Answer the
  two prompts; the defaults are `1` (**~6kHz**, **open drain**).
- Wiring: **data on SDA (MOSI), clock on SCL (CLK), plus ground**.  Open-drain
  output **needs a pull-up on each of SDA and SCL (2 kΩ–10 kΩ)**; normal
  output needs none.
- `W` engages the bus.  With open drain, `P` enables the pull-ups (feed VPU
  first — on v4 with `e2`/`e3`).
- `[` or `{` starts, `]` or `}` stops.  `r` reads a byte; `0xNN`, `0b…` or a
  decimal writes one — **no acknowledge bit is involved**.
- Bitwise controls: `^` clock tick(s), `/` clock high, `\` clock low, `-` data
  high, `_` data low, `.` reads the data pin, `!` reads a bit with a clock.
- `(0)` lists the macros: `(1)` is ISO 7816-3 ATR (reset the card, parse the
  reply), `(2)` is parse-only.  **Troubleshooting** →
  [edge-cases.md](edge-cases.md).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | present | present (default image) |
| speed menu | present | present |
| output-type menu | present | present |
| ISO 7816-3 ATR macros | present | present |
| `RAW1` binary sub-protocol | present | present |
| VPU source `e` | — (feed externally) | `e1` / `e2` / `e3` |

Full per-item detail and exact wording are in the topic files.

## Limits & out of scope

- **Not I2C.**  No acknowledge bits, no address scanner, no clock stretching,
  and no repeated-start model; none of the I2C macros exist here.
- No bus-reset or device-search macro.  `(0)` lists only the ATR macros; the
  firmware-level `#` reset belongs to the base layer.
- The smart-card use needs the card powered; the mode does not sense a card
  and does not report an error when none answers.
- The `RAW1` binary path has no ATR macro and no width setting.
- Shared prompt grammar (`r`, `W`, `P`, `(n)`, wrappers, `e`, `v`, bit ops,
  safe reset `#`, bootloader `$`) lives in
  [docs/generic/](../../../generic/README.md) and is not repeated here.

## Related docs

- [Wire protocol set](../README.md) — the cluster front door and the shared
  pull-up/VPU rules.
- [docs/generic/](../../../generic/README.md) — the shared prompt layer.
- [docs/SPEC.md](../../../SPEC.md) — the capability and function index.
- Sibling bus modes: [1-Wire set](../1wire/README.md) (the single-line
  sibling), [3-Wire set](../3wire/README.md) (shares the bit-bang core and the
  `RAW1` binary mode), and [I2C](../../i2c/README.md) (the acknowledged
  two-wire bus 2-Wire is *not*).
- [Raw-wire `RAW1` binary](../binary.md) — the shared framing; this set's
  [binary.md](binary.md) covers the 2-Wire specifics.
- [docs/protocol/binary/](../../binary/README.md) — the binary layer entry.
