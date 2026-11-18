# Workflow — from header to halted core

Part of the OpenOCD adapter set.  This page is the end-to-end sequence:
wire the target, choose the drive mode and power, prepare the host, connect,
run the debug or programming commands, and leave the board clean.  Options
and configurations are in [host-setup.md](host-setup.md); the failure cases
are in [limits-and-errors.md](limits-and-errors.md).

## Scope

A single-target JTAG session driven by OpenOCD's Bus Pirate driver.  The
firmware protocol is in [commands.md](commands.md); the TAP is in
[protocol/jtag/tap.md](../../protocol/jtag/tap.md).

## 1. Wire the target

| Bus Pirate | Direction | JTAG |
|---|---|---|
| MOSI | → | TDI |
| MISO | ← | TDO |
| CLK | → | TCK |
| CS | → | TMS |
| AUX | → | SRST (optional) |
| AUX1 (v4) / PGD (v3) | → | TRST (optional) |
| GND | — | GND |

- Check pin 1 and the ground before applying power.
- Connect SRST if `reset` commands must reset the CPU.  Without it, a reset
  request drives a line that goes nowhere.
- Connect TRST only if the target brings it out; most targets reset the TAP
  from TMS alone.
- Do not connect the target's VCC to both rails.  See
  [wiring.md](wiring.md).

## 2. Choose the drive mode and power

The drive mode is fixed when OpenOCD initialises.

| Situation | `buspirate mode` | `buspirate vreg` | Pull-ups |
|---|---|---|---|
| 3.3 V target, board powers it | `normal` | `1` | none |
| 3.3 V target, own supply | `normal` | `0` | none |
| 1.8 V or 2.5 V target, own supply | `open-drain` | `0` | 2–10 kΩ to the target rail |
| 5 V target, marginal 3.3 V high | `open-drain` | `0` | 2–10 kΩ to the target's 5 V rail |
| long or noisy wiring | `normal` first | `0` | none |
| option omitted | `open-drain` (default) | `0` | required |

Open-drain pulls each line low and lets the pull-up set the high level, so
the high level can be the target's own rail.  Push-pull drives the high
level itself; do not drive a 3.3 V push-pull output into a lower-voltage
chain.

`buspirate vreg 1` enables the board's fixed supply.  The path cannot select
a rail voltage, so leave it `0` for any target that is not powered from the
board's `3.3V` or `5.0V` rail, in particular every 1.8 V or 2.5 V target.
When the board does power the target, wire the target's VCC to exactly one
rail and disconnect any other supply.

`buspirate pullup 1` enables the on-board pull-up resistors.  They tie the
lines to Vpullup, so a voltage must be present on that pin; feed it
externally.  Per-line pull-ups to the target rail are the usual choice for
open-drain level shifting.

## 3. Prepare the host

1. Connect the Bus Pirate by USB and find its serial device.  v4 is a USB
   CDC port; v3 is an FTDI port.
2. Close the terminal program and any other program using the port.
3. Leave the board at the terminal prompt (`HiZ>`).  Press Enter a few
   times and send `#` if it may be sitting in a menu.  On v3, a terminal
   baud changed with `b` must be reset: OpenOCD opens the port at 115200.
4. Check that the OpenOCD build includes the Bus Pirate driver:
   `openocd -c "adapter list" -c "shutdown"` (0.10: `interface_list`) must
   list `buspirate`.  The
   driver is POSIX-only: native Windows (MinGW/MSYS) builds do not contain
   it.  Use Linux, macOS or BSD; on Windows, use Cygwin (runtime
   unverified), or WSL or a Linux VM with USB passthrough.
5. Write the interface and target configuration
   ([host-setup.md](host-setup.md)) and set `buspirate port` to the device
   found in step 1.

Do not enter a binary mode by hand.  OpenOCD sends the NUL entry and the
`0x06` request itself.

## 4. Connect

The driver's startup order, and what the board does:

| Step | Host action | Board response |
|---|---|---|
| 1 | opens the port at 115200 8N1 and discards stale bytes | — |
| 2 | sends 20 × `0x00` | `BBIO1` |
| 3 | sends `0x06` | `OCD1` |
| 4 | optional: `0x07` fast and `0xAA 0x55` | speed echo `0x07 0x01` |
| 5 | logs `Buspirate JTAG Interface ready!` | — |
| 6 | sends the pin mode (`0x01` + mode) | all five JTAG outputs driven low |
| 7 | sends the VREG feature | regulator on or off as configured |
| 8 | sends the pull-up feature | on-board pull-ups on or off |
| 9 | sends TRST and SRST features with action `1` | both resets released |
| 10 | the target script probes the chain and examines the target | IDCODE and flash banks in the log |

Steps 1–9 are the driver; step 10 is the target script's work.  If it fails,
the wiring or the target configuration is the place to look; the adapter is
already past entry.

## 5. Health check

While OpenOCD is running, `buspirate adc` reads the four ADC channels.  The
server log gains a line of this form:

```
Info : ADC: ADC_Pin = 0.00 VPullup = 5.00 V33 = 3.30 V50 = 0.00
```

The values are in volts.  Use them to confirm that the board rails and
Vpullup are present before blaming the chain:

| Reading | What it shows |
|---|---|
| `ADC_Pin` | the ADC/probe pin, usually 0 when nothing is connected |
| `VPullup` | the external voltage on Vpullup, needed by the on-board pull-ups |
| `V33` | the board's `3.3V` rail; 0 with VREG off and no board rail connection |
| `V50` | the board's `5.0V` rail |

`V33` and `V50` measure the board's rails, not the target's own supply.  With
VREG off and no rail connection they read 0; do not connect a self-powered
target's supply to a rail just to make it read.  That is back-driving.

`buspirate led 0` and `buspirate led 1` switch the mode LED off and on; use
them as a simple liveness check.  Both commands are runtime commands; before
init, `buspirate adc` sends nothing.

## 6. Run the session

A start reports the driver's own line; the target lines come from the
target configuration and vary with the part.  `...` marks elided lines:

```
$ openocd -f openocd.cfg
Open On-Chip Debugger 0.12.0
...
Info : Buspirate JTAG Interface ready!
Info : JTAG tap: stm32f1x.cpu tap/device found: 0x3ba00477
       (mfg: 0x23b (ARM Ltd), part: 0xba00, ver: 0x3)
Info : stm32f1x.cpu: hardware has 6 breakpoints, 4 watchpoints
Info : starting gdb server for stm32f1x.cpu on 3333
Info : Listening on port 3333 for gdb connections
```

Attach to the running server (telnet on port 4444 by default) or pass
commands with `-c`.  Addresses and bank sizes depend on the part:

```
$ telnet localhost 4444
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
Open On-Chip Debugger
> halt
target halted due to debug-request, current mode: Thread
xPSR: 0x01000000 pc: 0x08000144 msp: 0x20005000
> flash banks
#0 : stm32f1x.flash (flash) at 0x08000000, size 0x00020000, buswidth 0, chipwidth 0
> flash write_image erase firmware.elf
...
> reset run
> shutdown
shutdown command invoked
```

| Command | Use |
|---|---|
| `reset halt` | reset the CPU, then halt at the reset vector |
| `halt` | stop a running core without resetting |
| `examine` | probe the chain and the target; usually done at startup |
| `reg` | inspect core registers after a halt |
| `mdw <addr> [count]` | read memory words |
| `flash banks` | list the flash drivers the target script declared |
| `flash write_image erase <file>` | erase the target flash and program an image |
| `program <file> verify reset` | write, verify and reset in one step |
| `reset run` | reset and release the CPU |
| `shutdown` | leave OpenOCD cleanly |

- `reset` and `reset halt` drive SRST.  If SRST is not wired, the CPU does
  not reset; use the target's own reset or a TAP-level sequence.
- Before programming, the target must be examined and its flash driver
  loaded by the target config.
- `program` and `flash` are target helpers; their exact spelling and the
  image format depend on the part.
- A GDB session connects to port 3333 and uses the same server; OpenOCD
  stays in control of the adapter.
- `adapter speed` in a target script does not change TCK on this adapter.

## 7. Leave cleanly

Send `shutdown` (or Ctrl-C in the OpenOCD console) and let OpenOCD close
the adapter.  It:

1. sets the pin mode back to Hi-Z;
2. sets the link speed back to normal;
3. sends `0x00`; the board answers `BBIO1`;
4. sends `0x0F`; the board replies `0x01` and runs the terminal reset;
5. closes the port.

The board returns to `HiZ>`.  `0x00` has already made the BBIO pins inputs
and switched the supplies and the on-board pull-ups off; `0x0F` adds the
terminal reset (v3: a hardware reset).  On v4, every header pin is then an
input.  On v3, the link can emit stray bytes while the board comes back;
discard until the banner and the prompt.  Once the prompt is back, the
terminal can reopen the port.

If OpenOCD is killed instead of shut down, the board is left inside `OCD1`:

| Killed | State | Recovery |
|---|---|---|
| between commands | the loop is waiting for a command byte | send `0x00` (back to `BBIO1`), then `0x0F` |
| mid-shift | the loop is still reading TDI/TMS payload | press reset; or send the remaining payload, then `0x00` |

Mid-shift, `0x00` and `0x0F` are consumed as payload, so they do not leave
the mode.  Pressing reset is the reliable action.  The full recovery table
is in [limits-and-errors.md](limits-and-errors.md#leaving-and-recovery).

## 8. If it fails

| Symptom | First checks |
|---|---|
| no `BBIO1` / entry error | board not at the prompt, port held by a terminal, wrong device |
| `OCD1` reached, chain reads all zeroes | target unpowered, TDO not connected, no ground, SRST held low |
| chain reads all ones | TDI/TDO swapped, TDO floating, target not driving the bus |
| IDCODE found but target not examined | wrong target config (IR length, IDCODE) or chain order |
| no clock or data in open-drain | pull-ups missing, or switch to `buspirate mode normal` |
| the target stays in reset | release SRST/TRST with feature action `1` |
| programming fails after a good probe | flash driver or image for a different part |
| session aborts with a buffer overflow | a scan plus its TAP moves is longer than 8192 bits; split it |
| killed host leaves no prompt | between commands: `0x00`, then `0x0F`; mid-shift: press reset |

The message tables and recovery steps are in
[limits-and-errors.md](limits-and-errors.md).

## Related

| Doc | Why |
|---|---|
| [host-setup.md](host-setup.md) | requirements, options, reference configs |
| [commands.md](commands.md) | the in-mode command set |
| [wiring.md](wiring.md) | pins, rails, pull-ups, resets |
| [limits-and-errors.md](limits-and-errors.md) | limits, messages, recovery |
| [../xsvf/workflow.md](../xsvf/workflow.md) | the file-driven alternative |
| [../../protocol/jtag/tap.md](../../protocol/jtag/tap.md) | TAP states and scans |
