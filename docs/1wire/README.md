# 1-Wire

**Status:** draft — available on v3 and v4; v3 is build-only and unverified.

1-Wire (Dallas/Maxim) is a single-wire bus: one data line plus ground.  This
mode drives that line, offers standard and overdrive speeds, ROM-search
macros, and a device roster.  This README is a map, not the manual — the
details live in the files below.

| File | Covers |
|---|---|
| [entry-and-speed.md](entry-and-speed.md) | entering 1-WIRE, the speed setup menu, the in-place `O` toggle |
| [wiring.md](wiring.md) | the data pin, the **required** pull-up, VPU feeding, v3/v4 |
| [bus-operations.md](bus-operations.md) | bus reset, device detection, error codes, read/write and bit ops |
| [macros.md](macros.md) | ROM macros 51/85/204/236/240, the device roster, the `(0)` list |
| [timings.md](timings.md) | standard/overdrive reset and bit-slot behaviour |
| [binary.md](binary.md) | the `1W01` binary sub-protocol (client/host API) |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter: `m` then `2`, or `m2`, or `m 2 1`; answer the speed menu
  (`1` standard, `2` overdrive).
- Wiring headline: **data on MOSI plus ground; a pull-up of 2 kΩ–10 kΩ
  (≤ 2 kΩ for parasitically powered parts) to the VPU voltage is required.**
- `W` engages the bus; put a voltage on VPU before `P` — on v4 `e2` selects
  the on-board 3.3 V source (`e3` is 5 V); `P` enables the pull-ups; reset
  with `[` or `{`.
- `r` reads a byte; `0xNN`, `0b…` or a decimal write one.
- `(51)` reads the ROM of a single device; `(240)` searches all devices.
- `O` switches standard ⇄ overdrive in place and prints the new speed; `v`
  prints the pin table (the data pin is shown as `OWD`).

On v3 there is no on-board source: feed the VPU pin (Vpullup) from an external
supply.

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | present (build-only here) | present |
| required pull-up | yes | yes |
| in-place `O` toggle | present | present |
| VPU source `e` | — (feed VPU externally) | `e1` / `e2` / `e3` |
| binary `1W01` read-peripherals | — | present |

Full per-item detail and the exact wording are in the topic files.

## Limits & out of scope

- No device-specific command layer (for example DS18B20 temperature
  conversion) is built in — 1-Wire here is a manual byte/bit bus plus the
  ROM/roster macros.
- Search and roster macros are documented from the firmware and are
  unverified (see [macros.md](macros.md)).
- Shared prompt grammar (`r`, `W`, `P`, `(n)`, wrappers, bit ops, safe
  reset `#`, bootloader `$`) lives in
  [the generic layer](../generic/README.md) and is not repeated here.