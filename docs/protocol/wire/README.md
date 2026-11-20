# Wire protocols (1-Wire / 2-Wire / 3-Wire)

**Status:** three separate raw bit-bus modes; present in the v3 and v4
builds.  The v3 hardware behaviour is not verified.

These three sets are **raw bit-bus** modes, not protocol engines.  Each drives
the Bus Pirate pins by hand, one bit at a time: none of them is I2C and none is
SPI.  Each has its own mode number, its own setup prompts and its own limits.
This page is the cluster's front door; the per-mode detail lives in the three
sets below.

## Which mode fits the target

| What you have / need | Use | Prompt + mode | Pins | Headline extra | Start |
|---|---|---|---|---|---|
| One data line + ground; Dallas/Maxim parts, ROM search or a device roster | 1-Wire | `1WIRE`, mode 2 | data on MOSI + GND | ROM-search macros and the device roster | [1-Wire set](1wire/README.md) |
| Clock + one bidirectional data line; smart-card ATR or raw two-wire | 2-Wire | `2WIRE`, mode 6 | SDA (MOSI) / SCL (CLK) + GND (+ CS for the ATR) | ISO 7816-3 ATR macros | [2-Wire set](2wire/README.md) |
| Clock + separate data out/in + chip select; raw three-wire | 3-Wire | `3WIRE`, mode 7 | MOSI / MISO / CLK / CS + GND | none by design (primitive only) | [3-Wire set](3wire/README.md) |

**Not the same as:** 2-Wire is not I2C (no acknowledge bit, no addressing);
3-Wire is not SPI (no engine, no clock-polarity/phase settings); 1-Wire is a
manual byte/bit bus plus ROM macros, not a device-command layer.

## The file map

| File | Covers |
|---|---|
| [binary.md](binary.md) | the shared `RAW1` binary framing for 2-Wire and 3-Wire, the bulk groups, and the differences from the terminal wrappers |
| [1wire/README.md](1wire/README.md) | the 1-Wire set: entry, wiring, bus operations, ROM macros, AN126 timings, `1W01` binary |
| [2wire/README.md](2wire/README.md) | the 2-Wire set: entry, wiring, bus operations, ATR macros, smart cards, `RAW1` specifics |
| [3wire/README.md](3wire/README.md) | the 3-Wire set: entry, wiring, CS polarity, clock edges |
| [3wire/binary.md](3wire/binary.md) | the 3-Wire side of the shared `RAW1` binary path, with a worked exchange |

## Power, pull-ups and VPU

Pull-up rule:

| Mode | Pull-up | Value |
|---|---|---|
| 1-Wire | **required** (open-drain bus) | 2 kΩ–10 kΩ; 2 kΩ or less for parasitically powered parts |
| 2-Wire | **conditional** — open-drain output only | one 2 kΩ–10 kΩ resistor from each of SDA and SCL to the pull-up voltage |
| 3-Wire | **conditional** — open-drain output only | 2 kΩ–10 kΩ to the pull-up voltage |

Shared rules:

- `W` starts the on-board 3.3 V and 5.0 V rails and runs the mode's bus setup;
  it prints `POWER SUPPLIES ON` then `Clutch engaged!!!`.  `w` parks the bus
  again and prints `Clutch disengaged!!!` then `POWER SUPPLIES OFF`.
- `P` connects the on-board pull-ups to the **Vpullup** pin (the pin marked
  `Vpu` on the reference card).  A voltage must already be on that pin for the
  pull-ups to do anything.
- On v4 select the VPU source with `e2` (on-board 3.3 V) or `e3` (on-board
  5 V).  **Feed VPU before `P`**; otherwise `P` prints
  `Warning: no voltage on Vpullup pin` after switching the pull-ups on.  On v3
  there is no `e` command: feed Vpullup from an external 0–5 V supply.
- The target's VCC is separate from VPU.  `W` puts the on-board rails on their
  header pins; VPU only sets the reference the pull-ups tie to.
- If you fit your own external pull-up from the bus to a supply, you do not
  need `P` or VPU at all.

The shared prompt layer (`W`, `w`, `P`, `e`, `v`, macros, bit operations) is in
[docs/generic/](../../generic/README.md) and is not repeated here.

## Naming

Firmware-displayed strings are the name authority.  The mapping used across the
cluster:

| Context | Form | Example |
|---|---|---|
| 1-Wire mode menu, banner, prompt | `1WIRE` | `1WIRE>` |
| 1-Wire prose | `1-Wire` | the 1-Wire bus |
| 1-Wire speed messages only | `1-WIRE` | `1-WIRE speed: overdrive` |
| 2-Wire mode menu, banner, prompt | `2WIRE` | `2WIRE>` |
| 2-Wire library / settings header | raw 2-wire / `R2W` | `R2W (spd hiz)=( 1 1  )` |
| 3-Wire mode menu, banner, prompt | `3WIRE` | `3WIRE>` |
| 3-Wire library / settings header | raw 3-wire / `R3W` | `R3W (spd csl hiz)=( 1 1 1 )` |

`1WIRE`, `2WIRE` and `3WIRE` are the firmware tokens; `1-Wire`, `2-Wire` and
`3-Wire` are prose.  The lower-case forms `raw 2-wire` and `raw 3-wire` name
the library, and `2 wires` / `3 wires` is the binary wire-count setting;
neither is the mode name.  The 2-Wire header has **two spaces** before `)`; the
3-Wire header has one.  Both are exact; the entry pages explain the fields.

## Binary paths

| Mode | Identifier | Entry | Page |
|---|---|---|---|
| 1-Wire | `1W01` | BBIO `0x04` | [1wire/binary.md](1wire/binary.md) |
| 2-Wire / 3-Wire | shared `RAW1` | BBIO `0x05` | [binary.md](binary.md); per-wire detail in [2wire/binary.md](2wire/binary.md) and [3wire/binary.md](3wire/binary.md) |

`RAW1` serves both raw-wire counts; its `0x8x` configuration byte selects
2 wires or 3 wires, and it starts with 2 wires.  `1W01` is a separate command
set with its own reset, read, bulk-write and search commands.

## Start here

1. Pick the mode from the table above.
2. Wire it: 1-Wire — data + ground.  2-Wire — SDA + SCL + ground (CS only for
   the ATR).  3-Wire — MOSI + MISO + CLK + CS + ground.  Fit the pull-up the
   mode needs: 1-Wire always; 2-Wire and 3-Wire only in open drain.
3. Send `W`.  On v4 in open drain, select the VPU source with `e2`/`e3` and
   then `P` if you use the on-board pull-up; in normal output skip both.
4. Open the set for its entry prompts, commands and edge cases.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| 1-Wire (mode 2) | present | present |
| 2-Wire (mode 6) | present | present |
| 3-Wire (mode 7) | present | present |
| 1-Wire binary `1W01` | present | present |
| shared raw-wire binary `RAW1` | present | present |
| VPU source `e` | — (feed externally) | `e1` / `e2` / `e3` |

These rows cover both firmware builds; a `—` cell means the feature is absent
from that build.  The v3 hardware behaviour is not verified.

## Limits & out of scope

- These are raw bit buses: no protocol stack, no device drivers, no sniffers.
- 1-Wire has no device-specific command layer (for example DS18B20
  temperature conversion); 2-Wire has no I2C addressing or acknowledge;
  3-Wire has no macros at all.
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
