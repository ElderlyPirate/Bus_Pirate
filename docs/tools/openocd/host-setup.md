# OpenOCD adapter — host setup and configuration

Part of the OpenOCD adapter set (`docs/tools/openocd/`).  This page covers what
the host must provide, how OpenOCD is configured to drive the Bus Pirate, and a
reference configuration.  The byte protocol is in
[protocol/jtag/openocd.md](../../protocol/jtag/openocd.md); the end-to-end
startup and command sequence is in [workflow.md](workflow.md).

## Scope

- **Covered:** what the adapter is and is not, host and driver requirements,
  serial-port handling, the OpenOCD interface options, a reference target
  example, and reference configs.
- **Not covered:** the in-mode `OCD1` command table, which the JTAG set owns;
  the exact target script and part-specific settings beyond the reference
  example; the XSVF path.

## What the adapter is

The firmware's `0x06` path shifts one TMS bit and one TDI bit per clock and
returns the TDO bit.  It has no idea what those bits mean.  The host supplies
the meaning:

| Layer | Role |
|---|---|
| host debugger (OpenOCD) | TAP state machine, IR/DR layouts from the target config, flash algorithms, GDB server |
| `OCD1` byte protocol | pin modes, features, 16-bit TAP-shift blocks, ADC read, link speed, clock delay |
| Bus Pirate firmware (`0x06`) | drives TCK/TMS/TDI, samples TDO, reports TDO bytes |
| target TAP | the scan chain being driven |

Consequences:

- The firmware does not know IR lengths, DR layouts, IDCODEs, chain order or
  reset sequences.  All of that comes from the host's target configuration.
- The interface is not "JTAG over serial" in any standard sense.  It is a
  device-specific byte protocol (`OCD1`), so the host must be a driver that
  speaks it.  OpenOCD's Bus Pirate interface is the reference driver.
- A terminal cannot drive the chain.  There is no JTAG menu entry and no
  interactive JTAG command set.

The full entry, command and reply detail is in
[protocol/jtag/openocd.md](../../protocol/jtag/openocd.md).

## Host requirements

| Requirement | Detail |
|---|---|
| OpenOCD with the Bus Pirate driver | the reference host; the adapter driver is named `buspirate` |
| host OS | POSIX serial ports; **native Windows (MinGW) builds exclude the driver** — a Cygwin/POSIX layer may work, otherwise use Linux, macOS or BSD |
| serial device | v4 presents a USB CDC port (`/dev/ttyACM0` or similar); v3 presents an FTDI port (`/dev/ttyUSB0`) |
| port access | your user must be able to open the device read/write (serial group or a udev rule on Linux) |
| exclusive access | close the terminal, `screen`, `minicom`, PulseView or anything else holding the port |
| baud rate | leave it to the driver; do not configure the port by hand |
| board state | at the terminal prompt (`HiZ>`), with no configuration menu open |
| target power | the target must be powered and grounded before the adapter probes the chain |

Packaged OpenOCD builds vary: some distributions omit optional adapters.  If
the driver is missing, OpenOCD fails at startup with an unknown-adapter error
before it tries to open the port.  Check the build's configure summary (it
lists `Bus Pirate`) or the installed interface files.

### Port and baud handling

The driver owns the serial port for the whole session:

- It opens at **115200 baud, 8N1, no flow control** and discards stale bytes
  left over from a terminal session.
- `buspirate speed fast` switches the host port to **1 Mbaud** and asks the
  firmware to do the same.  On v3 that changes the physical UART rate; on v4
  the CDC rate is host metadata and the firmware change is a no-op.
- `buspirate speed normal` is the safe default and the value used at exit.

### Binary entry is automatic

You do not pre-enter a binary mode.  The driver sends the two entry bytes
itself: 20 × `0x00` (the board answers `BBIO1`), then `0x06` (the board
answers `OCD1`).

The full startup sequence — the optional speed change, the pin mode and
features, the reset release, and the chain probe/examine — is owned by
[workflow.md](workflow.md) §5.  The entry bytes belong to the binary layer —
see [the binary entry page](../../protocol/binary/entering-and-framing.md).
Leaving is the driver's job too; see
[limits-and-errors.md](limits-and-errors.md) §Leaving cleanly.

## Host-side configuration

This repository does **not** ship an OpenOCD configuration.  Everything in
this section is reference material for the host program.

| Item | Shipped here |
|---|---|
| firmware OpenOCD path (`0x06`) | yes — enabled on both builds |
| OpenOCD host program | no — separate project |
| OpenOCD interface config for the Bus Pirate | no — reference only |
| target config | no — OpenOCD's own target tree |

### Option names and the OpenOCD version matrix

The option spelling changed across OpenOCD releases.  Pick the row that
matches your installed OpenOCD; if a name is still rejected, open the
installed `interface/buspirate.cfg` and copy its spelling.

| OpenOCD | Driver selection | Option names |
|---|---|---|
| 0.12 and later | `adapter driver buspirate` | `buspirate port`, `buspirate speed`, `buspirate mode`, `buspirate vreg`, `buspirate pullup` |
| 0.11 | `adapter driver buspirate` | `buspirate_port`, `buspirate_speed`, `buspirate_mode`, `buspirate_vreg`, `buspirate_pullup` |
| 0.10 and earlier | `interface buspirate` | the same flat `buspirate_*` names |

The reference configs below use the 0.12-and-later form.  For 0.11 and
earlier, use the flat names:

```
# 0.11 and earlier — flat option names
adapter driver buspirate       # 0.11; on 0.10 use: interface buspirate
buspirate_port /dev/ttyACM0
buspirate_speed normal
buspirate_mode normal
buspirate_vreg 0
buspirate_pullup 0
reset_config srst_only
```

The option table below uses the 0.12-and-later form.

| Option | Values | Default | Effect |
|---|---|---|---|
| `buspirate port` | serial device path | none — required | the port the driver opens |
| `buspirate speed` | `normal` / `fast` | `normal` | 115200 vs 1 Mbaud link; v4 ignores the rate change |
| `buspirate mode` | `normal` / `open-drain` | `open-drain` | push-pull vs open-drain pin drive |
| `buspirate vreg` | `0` / `1` | `0` | on-board regulator feeding VCC |
| `buspirate pullup` | `0` / `1` | `0` | on-board pull-ups to Vpullup |

Two runtime commands are also available while OpenOCD is running:

| Command | Effect |
|---|---|
| `buspirate adc` | prints the ADC pin, Vpullup, 3.3 V and 5.0 V readings |
| `buspirate led 0` / `buspirate led 1` | drives the mode LED off / on |

On 0.11 and earlier the runtime commands use the flat names `buspirate_adc`
and `buspirate_led`.

**The driver's default pin mode is open-drain.**  The interface file shipped
with current OpenOCD keeps its `buspirate mode normal` line commented out, so
a configuration that only sets the port runs open-drain and needs pull-ups.
Set the mode explicitly for every session.

### Reference interface config

```
# interface.cfg — reference, 0.12 and later form; not shipped
adapter driver buspirate
buspirate port /dev/ttyACM0
buspirate speed normal
buspirate mode normal
buspirate vreg 0
buspirate pullup 0
reset_config srst_only
```

`reset_config srst_only` matches the interface file and tells OpenOCD that
only SRST is wired.  The firmware can drive both SRST and TRST as features;
adjust the reset configuration if both are connected.

### Target config — reference example only

```
source [find target/stm32f1x.cfg]
```

- OpenOCD's installed `scripts/target/` tree holds the supported parts;
  source the matching `.cfg`.  `stm32f1x.cfg` above is one example, not the
  only or a generic target.
- The target script supplies the chain description (IR length, IDCODE), the
  flash banks and the reset behaviour.  Use the script for the exact part on
  the board.
- A script for a similar part can still probe the chain, but usually fails at
  examine or at the first flash access.
- Target scripts often contain `adapter speed`.  The Bus Pirate driver does
  not forward a JTAG clock setting, so that value does not change TCK.  The
  shift rate is fixed by the firmware loop and the serial link.
- The target script's transport selection must be JTAG.  OpenOCD's Bus Pirate
  driver also has an SWD transport, which uses a different firmware path and
  is outside this set.

### Complete reference config

```
# openocd.cfg — reference, 0.12 and later form; not shipped
source [find interface/buspirate.cfg]
buspirate port /dev/ttyACM0
buspirate speed normal
buspirate mode normal
buspirate vreg 0
buspirate pullup 0
reset_config srst_only

source [find target/stm32f1x.cfg]
```

The same configuration can be given on the command line:

```
$ openocd -f interface/buspirate.cfg -f target/stm32f1x.cfg \
      -c "buspirate port /dev/ttyACM0"
```

Once the port is open and the target script is loaded, OpenOCD starts the
adapter and probes the chain.  The startup sequence, the startup log and the
reference command session are in [workflow.md](workflow.md) §5–§6; the clean
exit is in §7.

## If you write your own host

A custom driver must match the `OCD1` framing, not just send JTAG patterns.
The rules a host must honour:

| Rule | Why |
|---|---|
| read the 3-byte echo before the TDO bytes | the TDO reply follows the echoed command header |
| send TDI/TMS low byte first; add the high bytes for groups of 9–16 bits | the shift data is grouped per 16 bits |
| keep each scan at or below 8192 bits | a longer scan overflows the host's driver buffer; the firmware clamp leaves stray payload |
| set a pin mode before the first shift | entry leaves every pin an input |
| re-drive SRST/TRST after a mode change | the mode byte parks all five outputs low |
| leave with `0x00`, then `0x0F` | `0x0F` is not an `OCD1` command |

Link pins, features and the exact byte framing from
[protocol/jtag/openocd.md](../../protocol/jtag/openocd.md); do not guess them.

## Related docs

| Doc | Why |
|---|---|
| [protocol/jtag/openocd.md](../../protocol/jtag/openocd.md) | the in-mode protocol |
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | pins, levels, pull-ups |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and reply framing |
| [workflow.md](workflow.md) | wire, connect, run, exit |
| [limits-and-errors.md](limits-and-errors.md) | limits, messages, recovery |
| [../../SPEC.md](../../SPEC.md) | capability matrix |
