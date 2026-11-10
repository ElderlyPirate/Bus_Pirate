# JTAG

**Status:** draft — the v3 column is build-only (unverified); the XSVF player
is v4-only.

JTAG is a four-signal debug/configuration bus (TDI, TDO, TCK, TMS, plus an
optional reset).  The Bus Pirate drives JTAG **only from its binary layer**:
there is **no interactive JTAG mode** on the mode menu and no JTAG command set
at the terminal prompt.  Two host-driven binary paths exist:

- the **OpenOCD path** (binary bitbang command `0x06`) — a raw TAP shifter for
  a host debugger such as OpenOCD; and
- the **XSVF player** (binary bitbang command `0x18`, Bus Pirate v4 only) —
  plays a compiled Xilinx XSVF file to program or configure a CPLD/FPGA.

Both paths share the same physical pin mapping.  The host program owns the JTAG
intelligence; the Bus Pirate is the electrical layer and the byte transport.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [wiring.md](wiring.md) | the JTAG pin map, levels, pull-up requirements, v3/v4 |
| [openocd.md](openocd.md) | the `0x06` OpenOCD path: entry, command set, TAP shift, worked exchange |
| [xsvf.md](xsvf.md) | the `0x18` XSVF player: entry, player commands, stream framing, error codes, worked exchange |
| [edge-cases.md](edge-cases.md) | concrete failure modes and recovery |

## The five-minute version

- There is no JTAG menu entry.  Enter the binary bitbang layer (20 × `0x00` →
  `BBIO1`), then send `0x06` (OpenOCD path) or `0x18` (XSVF player).
- `0x06` prints `OCD1`; `0x18` prints `XSV1` and switches on the on-board
  voltage regulator.
- Wiring headline: **TDI→MOSI, TDO→MISO, TCK→CLK, TMS→CS**, plus ground;
  SRST/TRST sit on the auxiliary pins (see [wiring.md](wiring.md)).
- XSVF is **v4 only**; on v3 the `0x18` byte is answered with a failure.
- OpenOCD's open-drain pin mode needs **pull-ups (2–10 kΩ)**; the default
  push-pull mode and the XSVF player need **no pull-up**.
- There is no command grammar here — you drive the DUT with a host program
  (OpenOCD for the debug path; an XSVF player for the file path).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | present (build-only, unverified) | present |
| OpenOCD path (`0x06`) | present | present |
| open-drain pin mode | yes | yes |
| XSVF player (`0x18`) | — (byte answered with a failure) | present |
| XSVF entry enables the voltage regulator | — | yes |

The v3 column is **not verified** (build-only).

## Limits & out of scope

- **No terminal JTAG mode.**  The old terminal command set and the terminal
  "XSVF player" macro do not exist in this firmware; the player moved to the
  binary `0x18` path (see [xsvf.md](xsvf.md)).
- The OpenOCD path is a **TAP shifter only**: it has no knowledge of
  instruction registers, data registers, or the scan-chain topology.  The host
  owns all of that.
- The XSVF player's shift buffer caps the longest shift inside a file — see
  [xsvf.md](xsvf.md).  Large CPLD/FPGA configuration streams may not fit.
- **No JTAG self-test.**  The generic self-test (`~`, binary `0x10`/`0x11`)
  exercises power, ADC, EEPROM and the header pins; it does **not** drive or
  probe a JTAG chain.
- SWD (two-wire ARM debug) is a separate, planned function — not JTAG.

## Related docs

- [generic/README.md](../../generic/README.md) — the prompt and the binary entry
  sequence.
- [binary/README.md](../binary/README.md) — the binary bitbang layer.
- [SPEC.md](../../SPEC.md) — the capability matrix.
- Maintainer code map: [dev/code-map.md](../../dev/code-map.md).

## See also

- [tools/xsvf/README.md](../../tools/xsvf/README.md) — the XSVF player tool set.
- [tools/openocd/README.md](../../tools/openocd/README.md) — the OpenOCD adapter
  tool set.