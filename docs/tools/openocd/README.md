# OpenOCD adapter

**Scope:** host-side setup for the JTAG OpenOCD path (binary `0x06`).

The OpenOCD adapter is the host half of the Bus Pirate's JTAG debug path.  The
firmware's `0x06` mode is a **raw TAP shifter**: the host debugger walks the
TAP and the Bus Pirate supplies the pins and the byte transport.  OpenOCD's
Bus Pirate interface is the reference host driver for it.

Naming: the **OpenOCD path** is the firmware mode (`0x06`, answers `OCD1`);
the **OpenOCD adapter** is this host-side tool set.  The byte-level command
set (entry, pin modes, features, TAP shift, speed, delay) is owned by
[protocol/jtag/openocd.md](../../protocol/jtag/openocd.md); this set owns host
setup, configuration, workflow, limits and recovery.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [host-setup.md](host-setup.md) | host requirements, OpenOCD interface and target config, reference configs |
| [workflow.md](workflow.md) | end to end: wire, drive mode, power, connect, run, exit |
| [limits-and-errors.md](limits-and-errors.md) | TAP-shifter limits, shift clamp, host quirks, messages, recovery |
| [../../protocol/jtag/openocd.md](../../protocol/jtag/openocd.md) | the in-mode `OCD1` byte protocol |

## The five-minute version

- Use OpenOCD's `buspirate` adapter driver.  Native Windows (MinGW) builds
  exclude it; a Cygwin/POSIX layer may work, otherwise use Linux, macOS or
  BSD.
- Bus Pirate MOSI → target TDI, MISO ← target TDO, CLK → target TCK,
  CS → target TMS, plus ground; SRST on AUX0, TRST on AUX1 (v4) or PGD (v3).
- The driver's default pin mode is **open-drain**.  Set
  `buspirate mode normal` for push-pull (no pull-ups), or keep open-drain and
  fit **2–10 kΩ** pull-ups to the target rail.
- Set `buspirate port`; close the terminal first.  OpenOCD performs the binary
  entry itself (`20 × 0x00` → `BBIO1`, `0x06` → `OCD1`).
- `buspirate vreg 1` only if the Bus Pirate should power the target; the path
  cannot select the rail voltage, so leave it `0` for any target on its own
  supply (including 1.8 V and 2.5 V).  `buspirate pullup 1` needs a voltage
  source on Vpullup.
- The firmware knows nothing about IR/DR layouts or the chain: the target
  config describes them, and one scan must stay at or below **8192 bits**;
  OpenOCD does not split a longer scan.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| firmware OpenOCD path (`0x06`) | present (build-only, unverified) | present |
| OpenOCD adapter (this set) | usable (POSIX host) | usable (POSIX host) |
| open-drain pin mode | yes | yes |
| `buspirate speed fast` | changes the physical link | accepted, no rate change |

Hardware: a v3 or v4 Bus Pirate, a USB cable, the target on its JTAG header,
and a host running an OpenOCD build that includes the Bus Pirate driver.  The
host drives the target; the Bus Pirate only shifts bits.

## Wiring (headline)

| Bus Pirate | JTAG |
|---|---|
| MOSI | TDI |
| MISO | TDO |
| CLK | TCK |
| CS | TMS |
| AUX0 | SRST (optional) |
| AUX1 (v4) / PGD (v3) | TRST (optional) |
| GND | GND |

**Push-pull needs no pull-up; open-drain needs 2–10 kΩ** on TDI/TMS/TCK to
the target rail.  TDO is an input driven by the target.  The full map, levels
and v3/v4 differences are in
[protocol/jtag/wiring.md](../../protocol/jtag/wiring.md).

## Limits & out of scope

- **TAP shifter only.**  No IR/DR descriptions, no chain discovery, no IDCODE
  handling; the host and its target config own all of it.
- **No JTAG clock control from the host.**  `adapter speed` in a target script
  is not forwarded to the board; TCK is set by the firmware's shift loop.
- **8192-bit scan limit.**  Keep each scan at or below 8192 bits; OpenOCD
  does not split a single longer scan, and the firmware clamp leaves stray
  payload that desyncs the next command.
- **Host driver excludes native Windows.**  OpenOCD's Bus Pirate driver is not
  built on native Windows (MinGW); use a Cygwin/POSIX layer, Linux, macOS or
  BSD.
- **No config is shipped.**  This repository ships no OpenOCD or target
  configuration; the files here are reference material.
- v3 behaviour is **build-only, not verified**.
- SWD is a different firmware path and is out of scope here.

## Related docs

| Doc | Why |
|---|---|
| [protocol/jtag/openocd.md](../../protocol/jtag/openocd.md) | the `0x06` protocol: entry, commands, TAP shift, speed, delay |
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | pins, levels, pull-ups, v3/v4 |
| [protocol/jtag/README.md](../../protocol/jtag/README.md) | the JTAG set overview and exit table |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and reply framing |
| [../xsvf/README.md](../xsvf/README.md) | the sibling file-driven JTAG tool |
| [generic/README.md](../../generic/README.md) | prompt, serial settings, binary entry |
| [../../SPEC.md](../../SPEC.md) | function and capability index |
