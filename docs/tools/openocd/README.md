# OpenOCD adapter

**Status:** current · the firmware's `0x06`/`OCD1` JTAG path and the host
side that drives it.  The path is compiled into both builds; its v3
behaviour is not verified.  The host driver is POSIX-only.

OpenOCD's **Bus Pirate** adapter driver turns the Bus Pirate into a JTAG
cable.  The firmware's `0x06` mode is a raw TAP shifter: the host debugger
walks the TAP, and the Bus Pirate supplies the pins and the byte transport.
The target configuration, IR/DR lengths, chain order and flash algorithms
all stay on the host.

**Naming.**  The **OpenOCD path** is the firmware mode (`0x06`, answers
`OCD1`).  The **OpenOCD adapter** is the host side: OpenOCD's `buspirate`
driver and this set.  The other JTAG path is the **XSVF player** (`0x18`,
answers `XSV1`, v4 only); the XSVF set calls the same thing the **firmware
player**.  Header names are the printed ones: `AUX` (SRST;
AUX-target token `A0`) and `AUX1` (TRST on v4; the v3 TRST is the `PGD`
programming-header pin).

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [overview.md](overview.md) | what the path is, the host/firmware split, the XSVF comparison |
| [entry-and-exit.md](entry-and-exit.md) | `BBIO1` entry, `0x06`, the entry state, `0x00`/`0x0F` exit |
| [commands.md](commands.md) | the `OCD1` command set: pin modes, features, TAP shift, ADC, speed, delay |
| [wiring.md](wiring.md) | pins, rails, levels, pull-ups, SRST/TRST, v3/v4 |
| [host-setup.md](host-setup.md) | host build check, POSIX options, reference configs, custom-host rules |
| [workflow.md](workflow.md) | wire, power, connect, run, health check, leave |
| [exchanges.md](exchanges.md) | worked byte exchanges |
| [limits-and-errors.md](limits-and-errors.md) | limits, driver messages, failure and recovery |

## The five-minute version

- Wire MOSI → TDI, MISO ← TDO, CLK → TCK, CS → TMS, plus GND.  SRST is
  `AUX`; TRST is `AUX1` (v4) or `PGD` (v3).  Both resets are optional, and
  OpenOCD drives them as active-low.
- Use OpenOCD's `buspirate` adapter driver on a **POSIX host** (Linux, macOS,
  BSD, or Cygwin).  Native Windows (MinGW/MSYS) builds do not include it.
  Check a build with `openocd -c "adapter list" -c "shutdown"` (0.10:
  `interface_list`); the output must include `buspirate`.
- Close the terminal and set `buspirate port`; the driver enters `BBIO1` and
  `OCD1` itself.  Do not pre-enter a binary mode.
- The driver's default pin mode is **open-drain**: fit **2–10 kΩ** pull-ups
  on TDI/TMS/TCK to the target rail, or, for a 3.3 V target only, set
  `buspirate mode normal`.  A 1.8 V or 2.5 V target must stay open-drain
  with pull-ups to its own rail.
- The target's VCC goes to the `3.3V` rail **or** the `5.0V` rail, never
  both.  The binary entry switches the board supplies off; use
  `buspirate vreg 1` only when the board powers the target.  The path cannot
  select a rail voltage.
- The firmware shifts bits only.  One `0x05` request carries at most **8192
  bits**, including every TAP move in that request.  OpenOCD batches scans
  into one request and does not split a longer scan; keep the whole request
  inside the limit.

First session: [workflow.md](workflow.md); host build:
[host-setup.md](host-setup.md); byte lookup: [commands.md](commands.md).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| firmware path (`0x06`) | present, not verified on v3 | present |
| pin modes push-pull / open-drain | present, not verified on v3 | present |
| `buspirate speed fast` | about 1 Mbaud on the UART link | accepted; CDC rate unchanged |
| OpenOCD host driver | POSIX only | POSIX only |
| chain scan, IDCODE, flash | host only | host only |

Hardware: a Bus Pirate, a USB cable, the target's JTAG header and a common
ground, and a POSIX host with an OpenOCD build that includes the Bus Pirate
driver.

## Wiring (headline)

| Bus Pirate | Direction | JTAG |
|---|---|---|
| MOSI | → | TDI |
| MISO | ← | TDO |
| CLK | → | TCK |
| CS | → | TMS |
| AUX | → | SRST (optional) |
| AUX1 (v4) / PGD (v3) | → | TRST (optional) |
| GND | — | GND |

Push-pull needs no pull-up.  Open-drain needs 2–10 kΩ on TDI/TMS/TCK (and
on SRST/TRST when used) to the target rail.  TDO is an input driven by the
target and needs no Bus Pirate pull-up.  The full map is in
[wiring.md](wiring.md).

## Limits & out of scope

- **TAP shifter only.**  No IR/DR descriptions, chain discovery or IDCODE
  handling; the host owns all of it.
- **No TCK control from the host.**  The `0x08` delay exists, but OpenOCD
  never sends it; `adapter speed` is not forwarded to the board.
- **8192-bit block.**  One `0x05` request carries at most 8192 bits,
  including its TAP moves.  The clamp is not a clean truncation: the surplus
  payload desynchronises the next command, and the TDO reply is short.
- **Host driver is POSIX-only.**  Native Windows builds exclude it.
- **No project configuration is shipped.**  OpenOCD's installed
  `interface/buspirate.cfg` is the starting template; the interface and
  target files here are reference material.
- **v3 is compiled in but not verified on v3.**
- **SWD is out of scope** (a different firmware path).
- `0x00` leaves `OCD1`, makes the BBIO pins inputs and switches the
  regulator and the on-board pull-ups off.  `0x0F` adds the return to the
  terminal (v3: a hardware reset); it is not what removes the power.

## Related docs

| Doc | Why |
|---|---|
| [protocol/jtag/tap.md](../../protocol/jtag/tap.md) | TAP states, shifts, IDCODE, bypass |
| [protocol/jtag/openocd.md](../../protocol/jtag/openocd.md) | the `0x06` path byte protocol in the JTAG set |
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | shared pin map, levels, supplies |
| [protocol/jtag/clock.md](../../protocol/jtag/clock.md) | TCK delay and link speed |
| [protocol/jtag/entry.md](../../protocol/jtag/entry.md) | binary entry for both JTAG paths |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` framing and reply shapes |
| [../xsvf/README.md](../xsvf/README.md) | the file-driven JTAG alternative |
| [../../generic/README.md](../../generic/README.md) | prompt, serial settings, terminal commands |
| [../../SPEC.md](../../SPEC.md) | function and capability index |
