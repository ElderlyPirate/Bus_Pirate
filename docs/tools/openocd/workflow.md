# OpenOCD workflow — from header to halted core

Part of the OpenOCD adapter set (`docs/tools/openocd/`).  This page is the
end-to-end sequence: wire the target, choose the drive mode, set power and
pull-ups, connect OpenOCD, run the debug or programming commands, and leave the
board clean.  Options and configs are in [host-setup.md](host-setup.md); the
failure cases are in [limits-and-errors.md](limits-and-errors.md).

## Scope

A single-target JTAG session driven by OpenOCD's Bus Pirate interface.  The
firmware protocol is not repeated here; the commands and reply rules are in
[protocol/jtag/openocd.md](../../protocol/jtag/openocd.md).

## 1. Wire the target

| Bus Pirate | JTAG | Direction |
|---|---|---|
| MOSI | TDI | Bus Pirate → target |
| MISO | TDO | target → Bus Pirate |
| CLK | TCK | Bus Pirate → target |
| CS | TMS | Bus Pirate → target |
| AUX0 | SRST | Bus Pirate → target (optional) |
| AUX1 (v4) / PGD (v3) | TRST | Bus Pirate → target (optional) |
| GND | GND | — |

- Check pin 1 and the ground before applying power.
- Connect SRST if you want `reset` commands to reset the CPU.  Without it,
  reset commands drive a line that goes nowhere.
- Connect TRST only if the target brings it out; most targets reset the TAP
  from TMS alone.
- Levels are 3.3 V logic; the interface is 5 V safe, 5.5 V maximum.  The full
  map and v3/v4 differences are in
  [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md).

## 2. Choose the drive mode

The drive mode is fixed when OpenOCD initialises and is one of the two pin
modes the firmware offers.

| Situation | `buspirate mode` | `buspirate vreg` | Pull-ups |
|---|---|---|---|
| 3.3 V target, Bus Pirate drives the chain | `normal` (push-pull) | `0`, or `1` if the board powers the target | none |
| lower-voltage target (1.8 V, 2.5 V) | `open-drain` | `0` — leave the board supply off | 2–10 kΩ per line to the target rail |
| long or noisy wiring | `normal` first | `0` | none |
| option omitted | `open-drain` (driver default) | `0` (default) | required |

Open-drain pulls each line low and lets the pull-up set the high level, so the
high level can be the target's own rail.  Push-pull drives the high level
itself; do not drive a 3.3 V push-pull output into a lower-voltage chain.
TDO is always an input and needs no Bus Pirate pull-up.

On the lower-voltage row, leave `buspirate vreg 0`: the path cannot select a
rail voltage, so `vreg 1` would feed the board's fixed VCC into the target.

## 3. Set power and pull-ups

| Target supply | `buspirate vreg` | Note |
|---|---|---|
| target has its own supply | `0` (default) | do not back-drive the target |
| Bus Pirate should power the target | `1` | the regulator feeds VCC; disconnect the target's own supply |

The OpenOCD path cannot select the VCC voltage; `vreg 1` enables the board's
fixed supply.  Use `buspirate vreg 0` for any target that is not powered from
the board's VCC rail — in particular every 1.8 V or 2.5 V target.

`buspirate pullup 1` enables the Bus Pirate's on-board pull-up resistors.
They tie the lines to Vpullup, not to a per-line rail, so a voltage must be
present on Vpullup.  On the JTAG path the on-board source is not selectable:
feed Vpullup externally, or use per-line pull-ups to the target rail (the
usual choice for open-drain level shifting).

Prerequisites the host cannot skip:

- **SRST and TRST only drive after a pin mode is set.**  On entry they are
  inputs.  OpenOCD sets the mode at init and then deasserts both.
- **Feature `0x10` (pull-ups) needs a voltage on Vpullup.**  Enabling it with
  nothing there has no effect.
- **A mode change parks the reset lines low.**  The firmware resets all five
  outputs before applying the direction, so SRST/TRST are asserted until the
  host drives them high again.  OpenOCD does this automatically.

The feature and pin-mode bytes are in
[protocol/jtag/openocd.md](../../protocol/jtag/openocd.md).

## 4. Prepare the host

1. Connect the Bus Pirate by USB and find its serial device.  v4 is a USB CDC
   port; v3 is an FTDI port.
2. Close the terminal program and any other program using the port.
3. Leave the board at the terminal prompt (`HiZ>`).  Press Enter a few times
   and send `#` if it may be sitting in a menu.
4. Check that your OpenOCD build includes the Bus Pirate driver.
5. Write the interface and target configuration
   ([host-setup.md](host-setup.md)) and set `buspirate port` to the device
   found in step 1.

Do not enter a binary mode by hand.  OpenOCD sends the NUL entry and the
`0x06` request itself.

## 5. Connect

Start OpenOCD with the configuration loaded.  This is the full startup
sequence, in order:

| Step | Host action | Result |
|---|---|---|
| 1 | sends 20 × `0x00` | `BBIO1` |
| 2 | sends `0x06` | `OCD1` |
| 3 | optional speed change | `fast` or `normal` confirmed |
| 4 | sets the pin mode | outputs enabled, resets parked low |
| 5 | sets VREG and pull-ups | regulator / pull-ups as configured |
| 6 | deasserts SRST and TRST | chain released |
| 7 | probes the chain | IDCODE line in the log |
| 8 | examines the target | flash banks and core known |

Steps 7 and 8 come from the target config, not from the Bus Pirate.  If they
fail, the wiring or the target config is the place to look — the adapter
itself is already past entry.

### Health check

While OpenOCD is running, `buspirate adc` prints four readings: the ADC pin,
Vpullup, 3.3 V and 5.0 V.  Use it to confirm that the target rail and Vpullup
are present before blaming the chain.  `buspirate led 1` turns the mode LED
on as a simple liveness check.

## 6. Reference session — commands and output

A successful start reports the driver's own line:

```
$ openocd -f openocd.cfg
...
Info : Buspirate JTAG Interface ready!
Info : JTAG tap: ... tap/device found: 0x...
Info : starting gdb server for ... on 3333
```

The `Buspirate JTAG Interface ready!` line is the driver; the target lines
come from the target config and vary with the part.  `...` marks elided lines.

Attach to the running server (telnet on port 4444 by default) or pass the
commands with `-c`:

```
$ telnet localhost 4444
> reset halt
> reg
> flash write_image erase firmware.elf
> reset run
> shutdown
```

| Command | Use |
|---|---|
| `reset halt` | start from a known state: reset the CPU, then halt at the reset vector |
| `halt` | stop a running core without resetting |
| `examine` | probe the chain and the target; usually done automatically at startup |
| `reg` | inspect the core registers after a halt |
| `mdw <addr> [count]` | read memory words |
| `flash write_image erase <file>` | erase the target flash and program an image |
| `program <file> verify reset` | write, verify and reset in one step |
| `reset run` | reset and release the CPU |
| `shutdown` | leave OpenOCD cleanly |

- `reset` and `reset halt` drive SRST.  If SRST is not wired, the CPU does not
  reset; use the target's own reset or a TAP-level command sequence instead.
- Before programming, the target must be examined and its flash driver loaded
  by the target config.  `flash banks` lists what is available.
- `program` and `flash` are target helpers; their exact spelling and the image
  format depend on the part.
- A GDB session connects to port 3333 and uses the same server; OpenOCD stays
  in control of the adapter.

## 7. Leave cleanly

Send `shutdown` (or Ctrl-C once) and let OpenOCD close the adapter.  It:

1. sets the pin mode back to Hi-Z;
2. sets the link speed back to normal;
3. sends `0x00` to leave `OCD1`; the board answers `BBIO1`;
4. sends `0x0F`; the board replies `0x01` and runs the terminal reset;
5. closes the port.

The board returns to `HiZ>`: every header pin is an input, the pull-ups are
released and the supplies are off.  Once it is back at the prompt, the
terminal can reopen the port.

If OpenOCD is killed instead of shut down, the board is left inside `OCD1`.
Recovery depends on where it stopped:

| Killed | State | Recovery |
|---|---|---|
| between commands | the mode loop is waiting for a command byte | send `0x00` (back to `BBIO1`), then `0x0F` |
| mid-shift | the mode loop is still reading TDI/TMS payload | press reset; or send the remaining TDI/TMS bytes to finish the shift, then `0x00` |

Mid-shift, `0x00` and `0x0F` are consumed as shift data, so they do not leave
the mode.  Pressing reset is the reliable action.  The full recovery table is
in [limits-and-errors.md](limits-and-errors.md) §Leaving cleanly.

## 8. If it fails

| Symptom | First checks |
|---|---|
| no `BBIO1` / entry error | board not at the prompt, port held by a terminal, wrong device |
| `OCD1` reached, chain reads all zeroes | target unpowered, TDO not connected, no ground, SRST held low |
| chain reads all ones | TDI/TDO swapped, TDO floating, target not driving the bus |
| IDCODE found but target not examined | wrong target config (IR length/IDCODE) or chain order |
| no clock or data in open-drain | pull-ups missing, or switch to `buspirate mode normal` |
| programming fails after a good probe | flash driver or image for a different part |
| killed host leaves no prompt | between commands: send `0x00`, then `0x0F`; mid-shift: press reset |

The message tables and recovery steps are in
[limits-and-errors.md](limits-and-errors.md).

## Related docs

| Doc | Why |
|---|---|
| [host-setup.md](host-setup.md) | requirements, options, reference configs |
| [limits-and-errors.md](limits-and-errors.md) | limits, messages, recovery |
| [protocol/jtag/openocd.md](../../protocol/jtag/openocd.md) | the in-mode protocol |
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | pins, levels, pull-ups |
| [protocol/jtag/README.md](../../protocol/jtag/README.md) | the JTAG set overview |
| [../xsvf/workflow.md](../xsvf/workflow.md) | the file-driven alternative |
