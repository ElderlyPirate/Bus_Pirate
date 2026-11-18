# XSVF host tools

**Status:** the firmware player is v4 only; this set is the host side: two
host players, the SVF→XSVF converter, and the end-to-end run.

XSVF is the binary form of Xilinx's Serial Vector Format (SVF): a file of
JTAG commands that programs or configures a CPLD/FPGA.  The firmware plays
such a file over JTAG from binary sub-mode `0x18`; the programs in this set
stream the file to it and report the result.

**Naming.**  The firmware sub-mode is the **firmware player** (`0x18`,
answers `XSV1`); the JTAG set calls the same thing the **XSVF player**.  The
PC programs are the **host players** — two builds of one tool: the **Windows
player** (`BP4_Full_bitbang_XSVFplayer_Win.exe`) and the **cross-platform
player** (`BPXSVFplayer`).  Together with the converter they are the **host
tools**.  The host programs' banner also reads `BusPirate XSVF Player`; that
is the PC tool.  The firmware mode answers only `XSV1`.  The byte-level
player protocol is owned by the
[JTAG XSVF page](../../protocol/jtag/xsvf.md); this set owns the host side.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [player.md](player.md) | the firmware player from the tool side: entry/exit, commands, run flow, results |
| [players.md](players.md) | the two host-player builds: files, options, serial settings, chain scan, output, exit status |
| [converter.md](converter.md) | `svf2xsvf502` and `mergexsvf`: SVF→XSVF, family recipes, the shift-size check |
| [workflow.md](workflow.md) | end to end: file, prepare, run, probe and failure sessions, leave the player |
| [wiring.md](wiring.md) | five wires, rail and supply rules, levels, pull-ups, unused resets |
| [limits-and-errors.md](limits-and-errors.md) | firmware limits, result messages, host quirks, recovery |

## The five-minute version

- **Start here:** an XSVF file in hand → [workflow.md](workflow.md); only an
  SVF → [converter.md](converter.md).
- A host player enters the binary layer (`BBIO1`), sends `0x18` and waits for
  `XSV1`, then sends `0x03` and feeds the file in 4096-byte chunks.
- Wire MOSI → TDI, MISO ← TDO, CLK → TCK, CS → TMS, plus ground.  **No
  pull-up is needed.**  Target VCC goes to the `3.3V` rail **or** the `5.0V`
  rail, never both, and the target's own supply stays disconnected.
- **3.3 V push-pull only.**  The player has no open-drain mode, so a 1.8 V or
  2.5 V target cannot use it.
- The file's longest single shift must be **400 bits or less**.
- The host players do not leave the firmware player.  **Press reset** to get
  the terminal back; the rails go off.  To re-power them, enter a mode (`m`)
  and then `W`; `W` also runs that mode's setup and is refused in `HiZ>`.

## Get the tools

The host players and the converters are in the firmware repository under
`scripts/BP4_XSVFplayer_Win/` and `scripts/BPXSVFPlayer/`
([repository README](../../../README.md)).  The repository does not install
them: run the executable from its folder, or build it with the included
project or `Makefile`.  No configuration file is needed; the serial port is
the only setup.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| firmware player (`0x18`) | — (answers `0x00`) | present |
| these host players | nothing to drive | usable |
| entry switches the regulator (VREG) on | — | yes |

**v3:** there is no XSVF player.  Use OpenOCD's own `svf` command through the
`0x06` path ([the OpenOCD set](../openocd/README.md)); it plays SVF, not
XSVF.

Hardware: a Bus Pirate v4, a USB cable, the target's JTAG header and a common
ground, plus a Windows or Linux host.

## Wiring (headline)

| Bus Pirate | Direction | JTAG |
|---|---|---|
| MOSI | → | TDI |
| MISO | ← | TDO |
| CLK | → | TCK |
| CS | → | TMS |
| GND | — | GND |

**No pull-up needed** — TDI, TCK and TMS are 3.3 V push-pull outputs and TDO
is an input.  SRST and TRST are not used.  Supply and level detail:
[wiring.md](wiring.md).

## Limits & out of scope

- **Longest single shift: 400 bits.**  XC9500/XL/XV-class streams fit;
  CoolRunner/II and FPGA configuration streams end with
  `XSVF_ERROR_DATAOVERFLOW`.
- **Chunk size: 4096 bytes.**  The host players use this as the maximum; the
  last chunk is the remainder.  A custom host must not exceed it.
- **3.3 V push-pull only.**  No open-drain mode, no 1.8 V or 2.5 V targets.
- **v4 only.**  On v3 the `0x18` byte is answered with `0x00`.
- **The host players do not exit the player.**  Press reset afterwards; a
  raw-serial host can send `0x0F` twice at the player's command loop
  ([workflow.md](workflow.md#6-leave-the-player)).
- **A stall inside an `XCOMMENT` string never returns.**  Only a board reset
  exits it; see [limits-and-errors.md](limits-and-errors.md).
- The converter is a separate Xilinx utility.  The player reads XSVF, not
  SVF.
- The 400-bit and 4096-byte limits are code-derived and **unverified** on
  hardware.

## Related docs

| Doc | Why |
|---|---|
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player: full byte protocol, probe, result codes |
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | the authoritative pin map, levels, supplies, pull-ups |
| [protocol/jtag/tap.md](../../protocol/jtag/tap.md) | TAP states, shifts, IDCODE, bypass |
| [protocol/jtag/README.md](../../protocol/jtag/README.md) | the JTAG set overview and exit rules |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and reply framing |
| [../openocd/README.md](../openocd/README.md) | the v3 and larger-device alternative |
| [generic/README.md](../../generic/README.md) | prompt, serial settings, binary entry |
| [SPEC.md](../../SPEC.md) | function and capability index |
