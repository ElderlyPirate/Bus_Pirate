# XSVF host player

**Status:** v4 firmware only — the host-side tool set for the XSVF firmware
player: two host-player builds and the SVF→XSVF converter.

XSVF is the compressed binary form of Xilinx's Serial Vector Format (SVF): a
file of JTAG commands that configures or programs a CPLD/FPGA.  The firmware
plays such a file over JTAG from its binary sub-mode; the host programs in
this set stream the file to it and report the result.  The firmware sub-mode
itself — entry, player bytes, chunk framing and result codes — is the
[JTAG set's XSVF page](../../protocol/jtag/xsvf.md); this set owns the host
tools and the end-to-end workflow.

Naming: the firmware sub-mode is the **firmware player** (binary `0x18`,
answers `XSV1`); the PC programs are the **host tools**, two builds of the
**host player** — the **Windows player** and the **cross-platform player**.
Both are the same command-line tool with the same options.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [players.md](players.md) | the two host-player builds: what ships, which to use, options, serial settings, worked command |
| [converter.md](converter.md) | `svf2xsvf502` and `mergexsvf`: SVF→XSVF, family recipes, `-a` text listing |
| [workflow.md](workflow.md) | end to end: prepare, wire, connect, run, watch, exit/recover |
| [limits-and-errors.md](limits-and-errors.md) | firmware limits, result messages, tool quirks, recovery |

## The five-minute version

- **Start here:** if you already have an XSVF file, go to
  [workflow.md](workflow.md); if you have only an SVF, go to
  [converter.md](converter.md).
- The firmware player is **v4 only**; the host tools enter the binary layer
  and the firmware player itself (NUL bytes → `BBIO1`, `0x18` → `XSV1`),
  then send `0x03` and feed the file in chunks.
- Get the file from iMPACT (recommended) or convert an SVF with
  `svf2xsvf502`; the firmware player supports the XSVF 5.01 command set.
- Wire TDI→MOSI, TDO→MISO, TCK→CLK, TMS→CS, plus ground.  **No pull-up
  is needed** (the firmware player drives push-pull), and SRST/TRST are not
  used.
- Entry enables the on-board regulator, which feeds **VCC**.  **Do not also
  power the target externally while the player runs.**
- Close the terminal first, then run a host player with the port and file.
  Both builds accept the same options: `-p` port, `-s` speed, `-f` file, `-x`
  chain scan, `-r` chain reset.  On Windows prefer the Windows player (its
  `-x` scan is bounded).
- A clean run ends with `XSVF_ERROR_NONE` and `Success!`.  The host tool does
  not leave the firmware player: **press reset** afterwards.  `0x0F` twice
  works only once the firmware player is back in its command loop
  ([workflow.md](workflow.md) §6).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| firmware XSVF player (`0x18`) | — (answers `0x00`) | present |
| these host tools | no v3 player to drive | usable |
| entry enables the regulator | — | yes |

Hardware: a Bus Pirate v4, a USB cable, the target CPLD/FPGA on its JTAG
header, and a Windows or Linux host.  The firmware player feeds the target's
VCC from the on-board regulator; the target's own supply must stay
disconnected during the run.

## Wiring (headline)

| Bus Pirate | JTAG |
|---|---|
| MOSI | TDI |
| MISO | TDO |
| CLK | TCK |
| CS | TMS |
| GND | GND |

**No pull-up needed** — the firmware player drives TDI/TCK/TMS push-pull.
SRST/TRST are not used.  The full pin map, levels and v3/v4 differences are
in [wiring.md](../../protocol/jtag/wiring.md).

## Limits & out of scope

- **Longest single shift: 400 bits** (50 bytes).  XC9500/XL/XV-class streams
  fit; CoolRunner/II and FPGA configuration streams need longer shifts and
  end with `XSVF_ERROR_DATAOVERFLOW`.
- **Chunk size: 4096 bytes** — the host tools use exactly this; a custom host
  must not send more.
- **v4 only.**  On v3 the `0x18` byte is answered with `0x00` and the tools
  stop with an "Unable to activate … XSVF player mode" message.
- The host tools **do not exit the firmware player**; after a normal run the
  board stays in the player loop with the regulator on.  **Press reset** to
  leave it; `0x0F` twice only after the firmware player has returned to its
  command loop.
- The converter is a separate Xilinx utility; the firmware player is not an
  SVF player and cannot read `.svf` files directly.
- The 400-bit shift and 4096-byte chunk behaviour are code-derived and
  **unverified** on hardware.

## Related docs

| Doc | Why |
|---|---|
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware `0x18` player: entry, commands, stream framing, result codes |
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | pin map, levels, pull-ups, VREG/VCC |
| [protocol/jtag/README.md](../../protocol/jtag/README.md) | the JTAG set overview |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry, reply conventions, framing |
| [protocol/binary/README.md](../../protocol/binary/README.md) | the binary layer |
| [generic/README.md](../../generic/README.md) | prompt, serial settings, binary entry |
| [../openocd/README.md](../openocd/README.md) | the sibling JTAG tool (host-driven TAP shifter) |
| [../../SPEC.md](../../SPEC.md) | function and capability index |
