# SPI

**Status:** draft — v4 is the documented target; v3 is build-only
(unverified).

SPI is a four-wire synchronous bus (CS, MOSI, MISO, CLK) plus ground. This mode
is a master: it asserts chip select, clocks bytes out and in, changes the clock
polarity/edge/sample phase, and can sniff traffic on somebody else's SPI bus. It
has no device-specific command layer — it is a manual master plus macros and a
binary protocol.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering SPI (mode 5), the speed menu, the setup prompt chain, defaults, the one-line settings form, the buffer limit |
| [wiring.md](wiring.md) | CS/MOSI/MISO/CLK pins, direction, the pull-up and supply rules, the voltage limit, v3/v4 |
| [bus-operations.md](bus-operations.md) | CS assert/release, byte read/write, the `{` read-back form, a worked session |
| [macros.md](macros.md) | the `(0)` list, the sniff macros, the clock/polarity tweak macros, the removed macros |
| [sniffer.md](sniffer.md) | sniff formats, CS-low vs all-traffic, the hardware-CS behaviour, limits |
| [binary.md](binary.md) | the `SPI1` binary sub-protocol and a worked exchange |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter: `m` then `5`, or `m5`. Answer the setup chain — speed, clock
  polarity, clock edge, sample phase, CS polarity, output type.
- Wiring headline: **CS, MOSI, MISO, CLK and ground.** MOSI, CLK and CS are
  outputs; MISO is an input. **The default output type is open-drain, so a
  pull-up is required unless you select Normal output (option `2`)**.
- `[` asserts CS, `]` releases it; `{` asserts CS and makes every following
  write also report the byte clocked in. Type a number (`0xNN`) to write a
  byte; `r` reads a byte by sending `0xff`. A write prints `WRITE: 0xNN`; a
  read prints `READ: 0xNN`.
- `(1)` sniffs while CS is low, `(2)` sniffs all traffic, `(0)` lists the
  macros; macros 10–15 change the clock settings in place.
- The sniffer watches someone else's bus and is input-only, so **enter the
  sniffer before connecting the target**.
- **Troubleshooting** → [edge-cases.md](edge-cases.md): CS polarity confusion,
  a persisting `{` read-back, sniffer confusion, the buffer limits.

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | present (build-only) | present (default image) |
| speed menu (12 speeds) | yes | yes |
| sniffer (CS-low / all traffic) | yes (polled CS) | yes (hardware-latched CS) |
| clock/polarity tweak macros 10–15 | yes | yes |
| binary `SPI1` sub-protocol | yes | yes |
| extended AVR command `0x06` | yes | yes |

Full per-item detail and exact wording are in the topic files.

## Limits & out of scope

- No device-specific command layer (no flash-erase helper, no register decode).
  SPI here is a manual master plus the sniffer.
- The sniffer follows the clock edge and clock polarity, but not the input
  sample phase; its usable bus rate is not a fixed number — see
  [sniffer.md](sniffer.md).
- The older published "up to 10 MHz" sniffer claim is **unverified**.
- All v3 behaviour in this set is build-only and **unverified**.
- Shared prompt grammar (`r`, `:N`, `;N`, `W`, `P`, `(n)`, wrappers, `e`,
  bit ops, safe reset `#`, bootloader `$`) lives in `docs/generic/` and is not
  repeated here.