# JTAG

**Status:** OpenOCD path on v3 (build-only, unverified) and v4; the XSVF
player is v4 only.  v3 behaviour in this set is unverified.

JTAG is a four-signal debug and configuration bus: TDI, TDO, TCK and TMS,
with optional SRST and TRST.  The Bus Pirate drives it **only from the
binary layer** — there is no JTAG entry on the mode menu and no JTAG
command set at the terminal prompt.

Two binary paths share the same pins:

| Path | Entry in `BBIO1` | Identifier | TAP owner |
|---|---|---|---|
| OpenOCD path | `0x06` | `OCD1` | the host |
| XSVF player | `0x18` | `XSV1` | the player |

Both paths use the Bus Pirate as the electrical layer and byte transport;
the difference is who owns the TAP.  There is no standalone TAP-engine
mode: the OpenOCD path is a raw shifter and the XSVF player owns the TAP.

Naming: the **OpenOCD path** is the firmware mode; the host-side setup is
the **OpenOCD adapter** tool set.  The **XSVF player** is the firmware
sub-mode; the PC program that streams a file is the **XSVF host player**.
The player's identifier is `XSV1`.  The XSVF tool set calls these the
**firmware player** and the **host tools**; they are the same pair.

This set is for wiring a target and driving it through one of the two
paths.  The TAP mechanics — states, shifts, scan, IDCODE, BYPASS and reset
— belong here; host-tool installation, configuration and workflow live in
the tool sets linked at the end, not here.

This README is a map, not the manual; the details live in the files below.
Both paths are byte protocols, so the examples show host bytes and firmware
replies, not typed terminal commands.

| File | Covers |
|---|---|
| [entry.md](entry.md) | binary entry, identifiers, settings, exit rules |
| [wiring.md](wiring.md) | pin map, levels, TRST, pull-ups, supplies |
| [tap.md](tap.md) | TAP states, IR/DR shifts, scan/IDCODE, bypass, reset |
| [clock.md](clock.md) | TCK rate and delay, link speed, scan time |
| [openocd.md](openocd.md) | the `0x06` OpenOCD path byte protocol |
| [xsvf.md](xsvf.md) | the `0x18` XSVF player byte protocol |
| [edge-cases.md](edge-cases.md) | concrete failure modes and recovery |
| [../binary/README.md](../binary/README.md) | `BBIO1` entry, framing, replies |

## The five-minute version

- There is no JTAG menu entry.  Enter `BBIO1` with 20 × `0x00`, then send
  `0x06` (OpenOCD path) or `0x18` (XSVF player); the host tools do this for
  you.
- `0x06` answers `OCD1`; the pins are high-Z until `0x01 0x01` selects JTAG
  push-pull (or `0x01 0x02` open-drain).
- `0x18` answers `XSV1`, drives push-pull, and switches on the regulator
  that powers the `3.3V` and `5.0V` header rails.  It is v4 only; a v3
  build answers `0x00`.
- Wiring: MOSI→TDI, MISO←TDO, CLK→TCK, CS→TMS, plus ground.  SRST is on
  AUX; TRST is on AUX1 (v4) or PGD (v3).  Wire the target's VCC to `3.3V`
  or `5.0V`, never both.
- Pull-ups: push-pull needs none; open-drain needs 2–10 kΩ on TDI/TMS/TCK
  (and on SRST/TRST when used) to the target rail.
- Clock: `0x08` sets an OpenOCD-path TCK delay (default 1); the XSVF player
  has no TCK setting.  See [clock.md](clock.md).

## Leaving the modes

| Path | First step | Result |
|---|---|---|
| OpenOCD (`0x06`) | `0x00` | `BBIO1`; pins inputs, VREG and pull-ups off |
| XSVF (`0x18`) | `0x0F` | `BBIO1` silently; pins driven, VREG on |

From `BBIO1`, send `0x0F`: it replies `0x01` and runs the terminal reset
(v3: a hardware reset), which switches the target supply off.  The XSVF
player ignores `0x00`.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| OpenOCD path (`0x06`) | present, unverified | present |
| XSVF player (`0x18`) | — (answers `0x00`) | present |
| open-drain pin mode | present, unverified | present |
| XSVF entry enables VREG | — | yes |
| OpenOCD host | POSIX only (native Windows builds exclude the driver) | POSIX only |
| XSVF host | — | Windows or Linux |

The v3 column is **not verified on v3**.

Hardware: a Bus Pirate (v4 for the XSVF player), a USB cable, the target's
JTAG header and a common ground.  The OpenOCD path also needs a POSIX host
that speaks `OCD1`; native Windows builds of OpenOCD exclude the Bus Pirate
driver, and a Cygwin/POSIX layer may work.  The XSVF player needs a v4, a
Windows or Linux host, and an XSVF file.

## Limits & out of scope

- No terminal JTAG mode and no terminal command set.
- The OpenOCD path is a TAP shifter only: no IR/DR descriptions, no chain
  discovery, no IDCODE handling.  The host owns all of that.
- One `0x05` shift is clamped to 8192 bits; the echo reports the requested
  count.  See [openocd.md](openocd.md).
- The XSVF player supports the XSVF 5.01 command set, a 400-bit longest
  shift and 4096-byte chunks.  See [xsvf.md](xsvf.md).
- The XSVF player is **3.3 V push-pull only**: it has no open-drain mode and
  switches VREG on at entry.  A 1.8 V or 2.5 V target cannot use it; use the
  OpenOCD path with open-drain and external pull-ups.  See
  [wiring.md](wiring.md#power) and [xsvf.md](xsvf.md#limits).
- No JTAG self-test.  `~` and the binary `0x10`/`0x11` self-tests do not
  drive a chain.
- Boundary-scan instructions such as SAMPLE/PRELOAD and EXTEST are IR
  values supplied by the host or the XSVF file; the Bus Pirate does not
  interpret them.
- SWD is a different interface and is not implemented here.

## Related docs and tool sets

- [generic/README.md](../../generic/README.md) — prompt, serial settings,
  binary entry.
- [binary/README.md](../binary/README.md) — the `BBIO1` layer.
- [SPEC.md](../../SPEC.md) — the function and capability matrix.
- Tool sets:
  - [tools/openocd/README.md](../../tools/openocd/README.md) — host-side
    OpenOCD setup, configuration and workflow.
  - [tools/xsvf/README.md](../../tools/xsvf/README.md) — XSVF host player
    and SVF→XSVF converter.
