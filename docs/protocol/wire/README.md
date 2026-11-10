# Wire protocols (1-Wire / 2-Wire / 3-Wire)

**Status:** three separate raw bit-bus modes; all three are present on v3 and
v4, with v3 build-only here.

These three sets are **raw bit-bus** modes, not protocol engines.  Each drives
the Bus Pirate pins by hand, one bit at a time: none of them is I2C and none is
SPI.  Each has its own mode number and its own setup prompts.  This page is the
cluster's front door; the per-mode detail lives in the three sets below.

## Choose by target

| What you have / need | Use | Prompt + mode | Pins | Headline extra | Start |
|---|---|---|---|---|---|
| One data line + ground; Dallas/Maxim parts, ROM search or roster | 1-Wire | `1WIRE`, mode 2 | data on MOSI + GND | ROM-search macros and the device roster | [1-Wire set](1wire/README.md) |
| Clock + one bidirectional data line; smart card ATR or raw two-wire | 2-Wire | `2WIRE`, mode 6 | SDA (MOSI) / SCL (CLK) + GND (+ CS for the ATR) | ISO 7816-3 ATR macros | [2-Wire set](2wire/README.md) |
| Clock + separate data out/in + chip select; raw three-wire | 3-Wire | `3WIRE`, mode 7 | MOSI / MISO / CLK / CS + GND | none by design (primitive only) | [3-Wire set](3wire/README.md) |

**Not the same as:** 2-Wire is not I2C (no acknowledge bit, no addressing);
3-Wire is not SPI (no engine, no clock polarity/phase); 1-Wire is a manual
byte/bit bus plus ROM macros, not a device-command layer.

## Power, pull-ups and VPU

- **1-Wire:** a pull-up on the data line is **required** — 2 kΩ–10 kΩ, and
  2 kΩ or less for parasitically powered parts.
- **2-Wire / 3-Wire:** a pull-up is **conditional** — needed only with the
  **open-drain** output type (2 kΩ–10 kΩ); normal output needs none.
- Send `W` to engage the bus before any traffic; it starts the supplies and
  prints `Clutch engaged!!!`.  `w` stops them and parks the bus again.
- `P` connects the on-board pull-ups to the **Vpullup** pin, so a voltage must
  already be there.  On v4 select it with `e2` (3.3 V) or `e3` (5.0 V); on v3
  feed Vpullup from an external 0–5 V supply.  **Feed VPU before `P`**, or `P`
  only prints `Warning: no voltage on Vpullup pin`.
- `e` is accepted only while the pins are open-drain HiZ and no voltage is
  already on VPU.  If you fit your own external pull-up to a supply, you do not
  need `P`/VPU at all.
- The target's own supply (VCC) is separate from VPU: `W` powers the target,
  while VPU only sets the pull-up reference.
- **v3 is build-only** in this documentation: the v3 behaviour is taken from
  the build, not from hardware.  The v3 sets repeat that caveat.

The shared prompt layer (`W`, `w`, `P`, `e`, `v`, macros, bit operations) is in
[docs/generic/](../../generic/README.md) and is not repeated here.

## Naming

Firmware-displayed strings are the name authority.  One mode can appear under
more than one form; this is the mapping used across the cluster:

| Context | Form | Example |
|---|---|---|
| 1-Wire mode menu, banner, prompt | `1WIRE` | `1WIRE>` |
| 1-Wire prose | `1-Wire` | the 1-Wire bus |
| 1-Wire speed messages | `1-WIRE` | `1-WIRE speed: overdrive` |
| 2-Wire mode menu, banner, prompt | `2WIRE` | `2WIRE>` |
| 2-Wire library / settings header | raw 2-wire / `R2W` | `R2W (spd hiz)=( 1 1  )` |
| 3-Wire mode menu, banner, prompt | `3WIRE` | `3WIRE>` |
| 3-Wire library / settings header | raw 3-wire / `R3W` | `R3W (spd csl hiz)=( 1 1 1 )` |

## Binary paths

| Mode | Identifier | Entry | Page |
|---|---|---|---|
| 1-Wire | `1W01` | BBIO `0x04` | [1wire/binary.md](1wire/binary.md) |
| 2-Wire / 3-Wire | shared `RAW1` | BBIO `0x05` | [binary.md](binary.md) — shared framing; per-wire detail in [2wire/binary.md](2wire/binary.md) and [3wire/bus-operations.md](3wire/bus-operations.md) |

`RAW1` serves both raw-wire counts; its `0x8x` configuration byte selects
2-wire or 3-wire.  `1W01` is a separate command set.

## Start here

1. Pick the mode from the table above.
2. Wire it: data, clock and select plus ground, then the pull-up the mode needs.
3. Send `W`, then `e` on v4 → `P` when you use the on-board pull-up.
4. Open the set for its entry prompts, commands and edge cases.

## Availability

| | v3 | v4 |
|---|---|---|
| 1-Wire (mode 2) | present (build-only) | present |
| 2-Wire (mode 6) | present (build-only) | present |
| 3-Wire (mode 7) | present (build-only) | present |
| binary `1W01` / shared `RAW1` | present | present |

## Limits & out of scope

- These are raw bit buses: no protocol stack, no device drivers, no sniffers.
- 1-Wire has no device-specific command layer; 2-Wire has no I2C addressing or
  acknowledge; 3-Wire has no macros at all.
- Shared prompt grammar and the capability commands live in
  [docs/generic/](../../generic/README.md); the function index is
  [docs/SPEC.md](../../SPEC.md).

## Related docs

- [1-Wire set](1wire/README.md), [2-Wire set](2wire/README.md),
  [3-Wire set](3wire/README.md).
- [docs/generic/](../../generic/README.md) — the shared prompt and capability
  layer.
- [docs/protocol/binary/](../binary/README.md) — the BBIO byte layer.
- [docs/SPEC.md](../../SPEC.md) — the capability and function index.
