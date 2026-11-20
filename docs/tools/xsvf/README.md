# XSVF

**Status:** current — the v4 firmware player, the `bp xsvf` host player, the
SVF-to-XSVF conversion step, wiring and limits.

XSVF is the binary form of Xilinx's Serial Vector Format (SVF): a stream of
JTAG commands that programs or configures a CPLD or FPGA.  The Bus Pirate v4
firmware plays such a stream from binary sub-mode `0x18`; `bp xsvf` enters
that sub-mode and runs a reset, a chain scan or a file.

**Naming.**  The firmware sub-mode is the **firmware player** (`0x18`, answers
`XSV1`); the JTAG set calls the same thing the **XSVF player**.  The host
command is **`bp xsvf`**, also called the **host player**.  Its `--scan` option
uses the firmware's **chain probe** (`0x02`) and prints
`Chain Scan Result:`.  The SVF translator is **`svf2xsvf502`**, an external
Xilinx utility that is not part of this repository.

This README is a map, not the manual — details live in the files below.

| File | Covers |
|---|---|
| [workflow.md](workflow.md) | end to end: file, install, wire, scan, play, leave, recover |
| [players.md](players.md) | `bp xsvf`: install, options, port, output, exit status, chain scan, leave |
| [player.md](player.md) | the firmware player from the host side: TAP ownership, commands, handshake, results |
| [converter.md](converter.md) | SVF-to-XSVF conversion, family fit, the shift gate, merging a chain |
| [wiring.md](wiring.md) | five wires, rails and supply rules, levels, pull-ups, unused resets |
| [limits-and-errors.md](limits-and-errors.md) | firmware limits, result actions, host messages, recovery |

## The five-minute version

- **Install `bp`** (Python 3.12 or newer): create and activate a virtual
  environment in `scripts/bp`, then run `python -m pip install -e .`.  Run
  `bp ports` to list candidates
  ([players.md](players.md#install-and-port)).
- **Wire five signals.**  MOSI to TDI, MISO to TDO, CLK to TCK, CS to TMS,
  and ground to ground.  **No pull-up is needed.**  Wire the target's VCC to
  **one** rail only, `3.3V` or `5.0V`, and disconnect its own supply first.
- **Probe the chain:**

  ```
  bp xsvf -p COM5 --scan --leave
  ```

- **Play a file:**

  ```
  bp xsvf -p COM5 -f design.xsvf -r --leave
  ```

- **The file must fit.**  The longest single DR or IR shift is **400 bits**;
  chunks are **4096 bytes**.  Output is **3.3 V push-pull** only, so a 1.8 V
  or 2.5 V target cannot use this path.
- **The player is v4 only.**  Without `--leave` the board stays in the player
  and the rails stay on; press reset, or send `0x0F` twice from a raw-serial
  host.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| firmware player (`0x18`) | — (answers `0x00`) | present in the default v4 build |
| `bp xsvf` | stops with exit `2` | works |
| entry switches the regulator (VREG) on | — | yes |
| hardware needed | — | Bus Pirate v4, USB cable, JTAG header |

The v3 column is build-only: **not verified on v3**.  A v4 that does not
answer `XSV1` is not running an image with the player; flash a current v4
image ([flashing](../../dev/flashing.md)).  For a board or device outside
these limits, use OpenOCD's `svf` command through the `0x06` path
([OpenOCD set](../openocd/README.md)).

## Wiring (headline)

| Bus Pirate | Direction | JTAG |
|---|---|---|
| MOSI | to | TDI |
| MISO | from | TDO |
| CLK | to | TCK |
| CS | to | TMS |
| GND | — | GND |

**No pull-up needed:** TDI, TCK and TMS are 3.3 V push-pull outputs and TDO is
an input driven by the target.  The player does not drive SRST or TRST.
Supply and level detail: [wiring.md](wiring.md).

## Limits and out of scope

- **Longest single shift: 400 bits.**  Single-device XC9500/XL/XV streams
  fit; CoolRunner/II, PROM and FPGA configuration streams exceed the buffer
  and end with `XSVF_ERROR_DATAOVERFLOW`.
- **Chunk size: 4096 bytes.**  `bp xsvf` splits the file at this boundary;
  a custom host must not exceed it.
- **3.3 V push-pull only.**  No open-drain mode, no 1.8 V or 2.5 V targets.
- **v4 only.**  On v3, `0x18` is answered with `0x00`.
- **The chain scan is a wiring check, not an inventory.**  It reports at most
  63 devices and reads whatever the IDCODE path shifts out.
- **The converter is external.**  `bp` does not convert SVF; the firmware
  player reads XSVF, not SVF.
- **No host TCK setting.**  The player's clock is fixed by the firmware.

## Related docs

| Doc | Why |
|---|---|
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the full byte protocol, entry/exit and chain probe |
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | the authoritative pin map, levels, supplies, pull-ups |
| [protocol/jtag/tap.md](../../protocol/jtag/tap.md) | TAP states, shifts, IDCODE, bypass |
| [protocol/jtag/README.md](../../protocol/jtag/README.md) | the JTAG set overview and exit rules |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and framing |
| [../openocd/README.md](../openocd/README.md) | the v3 and larger-device alternative |
| [generic/README.md](../../generic/README.md) | prompt, serial settings, terminal reset |
| [SPEC.md](../../SPEC.md) | function and capability index |
