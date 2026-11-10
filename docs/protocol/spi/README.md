# SPI

**Status:** draft — v4 is the documented target; v3 is build-only
(unverified).

SPI is a four-wire synchronous bus (CS, MOSI, MISO, CLK) plus ground. This mode
is a master: it asserts chip select, clocks bytes out and in, changes the clock
polarity/edge/sample phase, and can sniff traffic on somebody else's SPI bus. It
has no device-specific command layer in the interactive mode — it is a manual
master plus macros and a binary protocol. The binary `0x06` AVR bulk read is the
one exception (see [binary.md](binary.md)).

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering SPI (mode 5), the speed menu, the setup prompt chain, defaults, the one-line settings form |
| [wiring.md](wiring.md) | CS/MOSI/MISO/CLK pins, direction, the pull-up and supply rules, the voltage limit, v3/v4 |
| [bus-operations.md](bus-operations.md) | CS assert/release, byte read/write, the `{` read-back form, a worked session |
| [macros.md](macros.md) | the `(0)` list, the sniff macros, the clock/polarity tweak macros, the empty 3–9 range |
| [sniffer.md](sniffer.md) | sniff formats, CS-low vs all traffic, CN CS capture, integrity and stopping, limits |
| [binary.md](binary.md) | the `SPI1` binary sub-protocol and a worked exchange |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

Related docs: [generic/](../../generic/README.md) owns the shared prompt grammar
and the capability commands; [SPEC.md](../../SPEC.md) is the feature index;
[binary.md](binary.md) is the `SPI1` byte protocol.

## The five-minute version

- Enter: `m` then `5`, or `m5`. Answer the setup chain — speed, clock
  polarity, clock edge, input sample phase, CS polarity, output type.
- Run `W`: it enables the Bus Pirate's VCC supply **and** runs the mode's pin
  and module setup, so the bus is not configured until it runs. `W` is required
  even when the target is self-powered (leave VCC disconnected); it prints
  `POWER SUPPLIES ON` and `Clutch engaged!!!`.
- Wiring headline: **CS, MOSI, MISO, CLK and ground.** MOSI, CLK and CS are
  outputs; MISO is an input. **The default output type is open-drain, so a
  pull-up is required unless you select Normal output (option `2`)**.
- `[` asserts CS, `]` releases it; `{` asserts CS and makes every following
  write also report the byte clocked in. Type a number (`0xNN`) to write a
  byte; `r` reads a byte by sending `0xFF`. A write prints `WRITE: 0xNN`; a
  read prints `READ: 0xNN`.
- `(1)` sniffs while CS is low, `(2)` sniffs all traffic, `(0)` lists the
  macros; macros 10–15 change the clock polarity/edge and input sample phase
  in place.
- The sniffer watches someone else's bus and is input-only, so **enter the
  sniffer before connecting the target**, and **stop it only when the
  external master is quiet** — exiting restores the master pin configuration.
- **Troubleshooting** → [edge-cases.md](edge-cases.md): a missing `W`, CS
  polarity confusion, a persisting `{` read-back, sniffer overflow.

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | present (build-only) | present (default image) |
| speed menu (12 speeds) | yes | yes |
| sniffer (CS-low / all traffic) | yes (polled CS) | yes (CN CS capture) |
| clock/polarity tweak macros 10–15 | yes | yes |
| binary `SPI1` sub-protocol | yes | yes |
| extended AVR command `0x06` | yes | yes |

Full per-item detail and exact wording are in the topic files.

## Limits & out of scope

- No device-specific command layer in the interactive mode (no flash-erase
  helper, no register decode); SPI here is a manual master plus the sniffer.
  The binary `0x06` AVR bulk read is the documented exception.
- The sniffer follows the clock polarity and edge, but not the input sample
  phase. No maximum bus rate is specified: a stream that outruns the capture
  path is reported or aborted, so do not assume a 10 MHz bus can be captured
  — see [sniffer.md](sniffer.md).
- All v3 behaviour in this set is build-only and **unverified**.
- Shared prompt grammar (`r`, `:N`, `;N`, `W`, `P`, `(n)`, wrappers, `e`,
  bit ops, safe reset `#`, bootloader `$`) lives in `docs/generic/` and is not
  repeated here.
