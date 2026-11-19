# SPI

**Status:** v3 and v4 documented; the v3 hardware behaviour is not verified.

SPI is a four-wire synchronous bus plus ground. In this mode the Bus Pirate is
the master: it drives CS, CLK and MOSI, reads MISO, and can change the clock
polarity, edge and sample phase. It can also listen passively to somebody
else's SPI bus. The terminal mode is a manual master — you assemble a
transaction from chip select, byte writes and reads, and macros. The binary
`SPI1` sub-protocol adds bulk transfers, configuration commands and one
AVR flash-read helper.

This README is a map, not the manual; the details live in the files below.
**Start here:** [The five-minute version](#the-five-minute-version) for entry,
wiring and one transaction.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering SPI (mode 5), the speed menu, the setup prompt chain, defaults, the one-line settings form, bring-up with `W` |
| [wiring.md](wiring.md) | CS/MOSI/MISO/CLK pins, directions, pull-up and supply requirements, VPU, the voltage limit, the `v` pin columns |
| [bus-operations.md](bus-operations.md) | CS assert/release, byte read/write, the `{` read-back form, bit/word limits, a worked session |
| [macros.md](macros.md) | the `(0)` list, the sniff macros, the clock/sample tweak macros, the empty 3–9 range |
| [sniffer.md](sniffer.md) | sniff formats, CS-low vs all traffic, the CN CS path, integrity, stopping, limits |
| [binary.md](binary.md) | the `SPI1` binary sub-protocol, configuration, bulk transfers, worked frames |
| [edge-cases.md](edge-cases.md) | concrete failure modes and quirks by topic |

## The five-minute version

- Enter with `m5` (or `m` then `5`), answer the six prompts, then run `W`.
  `W` enables the supplies **and** runs the pin/module setup, so the bus is not
  configured until it succeeds. It is required even when the target is
  self-powered; leave the 3.3V and 5.0V pins disconnected in that case. On v4,
  for on-board pull-ups select the Vpullup source with `e2`/`e3` **before**
  `W`, then enable `P`; match the source to the target's logic voltage (see
  [wiring.md](wiring.md#bring-up-order-with-the-on-board-pull-ups-v4)).
- Wiring: **CS, CLK and MOSI are outputs; MISO is an input; ground is common.**
  The default output type is open drain, so **a pull-up is required unless you
  select `Normal (H=3.3V, L=GND)` at the output-type prompt**. Details in
  [wiring.md](wiring.md).
- `[` asserts CS, `]` releases it; `{` asserts and makes every following write
  also report the byte clocked in. The flag is sticky: `]`/`}` and `w`/`W` do
  not clear it; `[` does. A number writes one byte; `r` reads one byte by
  sending `0xFF`. A write prints `WRITE: 0xNN`; a read prints `READ: 0xNN`.
- `(0)` lists the macros. `(1)` sniffs while CS is low, `(2)` sniffs all
  traffic; macros 10–15 change the clock polarity/edge and input sample phase
  in place.
- Binary: 20 NUL bytes enter `BBIO1`, then `0x01` enters `SPI1`. `0x02` asserts
  CS and enables the output; `0x03` releases it (input/Hi-Z in open drain,
  driven high in Normal). `0x04`/`0x05` are write-then-read, `0x1x` is bulk,
  `0x4x` sets power/pull-ups/AUX, `0x6x` sets speed and `0x8x` sets
  clock/output options. Open drain needs pull-ups in binary too — or select
  Normal with `0x8x`.
- Sniffer: enter it **before** connecting the target, and stop it only when the
  external master is quiet. No maximum bus rate is specified; see
  [sniffer.md](sniffer.md).

## Availability (summary)

| Item | v3 | v4 |
|---|---|---|
| build | included in the default build | included in the default build |
| terminal mode (`m5`) | yes | yes |
| 12-speed table | yes | yes |
| CS / byte read/write / read-back | yes | yes |
| macros 0–2 and 10–15 | yes | yes |
| sniffer | yes, polled CS | yes, CN CS capture (bounded) |
| binary `SPI1` | yes | yes |
| `0x5x` VPU source select | no (`0x00`) | yes |
| `0x06` AVR bulk read | yes | yes |
| terminal output buffer | 4096 B | 4096 B |

The v3 column is **not verified**.

## Names used in this set

| Firmware form | Meaning |
|---|---|
| `SPI` / `SPI>` | terminal mode name / prompt |
| `SPI1` | binary sub-protocol identifier returned by `0x01` |
| `CS` / `/CS` | active-high / active-low chip select; the status line adds `/` only for `/CS` |
| `spd` | settings-header speed field: the menu number `1`–`12` |
| `hiz` | settings-header output-type field: `1` open drain, `0` normal |
| `Vpullup` (`Vpu`, `VPU`) | pull-up reference: the `Vpu` header pin, the `VPU` column in `v` |

The settings header is `SPI (spd ckp ske smp csl hiz)=( 4 0 1 0 1 0 )`; the
fields are explained in [entry-and-setup.md](entry-and-setup.md).

**Buffers.** The *terminal output buffer* is the 4096 B queue that holds
output until the host link drains it. The *command buffer* is the input line
buffer, 128 B on v3 and 256 B on v4. They are separate buffers.

## Limits & out of scope

- **Byte-wide only.** SPI clocks eight bits per value and per read. There are
  no bit-level or 16-bit transfer commands; the shared width setting changes
  the displayed width, not the number of clocks — see
  [bus-operations.md](bus-operations.md).
- **No device-specific command layer in the terminal mode** (no flash erase,
  no register decode). The binary `0x06` AVR bulk read is the one exception
  ([binary.md](binary.md)).
- The sniffer follows the clock polarity and edge but not the input sample
  phase. No maximum bus rate is specified; a stream that outruns the capture
  path is reported or aborted. Do not assume a 10 MHz bus can be captured.
- Shared prompt grammar (`r`, `:N`, `;N`, `W`/`w`, `P`/`p`, `e`, `v`,
  `o`, `l`/`L`, `(n)`, wrappers, `#`, `$`) lives in
  [docs/generic/](../../generic/README.md) and is not repeated here.

## Related docs

- [generic/](../../generic/README.md) — prompt grammar, `W`/`w`, `P`/`p`,
  `e`, `v`, reset and bootloader.
- [binary/](../binary/README.md) — BBIO1 entry, framing, shared command groups.
- [edge-cases.md](edge-cases.md) — concrete failure modes and recovery.
- [SPEC.md](../../SPEC.md) — the function and capability index.
- [BASIC bus access](../../tools/basic/bus-access.md) — the `MODE` statement,
  which applies a mode's defaults without the terminal `W` step.
