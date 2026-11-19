# Limits, errors and recovery

Part of the OpenOCD adapter set.  This page owns what the firmware path and
the host driver cannot do, what their messages mean, and how to recover.
The byte-level rules are in [commands.md](commands.md); the host options are
in [host-setup.md](host-setup.md).

## Scope

Firmware limits (TAP shifter only, the 8192-bit block, zero counts, speed and
delay), host-driver limits, the exit rules, the message tables and the
recovery steps.  It does not repeat the command table.

## Firmware limits

| Limit | Value | Host-side consequence |
|---|---|---|
| shift per `0x05` | 8192 bits per request, clamped | keep the whole request, including its TAP moves, at or below 8192 bits |
| TAP knowledge | none | the host and its target config own IR/DR/chain |
| TAP state kept by the firmware | none; only the pin levels persist | the host tracks the TAP position across commands |
| pin state at entry | TDI, TMS, TCK and SRST inputs; TDO input; TRST not touched | set a pin mode before the first shift |
| JTAG clock | only the `0x08` loop delay | `adapter speed` does not change TCK |
| TCK delay | default `1` | OpenOCD never sends `0x08`, so the default applies |
| link speed | normal / fast | v4 accepts fast but does not change the rate |
| zero count | no-op on both builds; 3-byte echo only | no payload read, no TDO byte; OpenOCD never sends one |

The v3 path is compiled into the build; other v3 behaviour is not verified on
v3.  The byte protocol is the same on both boards.

### The TAP-shifter boundary

The firmware shifts bits; it does not model the TAP.  It has:

- no instruction-register or data-register descriptions;
- no chain discovery, IDCODE read or chain-order knowledge;
- no automatic TMS reset pattern or state-transition shortcuts;
- no knowledge of the target's reset behaviour or flash layout.

All of that is the host's.  The target configuration supplies the IR
lengths, the IDCODEs, the flash banks and the reset sequence.  If the target
script names a different part or chain order, the probe can still read an
IDCODE but examine and flash fail.  A successful entry is not a successful
target probe.

### The 8192-bit block

`0x05` carries a 16-bit count of bits, capped at 8192.  The reply echoes the
**requested** count even when the firmware clamped it, so a host cannot
detect the clamp from the echo.  The TDO reply is sized for the clamped
count, not the requested count.

The clamp is not a clean truncation: only the clamped payload is consumed,
and the leftover TDI/TMS bytes stay in the serial stream and are read as
requests.  The mode may leave, shift, or answer an unknown-command `0x00`;
the session no longer matches the host's model.

On the host side, OpenOCD packs queued scans and their TAP moves into one
`0x05` request.  The driver has two 1024-byte lane buffers, one for TDI and
one for TMS, so the request holds at most 8192 bits.  It flushes when the
next scan does not fit, but does **not** split one scan that is longer than
the buffer: a single scan above 8192 bits overflows, logs
`tap_chain overflow, bad things will happen` and exits.  A scan of 8192 bits
also overflows if TAP moves precede it in the same request.

Keep the whole `0x05` request, including its TAP moves, at or below 8192
bits with margin.  A custom host must split long scans before sending them.

### Zero-count shifts

A zero count is a guarded no-op on both builds.  The 3-byte echo is sent; no
payload is read and no TDO byte follows.

No host needs a zero-bit shift.

### Speed and delay

| Setting | Behaviour |
|---|---|
| `buspirate speed normal` | 115200 link; always safe |
| `buspirate speed fast` (v3) | about 1 Mbaud on the UART |
| `buspirate speed fast` (v4) | accepted and acknowledged; the CDC rate does not change |
| `0x07` handshake | not a status: the reply repeats the selector, and a bad `0xAA 0x55` forces normal |
| `0x08` clock delay | two bytes, default `1`; a larger value slows TCK |

A custom host must send the selector, switch its own port to the requested
rate, then send the confirmation in a separate write; on v3 the firmware
changes its UART first.  A failed confirmation reverts the firmware to
normal, and the reply arrives at normal.

OpenOCD's driver has no JTAG clock control and never sends `0x08`.  A stock
session therefore runs at the firmware's default delay, and `adapter speed`
in a target script is cosmetic for this adapter.

## Host-driver limits

| Limit | Detail |
|---|---|
| POSIX serial only | native Windows (MinGW/MSYS) builds do not include the driver; use Cygwin (runtime unverified), WSL or a Linux VM with USB passthrough |
| one owner of the port | a terminal or another program holding the port blocks the driver |
| one OpenOCD session | do not start a second instance on the same port; the two byte streams interleave and corrupt each other |
| open-drain by default | an unconfigured session needs pull-ups |
| reset commands need SRST | without SRST wiring, a reset request drives a line that goes nowhere |
| no project config shipped | OpenOCD's installed interface file is the starting point; target and clock settings are the host's responsibility |
| no clock control | `adapter speed` is not forwarded |
| runtime vs config | `buspirate mode`, `speed`, `vreg` and `pullup` are config-stage only |

## Leaving and recovery

Entry (`0x06`, and the 20-NUL binary entry before it) switches VREG and the
on-board pull-ups off; the exit chain below does the same on `0x00`.

The exit chain and its effect:

| Action | Effect | Next |
|---|---|---|
| `0x00` inside `OCD1` | leaves the mode; the five BBIO pins (`AUX`, `CS`, `MISO`, `CLK`, `MOSI`) become inputs; TRST keeps its last level; **VREG and the on-board pull-ups switch off**; `BBIO1` | `0x0F` from `BBIO1` |
| `0x0F` inside `BBIO1` | `0x01`, then the terminal reset; v4 prints the banner and `HiZ>`, v3 restarts | re-enter with 20 × `0x00` |
| OpenOCD `shutdown` | Hi-Z mode, normal speed, then `0x00` and `0x0F` | board at `HiZ>`, supplies off |

`0x00` already switches the regulator and the on-board pull-ups off;
`0x0F` adds the terminal return (v4: banner and `HiZ>`; v3: hardware reset).
A hand-driven exit may stop after `0x00` if the board is to stay in
`BBIO1`; the supplies are already off.  TRST keeps its last level until a
pin mode or the terminal reset takes it over.

### Edge cases and recovery

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| OpenOCD killed between commands | no prompt; `0x0F` does nothing | the loop waits for a command byte | send `0x00`, then `0x0F` |
| OpenOCD killed mid-shift | no prompt; `0x00` and `0x0F` are consumed | the loop still reads payload | press reset, or finish the payload |
| `0x0F` inside `OCD1` | a single `0x00`, no exit | unknown command | send `0x00` first |
| `0x04` where the shift belongs | a single `0x00`, no shift | unknown command | use `0x05` |
| a new pin mode with SRST/TRST wired | target held in reset; chain reads zero | both resets driven low | release with feature action `1` |
| an even action such as `0x02` on a reset feature | the line stays low | only the low bit of the action is used | use `0x00` or `0x01` |
| full payload after a clamped count | the next reply is misaligned; the mode may leave | surplus bytes read as requests | reset; resend with the whole request at or below 8192 bits |
| a `0x07` confirmation at the wrong rate on v3 | the speed reply is unreadable | firmware reverts to normal | set the port back to 115200 |
| open-drain with no pull-ups | chain reads all zeroes | lines never go high | fit 2–10 kΩ, or use push-pull |
| on-board pull-ups with no Vpullup | the pull-ups have no effect | no high level on the lines | feed Vpullup externally |
| entry fails after a previous session | no `BBIO1` | port held, or the board is still in a binary sub-mode | close the other program or reset |
| a v3 terminal baud changed with `b` | no `BBIO1` after entry | the driver opens the port at 115200 | reset the board, or set `b` back to 115200 |
| a v3 session killed at fast | the next 115200 open gets no `BBIO1` | the UART stays at 1 Mbaud and the board stays in `OCD1` | reset the board |
| chain probes but the part is wrong | examine or flash fails | wrong target config | use the script for the exact part |
| VREG enabled with the target's own supply | contention risk | the board back-drives the target | set `buspirate vreg 0` |
| programming fails after a good probe | flash error | wrong flash driver or image | check `flash banks` |

The XSVF player is the exception to the shared exit pattern: it ignores
`0x00` and leaves on `0x0F`.  The comparison is in
[protocol/jtag/README.md](../../protocol/jtag/README.md).

## Driver messages and startup symptoms

Wording is from current OpenOCD; other releases differ.

| Message or symptom | Cause | Recovery |
|---|---|---|
| `You need to specify the serial port!` | no `buspirate port` | add the option |
| `Could not open serial port` | missing device, permissions, or port in use | check the device; close the terminal |
| `Error configuring the serial port.` | the port rejected the settings | check the device and permissions |
| `Buspirate error. Is binary/OpenOCD support enabled?` | no `BBIO1` after entry | check the board state, the port and the firmware |
| `Buspirate did not answer correctly! Do you have correct firmware?` | unexpected reply | reset; close other programs |
| `Unsupported binary protocol` | a `BBIOx` other than `BBIO1` | use matching firmware |
| `Buspirate did not ack speed change` | the `0x07` handshake failed | use `speed normal`; check the link |
| `Buspirate did not reply as expected to the speed change command` | speed reply mismatch | use `speed normal` |
| `Buspirate switched to FAST mode` | informational after a fast switch | — |
| `Unable to restart buspirate!` | `0x00` did not return `BBIO1` | press reset; the next session re-enters |
| `Some stale data from a previous connection was discarded.` | leftover terminal bytes | informational |
| `tap_chain overflow, bad things will happen` | a scan plus its TAP moves exceeds the 8192-bit block | split the scan; the session exits |
| `error writing :(` / `error reading` | the link failed mid-transfer | reset and retry |
| `Buspirate JTAG Interface ready!` | the driver reached the TAP stage | — |
| `Shutting down buspirate.` | a clean `shutdown` started | — |
| unknown adapter driver at startup | OpenOCD built without the Bus Pirate driver | use a POSIX build that includes it |

## Target and chain messages and symptoms

These come from OpenOCD and the target config, not from the Bus Pirate.

| Message or symptom | Cause | Recovery |
|---|---|---|
| `JTAG scan chain interrogation failed: all zeroes` | TDO low: no power, open TDO, SRST low | power and ground; check TDO and SRST |
| `JTAG scan chain interrogation failed: all ones` | TDO high: TDI/TDO swapped or floating | check the signal map and the TDO driver |
| `Target not examined yet` | a target command before examine | run `examine`, then retry |
| `tap/device found` with the wrong IDCODE | wrong target script, chain order or IR length | use the script for the exact part |
| IDCODE found, then examine or flash fails | wrong target script or chain order | use the script for the exact part |

## Related

| Doc | Why |
|---|---|
| [commands.md](commands.md) | the command set and reply shapes |
| [entry-and-exit.md](entry-and-exit.md) | entry state and the two-step exit |
| [host-setup.md](host-setup.md) | options, defaults and reference configs |
| [workflow.md](workflow.md) | the end-to-end sequence and health checks |
| [../../protocol/jtag/edge-cases.md](../../protocol/jtag/edge-cases.md) | the JTAG set's failure table |
| [../xsvf/limits-and-errors.md](../xsvf/limits-and-errors.md) | the sibling tool's limits |
