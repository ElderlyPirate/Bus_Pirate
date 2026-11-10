# JTAG

**Applies to:** v4 (documented); v3 is build-only (unverified).  The XSVF
player is v4-only.

JTAG is a four-signal debug/configuration bus (TDI, TDO, TCK, TMS, plus an
optional reset).  The Bus Pirate drives JTAG **only from its binary layer**:
there is **no interactive JTAG mode** on the mode menu and no JTAG command set
at the terminal prompt.  Two host-driven binary paths exist:

- the **OpenOCD path** (firmware mode, binary bitbang command `0x06`) — a raw
  TAP shifter for a host debugger such as OpenOCD; and
- the **XSVF player** (firmware sub-mode, binary bitbang command `0x18`, Bus
  Pirate v4 only) — plays a compiled Xilinx XSVF file to program or configure
  a CPLD/FPGA.

Naming: "OpenOCD path" is the firmware mode; the host-side setup is the
"OpenOCD adapter" tool set.  "XSVF player" is the firmware sub-mode; the host
program that streams a file is the "XSVF host player" tool.

For the OpenOCD path the host walks the TAP; for the XSVF path the firmware
player walks the TAP from the file.  In both cases the Bus Pirate is the
electrical layer and the byte transport.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [wiring.md](wiring.md) | the JTAG pin map, levels, pull-up requirements, v3/v4 |
| [openocd.md](openocd.md) | the `0x06` OpenOCD path: entry, commands, TAP shift |
| [xsvf.md](xsvf.md) | the `0x18` XSVF player: entry, commands, stream, result codes |
| [edge-cases.md](edge-cases.md) | concrete failure modes and recovery |
| [../binary/README.md](../binary/README.md) | the binary layer: entry, framing, reply conventions |

## The five-minute version

- There is no JTAG menu entry.  Enter the binary bitbang layer (20 × `0x00` →
  `BBIO1`), then send `0x06` (OpenOCD path) or `0x18` (XSVF player).
- `0x06` prints `OCD1`; `0x18` prints `XSV1` and switches on the on-board
  voltage regulator, which feeds the VCC pin (see [wiring.md](wiring.md)).
- Wiring headline: **TDI→MOSI, TDO→MISO, TCK→CLK, TMS→CS**, plus ground;
  **SRST on AUX0; TRST on AUX1 (v4) or PGD (v3)**.
- TAP in one line: TMS selects the next state; the sequence is
  reset → idle → shift IR/DR.  The host (OpenOCD) or the firmware player
  (XSVF) does it.
- OpenOCD entry leaves the pins high-Z; send `0x01 0x01` (JTAG push-pull)
  before a shift.  Push-pull needs **no pull-up**; open-drain
  (`0x01 0x02`) needs **2–10 kΩ** to the target rail.  The XSVF player drives
  push-pull and needs no pull-up.
- XSVF is **v4 only**; on v3 the `0x18` byte is answered with `0x00`.
- There is no command grammar here — you drive the DUT with a host program
  (OpenOCD for the debug path; an XSVF host player for the file path).

## Leaving the modes

| Path | Leave the mode | Then |
|---|---|---|
| OpenOCD (`0x06`) | `0x00` → `BBIO1`; pins/regulator reset | `0x0F` → `0x01`, terminal reset |
| XSVF (`0x18`) | `0x0F` → `BBIO1` silently; pins/VREG stay | `0x0F` → `0x01`, terminal reset |

The XSVF player ignores `0x00`.  The terminal reset returns every pin to an
input and switches the supplies off.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | present (build-only, unverified) | present |
| OpenOCD path (`0x06`) | present | present |
| open-drain pin mode | yes | yes |
| XSVF player (`0x18`) | — (byte answered with `0x00`) | present |
| XSVF entry enables the voltage regulator | — | yes |

The v3 column is **not verified** (build-only).

## Limits & out of scope

- **No terminal JTAG mode.**  The old terminal command set (including the
  `[` / `]` bit-bang commands) and the terminal "XSVF player" macro do not
  exist in this firmware; the player moved to the binary `0x18` path (see
  [xsvf.md](xsvf.md)).
- The OpenOCD path is a **TAP shifter only**: it has no knowledge of
  instruction registers, data registers, or the scan-chain topology.  The host
  owns all of that.
- The XSVF player's shift buffer caps the longest shift inside a file — see
  [xsvf.md](xsvf.md).  Large CPLD/FPGA configuration streams may not fit.
- **No JTAG self-test.**  The generic self-test (`~`, binary `0x10`/`0x11`)
  exercises power, ADC, EEPROM and the header pins; it does **not** drive or
  probe a JTAG chain.
- SWD (two-wire ARM debug) is a separate, planned function — not JTAG.

## Related docs and tool sets

- [generic/README.md](../../generic/README.md) — the prompt and the binary
  entry sequence.
- [binary/README.md](../binary/README.md) — the binary bitbang layer.
- [SPEC.md](../../SPEC.md) — the capability matrix.
- Tool sets (the host-side detail lives there):
  - [tools/xsvf/README.md](../../tools/xsvf/README.md) — the XSVF host player
    and the SVF→XSVF converter.
  - [tools/openocd/README.md](../../tools/openocd/README.md) — the OpenOCD
    adapter.
