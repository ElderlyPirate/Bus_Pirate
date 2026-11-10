# 3WIRE (raw 3-wire)

**Status:** available on v3 and v4; v3 is build-only and unverified.

Part of the [wire protocol set](../README.md).  The **raw 3-wire** library is a
generic bit-bus you drive by hand: it clocks bits out on one line, clocks bits
in on a second, toggles a clock line, and holds a chip-select line.  It is
**not SPI** — there is no SPI engine here, no clock-polarity/phase setting, no
byte-oriented framing, and **no macros**.  It is the bare set of clock,
data-out, data-in and select primitives, useful when a target speaks a
three-wire-ish protocol the firmware has no dedicated mode for (proprietary
shift-register buses, hand-timed sensors, a bus you are reverse-engineering).
For a real SPI target, use the SPI mode instead.

The mode displays as `3WIRE`; the library it implements is the "raw 3-wire"
library and the settings header abbreviates it `R3W`.  All three names refer
to the same mode throughout this set.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering `3WIRE`, the speed / CS / output-type menus, the one-line settings form, defaults |
| [wiring.md](wiring.md) | MOSI/MISO/CLK/CS pins, the output types, the pull-up statement, voltage limit, v3/v4 |
| [bus-operations.md](bus-operations.md) | CS control, byte read/write, the bitwise controls, no macros, the shared `RAW1` binary path |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter with `m` then `7`, or `m7` — 3-Wire is mode number **7**.  Answer the
  three prompts (speed, CS polarity, output type); the defaults are
  `~6kHz`, `/CS` and **open drain**.
- Wiring headline: **data out on MOSI, data in on MISO, clock on CLK, chip
  select on CS, plus ground**.  Open-drain output **needs a pull-up
  (2 kΩ–10 kΩ)**; normal output needs none.
- `W` engages the bus.  At engagement CLK and MOSI idle low and CS goes to its
  inactive level (`/CS` high, `CS` low), so the first `^` starts with a rising
  edge.  With open drain, `P` enables the pull-ups (feed VPU first — on v4
  with `e2`/`e3`).
- CS: with the default `/CS`, `[`/`{` assert CS **low** and print
  `/CS ENABLED`; `]`/`}` release it high and print `/CS DISABLED`.  With the
  `CS` option they assert CS **high** and print `CS ENABLED` / `CS DISABLED`.
  `{` also sets the read-back flag so a write reports the byte read from MISO.
- `r` reads a value (clocking `0xFF` out); `0xNN`, `0b…` or a decimal writes
  one.  A write is full duplex — data goes out on MOSI while MISO is sampled.
- Bitwise controls: `^` clock tick(s), `/` clock high, `\` clock low, `-` data
  high, `_` data low, `.` reads the MISO pin, `!` reads a bit with a clock.
- **No macros.**  `(0)` reports an error rather than a menu — see
  [bus-operations.md](bus-operations.md).  **Troubleshooting** →
  [edge-cases.md](edge-cases.md).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | present (build-only) | present (default image) |
| speed menu | present | present |
| CS-polarity menu | present | present |
| output-type menu | present | present |
| binary `RAW1` 3-wire config | present | present |
| `e` VPU source | — (feed externally) | present |

Full per-item detail and exact wording are in the topic files.

## Limits & out of scope

- **Not SPI.**  No SPI hardware engine, no mode/polarity/edge settings, no
  byte command set, no sniffer.  Use the SPI mode for SPI targets.
- **No macros by design.**  `(0)` does not list a macro menu here (see
  [bus-operations.md](bus-operations.md)); the mode is deliberately
  primitive.  Use the base-layer user macros `<n>` or a host script if you
  need repeatable sequences.
- No bus-reset or device-search command.  `BUS RESET` is a 1-Wire string, not
  a 3-Wire command; the firmware-level `#` reset belongs to the base layer.
- No address scanner, no acknowledge model, no clock-stretch handling.
- Shared prompt grammar (`r`, `W`, `P`, `(n)`, wrappers, `e`, `v`, bit ops,
  safe reset `#`, bootloader `$`) lives in `docs/generic/` and is not repeated
  here.

## Related docs

- [Wire protocol set](../README.md) — the cluster front door and the shared
  pull-up/VPU rules.
- [docs/generic/](../../../generic/README.md) — the shared prompt layer.
- [docs/SPEC.md](../../../SPEC.md) — the capability and function index.
- Sibling bus modes: [1-Wire set](../1wire/README.md) (the single-line
  sibling), [2-Wire set](../2wire/README.md) (shares the bit-bang core and the
  `RAW1` binary mode), and the [SPI mode](../../spi/README.md) (the engine this
  mode is not).
- [Raw-wire `RAW1` binary](../binary.md) — the shared framing; the 3-wire
  specifics are in [bus-operations.md](bus-operations.md) §Scripting.
- [docs/protocol/binary/](../../binary/README.md) — the binary layer entry.
