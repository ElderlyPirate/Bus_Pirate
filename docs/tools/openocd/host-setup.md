# Host setup — OpenOCD, the Bus Pirate driver and the port

Part of the OpenOCD adapter set.  This page covers what the host must
provide, the POSIX-only build, the OpenOCD options, and reference
configurations.  The startup sequence and the run commands are in
[workflow.md](workflow.md); the byte protocol is in
[commands.md](commands.md).

## Scope

- **Covered:** host and driver requirements, the port, the OpenOCD option
  names across releases, reference interface and target configurations, and
  the rules for a custom host.
- **Not covered:** part-specific target settings beyond the reference
  example, the in-mode command set, and the XSVF path.

## Host requirements

| Requirement | Detail |
|---|---|
| OpenOCD with the Bus Pirate driver | the reference host; the adapter driver is named `buspirate` |
| host OS | **POSIX only** — Linux, macOS or BSD; on Windows see the routes below |
| serial device | v4: a USB CDC port (`/dev/ttyACM0` or similar); v3: an FTDI port (`/dev/ttyUSB0` or similar) |
| port access | the user must be able to open the device read/write (serial group or a udev rule) |
| exclusive access | no terminal, `screen`, `minicom` or other program may hold the port |
| board state | at the terminal prompt (`HiZ>`) with no menu open, or a board that can be reset to it |
| target power | the target must be powered and grounded before the chain is probed |

### The driver is not built on native Windows

OpenOCD's Bus Pirate driver is POSIX-only.  A MinGW or MSYS build disables
the adapter, and asking for it explicitly is rejected by the configure step.
Native Windows builds of OpenOCD therefore do not contain it.

| Host | Driver present |
|---|---|
| Linux, macOS, BSD | yes, if the build includes the adapter |
| Cygwin | yes, if built there (runtime unverified) |
| native Windows (MinGW/MSYS) | **no** — disabled by the build |

This is a host-build limit, not a firmware one: the firmware path is present
in both Bus Pirate builds.

On Windows, the workable routes are:

| Route | Requirement |
|---|---|
| Cygwin build | build OpenOCD inside Cygwin (runtime unverified) |
| WSL | USB passthrough of the Bus Pirate port to the Linux instance (unverified) |
| Linux VM | USB passthrough of the Bus Pirate port to the VM |

Native Windows cannot be made to work by adding a flag: the configure step
forces `--enable-buspirate` off on MinGW/MSYS and rejects it if asked
explicitly.

### Check the build

Not every packaged OpenOCD includes every adapter.  List the built-in
drivers before wiring anything:

```
$ openocd -c "adapter list" -c "shutdown"
buspirate { jtag swd }
```

The output is a Tcl dictionary: the adapter name, then the transports it
supports.  `buspirate` must appear.  On OpenOCD 0.10 the command is
`interface_list`.  If `buspirate` is not in the
list, build OpenOCD from source with the adapter enabled:

```
$ ./configure --enable-buspirate
$ make
$ sudo make install
```

The configure summary lists `Bus Pirate` with `yes` or `no`; confirm it says
`yes` before building.  A distro package cannot be extended this way; use a
release that includes the driver or install the local build.

### The serial port

The driver owns the port for the whole session:

- It opens at **115200 baud, 8N1, no flow control**, and discards bytes
  left over from a terminal session.
- v4 presents a USB CDC port; the baud rate is host metadata and the
  firmware does not reconfigure the link.
- v3 presents an FTDI port; the baud rate is real and the firmware can
  change it.  The driver opens at 115200, so a board whose terminal rate was
  changed with `b` is not reachable until it is reset (or set back to
  115200).
- `buspirate speed fast` switches the host port to **1 Mbaud** and asks the
  firmware to do the same.  On v3 that changes the physical UART rate; on v4
  the CDC rate does not change.
- `buspirate speed normal` is the safe default and the value used at exit.

## OpenOCD option names across releases

The option spelling changed across OpenOCD releases.  Pick the row that
matches the installed version; if a name is still rejected, open the
installed `interface/buspirate.cfg` and copy its spelling.

| OpenOCD | Driver selection | Option names |
|---|---|---|
| 0.12 and later | `adapter driver buspirate` | `buspirate port`, `buspirate speed`, `buspirate mode`, `buspirate vreg`, `buspirate pullup` |
| 0.11 | `adapter driver buspirate` | `buspirate_port`, `buspirate_speed`, `buspirate_mode`, `buspirate_vreg`, `buspirate_pullup` |
| 0.10 and earlier | `interface buspirate` | the same flat `buspirate_*` names |

The tables and configs below use the 0.12-and-later form.  For 0.11 and
earlier:

```
adapter driver buspirate
buspirate_port /dev/ttyACM0
buspirate_speed normal
buspirate_mode normal
buspirate_vreg 0
buspirate_pullup 0
reset_config srst_only
```

On 0.10 and earlier, replace the first line with `interface buspirate`.

## Options

| Option | Values | Default | Set at | Effect |
|---|---|---|---|---|
| `buspirate port` | serial device path | none — required | config | the port the driver opens |
| `buspirate speed` | `normal` / `fast` | `normal` | config | 115200 or 1 Mbaud link; v4 ignores the rate change |
| `buspirate mode` | `normal` / `open-drain` | `open-drain` | config | push-pull or open-drain pin drive |
| `buspirate vreg` | `0` / `1` | `0` | config | on-board regulator, feeding the `3.3V` and `5.0V` rails |
| `buspirate pullup` | `0` / `1` | `0` | config | on-board pull-ups to Vpullup |
| `buspirate adc` | — | — | runtime | print the four ADC readings |
| `buspirate led` | `0` / `1` | — | runtime | drive the mode LED |

"Config" options are accepted while OpenOCD reads its configuration; they
cannot be changed from a running session.  "Runtime" options work at the
telnet prompt after the adapter is initialised; before init, `buspirate adc`
returns without sending anything.

**The default pin mode is open-drain.**  The interface file shipped with
current OpenOCD keeps its `buspirate mode normal` line commented out, so a
configuration that only sets the port runs open-drain and needs pull-ups.
Set the mode explicitly for every session.

On 0.11 and earlier the runtime commands are `buspirate_adc` and
`buspirate_led`.

## Reference interface config

```
# interface.cfg — reference; OpenOCD ships interface/buspirate.cfg
adapter driver buspirate
buspirate port /dev/ttyACM0
buspirate speed normal
buspirate mode normal
buspirate vreg 0
buspirate pullup 0
reset_config srst_only
```

`reset_config srst_only` matches the interface file and tells OpenOCD that
only SRST is wired.  Use `reset_config trst_and_srst` if TRST is connected
too.

## Reference target config

```
# openocd.cfg — reference; source OpenOCD's installed interface file
source [find interface/buspirate.cfg]
buspirate port /dev/ttyACM0
buspirate speed normal
buspirate mode normal
buspirate vreg 0
buspirate pullup 0
reset_config srst_only

source [find target/stm32f1x.cfg]
```

- OpenOCD's installed `scripts/target/` tree holds the supported parts.
  Source the `.cfg` for the exact part; `stm32f1x.cfg` is one example, not a
  generic target.
- The target script supplies the chain description (IR length, IDCODE), the
  flash banks and the reset behaviour.
- A script for a similar part can still probe the chain but usually fails at
  examine or at the first flash access.
- Target scripts often contain `adapter speed`.  The Bus Pirate driver does
  not forward a JTAG clock setting, so that value does not change TCK.
- The transport must be JTAG.  OpenOCD's Bus Pirate driver also has an SWD
  transport, which uses a different firmware path and is outside this set.

The same configuration can be given on the command line:

```
$ openocd -f interface/buspirate.cfg -f target/stm32f1x.cfg \
      -c "buspirate port /dev/ttyACM0"
```

## If you write your own host

A custom driver must match the `OCD1` framing, not just send JTAG patterns.

| Rule | Why |
|---|---|
| read the 3-byte echo before the TDO bytes | the TDO reply follows the echoed header |
| send TDI/TMS in groups of up to 16 bits, low byte first | that is the only payload layout the path accepts |
| keep the whole `0x05` request, including TAP moves, at or below 8192 bits | a longer request is clamped and the surplus desyncs the next command |
| set a pin mode before the first shift | entry leaves every pin an input |
| release SRST/TRST with feature action `1` | the pin mode drives both lines low |
| do not wait for a reply to `0x01`, `0x02` or `0x08` | those requests have no reply |
| on v3, send the `0x07` selector and the `0xAA 0x55` confirmation in separate writes | the firmware changes its UART before the confirmation |
| on v3, follow a `0x07` rate change and its revert | a failed confirmation returns the firmware to normal |
| leave with `0x00`, then `0x0F` | `0x0F` is not an `OCD1` command |
| do not use `0x04` for the shift | the current TAP shift is `0x05` |

The exact byte values, the ADC reply and the speed handshake are in
[commands.md](commands.md).

## Related

| Doc | Why |
|---|---|
| [workflow.md](workflow.md) | startup sequence, health check, run and exit |
| [commands.md](commands.md) | the command set a custom host must speak |
| [wiring.md](wiring.md) | pins, rails, pull-ups |
| [limits-and-errors.md](limits-and-errors.md) | driver messages and recovery |
| [../../protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and framing |
| [../../generic/README.md](../../generic/README.md) | terminal port settings before a session |
