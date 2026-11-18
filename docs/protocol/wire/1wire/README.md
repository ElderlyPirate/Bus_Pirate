# 1-Wire (Dallas/Maxim)

**Status:** present in the v3 and v4 builds; the v3 hardware behaviour is not
verified.

Part of the [wire protocol set](../README.md).  1-Wire (Dallas/Maxim) is a
single-wire bus: one data line plus ground.  This mode drives that line, offers
standard and overdrive speeds, ROM-search macros, and a device roster.  This
README is a map, not the manual — the details live in the files below.

**Naming:** the firmware token is `1WIRE` (mode menu, banner and prompt); this
set writes `1-Wire` in prose and keeps `1-WIRE` inside the firmware's speed
messages (`1-WIRE speed: overdrive` / `1-WIRE speed: standard`).  The same
mapping is stated in [the generic layer](../../../generic/README.md).

| File | Covers |
|---|---|
| [entry-and-speed.md](entry-and-speed.md) | entering `1WIRE`, the speed setup menu, the in-place `O` toggle |
| [wiring.md](wiring.md) | the data pin, the **required** pull-up, parasitic power, strong pull-up, VPU feeding, v3/v4 |
| [bus-operations.md](bus-operations.md) | bus reset, device detection, error codes, read/write and bit operations |
| [macros.md](macros.md) | ROM macros 51/85/204/236/240, the device roster, the `(0)` list |
| [timings.md](timings.md) | AN126 reset and bit-slot behaviour, standard vs overdrive |
| [binary.md](binary.md) | the `1W01` binary sub-protocol |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter: `m` then `2`, or `m2` (answer the speed menu: `1` standard,
  `2` overdrive), or `m2 1` (speed supplied).  1-Wire is mode number **2**.
- Wiring: **data on MOSI plus ground; a pull-up of 2 kΩ–10 kΩ to the pull-up
  voltage is required** (2 kΩ or less for parasitically powered parts).
- `W` engages the bus.  Put a voltage on VPU before `P` — on v4 `e2` selects
  the on-board 3.3 V source (`e3` is 5 V) — then `P` enables the pull-ups.
- Reset with `[` or `{`.  `r` reads a byte; `0xNN`, `0b…` or a decimal writes
  one; a value always transfers as eight time slots.
- `(51)` reads the ROM of a single device; `(240)` searches all devices and
  fills the roster; `(236)` searches only alarm devices.
- `O` switches standard ⇄ overdrive in place and prints the new speed; it
  changes only the host timing, so send the Overdrive Skip ROM byte `0x3C` at
  standard speed first (see [entry-and-speed.md](entry-and-speed.md)).  `v`
  prints the pin table (the data pin is shown as `OWD`).

On v3 there is no on-board VPU source: feed the Vpullup pin from an external
0–5 V supply.

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | present | present |
| required pull-up | yes | yes |
| in-place `O` toggle | present | present |
| VPU source `e` | — (feed VPU externally) | `e1` / `e2` / `e3` |
| binary `1W01` pull-up feed (`0x5x`) | — (request returns `0x00`) | present |

Full per-item detail and the exact wording are in the topic files.

## Limits & out of scope

- No device-specific command layer (for example DS18B20 temperature
  conversion) is built in — 1-Wire here is a manual byte/bit bus plus the
  ROM/roster macros.
- There is no strong-pull-up command and the mode never drives the data line
  high; parasitic parts that need a strong pull-up require external circuitry
  (see [wiring.md](wiring.md) §Power and strong pull-up).
- The search/roster macros and the `1W01` binary path are **unverified**; see
  [macros.md](macros.md) and [binary.md](binary.md).
- Shared prompt grammar (`r`, `W`, `P`, `(n)`, wrappers, bit ops, safe reset
  `#`, bootloader `$`) lives in
  [the generic layer](../../../generic/README.md) and is not repeated here.

## Related docs

- [Wire protocol set](../README.md) — the cluster front door and the shared
  pull-up/VPU rules.
- [docs/generic/](../../../generic/README.md) — the shared prompt layer.
- [docs/SPEC.md](../../../SPEC.md) — the capability and function index.
- [2-Wire set](../2wire/README.md) and [3-Wire set](../3wire/README.md) — the
  sibling raw-wire modes.
- [Raw-wire `RAW1` binary](../binary.md) — the shared raw-wire binary path
  (1-Wire uses its own `1W01`).
