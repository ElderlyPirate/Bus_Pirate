# JTAG

**Status:** current · two binary JTAG paths; the XSVF player is v4 only.

JTAG is a four-signal debug and configuration bus: TDI, TDO, TCK and TMS,
with optional SRST and TRST.  The Bus Pirate drives it **only from the
binary layer**.  There is no JTAG entry on the mode menu and no JTAG
command at the terminal prompt.

Two paths share the same pins:

| Path | Entry in `BBIO1` | Identifier | TAP owner | Board |
|---|---|---|---|---|
| OpenOCD path | `0x06` | `OCD1` | the host | v3, v4 |
| XSVF player | `0x18` | `XSV1` | the player | v4 |

Both use the Bus Pirate as the electrical layer and byte transport.  The
difference is who owns the TAP: the OpenOCD path is a raw shifter, and the
XSVF player walks the TAP itself from a file.  There is no standalone TAP
mode.

**Naming.**  The **OpenOCD path** is the firmware mode; the host-side
debugger setup is the **OpenOCD adapter**.  The **XSVF player** is the
firmware sub-mode; the PC command that streams a file is **`bp xsvf`**, the
**host player**.  The XSVF set calls the firmware sub-mode the **firmware
player**; it is the same object.  The identifiers are the firmware's own
strings: `OCD1` and `XSV1`.

This set is for wiring a target and driving it through one of the two
paths.  The TAP mechanics, bytes, clocking and failure modes are here; the
files below are the map.  Host-tool installation, configuration and
workflow live in the tool sets at the end.

| File | Covers |
|---|---|
| [entry.md](entry.md) | binary entry, identifiers, settings, exit and recovery |
| [wiring.md](wiring.md) | pins, power, levels, pull-ups, reset lines, v3/v4 |
| [tap.md](tap.md) | TAP states, shifts, scans, IDCODE, BYPASS, resets |
| [clock.md](clock.md) | TCK delay, link speed, scan time |
| [openocd.md](openocd.md) | the `0x06` command set and byte rules |
| [xsvf.md](xsvf.md) | the `0x18` player, stream handshake, results, host run |
| [edge-cases.md](edge-cases.md) | concrete failure modes and recovery |
| [../binary/README.md](../binary/README.md) | `BBIO1` entry and framing |

## Start here

| You have | Host | Goal | Path |
|---|---|---|---|
| v4 | Windows, Linux, macOS | configure a CPLD/FPGA from a file | XSVF player |
| v4 | Windows, Linux, macOS | scan or reset a chain | XSVF player |
| v3 or v4 | POSIX | debug or program a CPU | OpenOCD path |
| v3, or 1.8 V / 2.5 V on v4 | native Windows | any JTAG goal | no ready-made path |
| any board | POSIX | a 1.8 V or 2.5 V chain | OpenOCD path, open-drain |

A **POSIX host** here means Linux, macOS, BSD or Cygwin.  Native Windows
builds of OpenOCD do not include the Bus Pirate driver; WSL or a Linux VM
with USB passthrough is another route.  `bp xsvf` needs none of that.

First commands:

```
bp xsvf -p COM5 --scan --leave
bp xsvf -p COM5 -f design.xsvf -r --leave
openocd -f openocd.cfg
```

`bp` is installed from `scripts/bp` at the repository root; OpenOCD needs
the `buspirate` driver and a target configuration.  Host-side setup is owned
by the tool sets linked at the end.

## Worked examples

| Example | Where |
|---|---|
| Reset-default IDCODE read, 43 bits | [tap.md](tap.md#one-idcode-read) |
| IR load for BYPASS, 10 bits | [tap.md](tap.md#loading-an-instruction) |
| OpenOCD-path byte exchange | [openocd.md](openocd.md#worked-exchange) |
| `bp xsvf` scan and file run | [xsvf.md](xsvf.md#worked-sessions) |
| Every exit route | [entry.md](entry.md#leaving) |

## The five-minute version

- There is no JTAG menu entry.  Enter `BBIO1` with 20 × `0x00`, then send
  `0x06` (OpenOCD path) or `0x18` (XSVF player); the host tools do both
  steps for you.
- OpenOCD: use the `buspirate` adapter on a POSIX host.  The driver enters
  `BBIO1` and `OCD1` itself.
- XSVF: a v4 scans a chain with `bp xsvf -p COM5 --scan --leave`, or plays a
  file with `bp xsvf -p COM5 -f design.xsvf -r --leave`.
- Wiring: MOSI → TDI, MISO ← TDO, CLK → TCK, CS → TMS, plus ground.  SRST
  is `AUX`; TRST is `AUX1` (v4) or `PGD` (v3); both are optional.
- Power: VREG is off at binary entry.  Wire the target's VCC to `3.3V` or
  `5.0V` **only when the board powers the target** — never both rails, and
  never with the target's own supply still connected.  The XSVF player
  switches VREG on at entry and cannot leave it off.
- Pull-ups: none for push-pull.  Open-drain needs 2–10 kΩ on TDI/TMS/TCK
  (and on SRST/TRST when used) to the target rail.  The XSVF player is
  push-pull only.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| OpenOCD path (`0x06`) | present | present |
| XSVF player (`0x18`) | not present; answers `0x00` | present |
| open-drain pin mode | present | present |
| OpenOCD host driver | POSIX only | POSIX only |
| `bp xsvf` host command | no player to drive | Windows, Linux, macOS |

Of the two JTAG paths, the v3 build contains only the OpenOCD path, and
its hardware behaviour is not verified on v3.  Hardware: a Bus Pirate (v4
for the player), a USB cable, the target's JTAG header and a common ground.
The OpenOCD path also needs a POSIX host with the `buspirate` driver; the
player needs a v4, a Python 3.12+ host and an XSVF file (not SVF).

## Wiring (headline)

| Bus Pirate | Direction | JTAG | Required |
|---|---|---|---|
| MOSI | → | TDI | yes |
| MISO | ← | TDO | yes |
| CLK | → | TCK | yes |
| CS | → | TMS | yes |
| GND | — | GND | yes |
| AUX | → | SRST | optional |
| AUX1 (v4) / PGD (v3) | → | TRST | optional |

No pull-up is needed for a push-pull output.  The full map, the supply
rules and the reset-line behaviour are in [wiring.md](wiring.md).

## Leaving the paths

Every exit route is in [entry.md](entry.md#leaving).  The short form:
`0x00` leaves the OpenOCD path; `0x0F` leaves the player for `BBIO1`, and a
second `0x0F` returns to the terminal.  A reset button works from any
state.

## Limits & out of scope

- No terminal JTAG mode and no terminal command set.
- The OpenOCD path is a TAP shifter only: no IR/DR descriptions, no chain
  discovery, no IDCODE handling.  The host owns all of it.
- One `0x05` shift is clamped to 8192 bits; the echo repeats the requested
  count.  A longer payload desynchronises the next command.
- The XSVF player supports the XSVF 5.01 command set, a 400-bit longest
  shift and 4096-byte chunks.  See [xsvf.md](xsvf.md#limits).
- The XSVF player is **3.3 V push-pull only** and switches VREG on.  A
  1.8 V or 2.5 V target cannot use it; use the OpenOCD path with open-drain
  and pull-ups — from a POSIX host.
- No JTAG self-test.  `~` and the binary `0x10`/`0x11` self-tests do not
  drive a chain.
- Boundary-scan instructions such as SAMPLE/PRELOAD and EXTEST are IR
  values supplied by the host or the XSVF file.  The Bus Pirate does not
  interpret them.
- SWD is a different interface and is not implemented here.

## Related docs and tool sets

| Doc | Why |
|---|---|
| [generic/README.md](../../generic/README.md) | prompt, serial settings, terminal reset |
| [binary/README.md](../binary/README.md) | the `BBIO1` layer |
| [SPEC.md](../../SPEC.md) | function and capability matrix |
| [tools/openocd/README.md](../../tools/openocd/README.md) | host setup, configs, workflow |
| [tools/xsvf/README.md](../../tools/xsvf/README.md) | `bp xsvf`, conversion, workflow |
