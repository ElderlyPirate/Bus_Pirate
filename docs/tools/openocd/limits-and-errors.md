# OpenOCD limits, errors and recovery

Part of the OpenOCD adapter set (`docs/tools/openocd/`).  This page owns what
the firmware path and the host driver cannot do, what their messages mean, and
how to recover.  The byte-level protocol is defined in
[protocol/jtag/openocd.md](../../protocol/jtag/openocd.md); this page is the
host-side reading of it.

## Scope

Firmware limits (TAP shifter only, shift clamp, speed and delay), host-driver
limits, the exit rules, the message tables and the recovery steps.  It does not
repeat the in-mode command table; follow the link for that.

## Firmware limits

| Limit | Value | Host-side consequence |
|---|---|---|
| shift per `0x05` command | 8192 bits | keep each scan at or below 8192 bits; OpenOCD does not split a longer scan |
| TAP knowledge | none | the host and its target config own IR/DR/chain |
| persistent state | pin levels only | the host tracks the TAP position across commands |
| default pin state at entry | all inputs (Hi-Z) | set a pin mode before the first shift |
| JTAG clock | no host control | `adapter speed` does not change TCK |
| inter-bit delay | default `1` | OpenOCD never changes it, so the default applies |
| link speed | normal / fast | v4 accepts `fast` but does not change the rate |
| v3 | build-only | not verified |

### The TAP-shifter boundary

The firmware shifts bits; it does not model the TAP.  It has:

- no instruction-register or data-register descriptions;
- no chain discovery, IDCODE read or chain-order knowledge;
- no automatic TMS reset pattern or state-transition shortcuts;
- no knowledge of the target's reset behaviour or flash layout.

All of that is the host's.  The target configuration in OpenOCD supplies the
IR lengths, the IDCODEs, the flash banks and the reset sequence.  If the
target script names a different part or chain order, the probe can still read
an IDCODE but examine and flash fail.  Do not read a successful entry as a
successful target probe.

### The 8192-bit shift clamp

`0x05` carries a 16-bit count of bit sequences, capped at 8192 bits.  The
reply echoes the **requested** count even when the firmware clamped it, so a
host cannot detect the clamp from the echo.

OpenOCD flushes its scan queue at the 8192-bit boundary, but it does **not**
split a single scan that is longer.  One scan above 8192 bits overflows the
driver's chain buffer (`tap_chain overflow`) and aborts the session.  Keep
any one scan at or below 8192 bits.

The firmware clamp is not a safe way to truncate an oversized shift: it
consumes only the clamped payload, and the leftover TDI/TMS bytes stay in the
serial stream and desynchronise the next command.  A custom host must split
long scans itself before sending them.

### Speed and delay

| Setting | Behaviour |
|---|---|
| `buspirate speed normal` | 115200 link; always safe |
| `buspirate speed fast` (v3) | 1 Mbaud physical link |
| `buspirate speed fast` (v4) | accepted and acknowledged; the CDC rate does not change |
| `0x08` clock delay | two bytes, default `1`; a larger value slows TCK |

`0x07` (link speed) is not a success code: the reply repeats the selector, and
a bad `0xAA 0x55` confirm forces the link back to normal.  A custom host must
switch its own port to the requested rate after the command byte, then send
the confirm at that rate; OpenOCD does this for you.

OpenOCD's Bus Pirate driver has no JTAG clock control and never sends the
`0x08` delay command.  A stock session therefore runs at the firmware's
default delay, and `adapter speed` in a target script is cosmetic for this
adapter.  If a chain needs a slower clock, a custom host must set `0x08`
itself.

## Host-driver limits

| Limit | Detail |
|---|---|
| POSIX serial only | native Windows (MinGW) builds exclude the driver; a Cygwin/POSIX layer may work |
| one owner of the port | a terminal or another program holding the port blocks the driver |
| one OpenOCD session | a second instance cannot open the adapter |
| open-drain by default | an unconfigured session needs pull-ups |
| reset commands need SRST | without SRST wiring, reset commands do not reset the CPU |
| no config shipped | interface, target and clock settings are the host's responsibility |

## Leaving cleanly

| Action | Effect | Next |
|---|---|---|
| `0x00` inside `OCD1` | leaves the mode; pins and regulator reset; `BBIO1` | `0x0F` to the terminal |
| `0x0F` inside `BBIO1` | `0x01`, terminal reset; v4 prints `HiZ>` | re-enter with 20 × `0x00` |
| OpenOCD `shutdown` | sets Hi-Z, normal speed, then `0x00` and `0x0F` | board at `HiZ>`, supplies off |

Three rules matter when recovering by hand:

- **`0x0F` is not an `OCD1` command.**  Inside the mode it is answered as an
  unknown command with a single `0x00`; it does not leave.  Send `0x00`
  first.
- **A killed host leaves the board in `OCD1`.**  OpenOCD normally performs
  the exit sequence.  If it is killed **between commands**, send `0x00`
  (back to `BBIO1`), then `0x0F`.
- **A kill mid-shift needs a reset.**  While a `0x05` shift is in progress
  the firmware is still reading TDI/TMS payload, so `0x00` and `0x0F` are
  consumed as shift data and do not leave the mode.  Press reset (the
  reliable action), or send enough remaining TDI/TMS bytes to finish the
  shift before sending `0x00`.

The XSVF player is the exception to the shared exit pattern (it ignores
`0x00` and leaves on `0x0F`); the comparison table is in
[protocol/jtag/README.md](../../protocol/jtag/README.md) §Leaving the modes.

## Driver messages

Wording varies between OpenOCD versions; these are the current driver strings.

| Message | Cause | Recovery |
|---|---|---|
| `You need to specify the serial port!` | no `buspirate port` | add the option |
| `Could not open serial port` | missing device, permissions, or port in use | check the device; close the terminal |
| `Buspirate error. Is binary/OpenOCD support enabled?` | no `BBIO1` after entry | check board state, port and firmware |
| `Buspirate did not answer correctly! Do you have correct firmware?` | unexpected reply | reset; close other programs |
| `Unsupported binary protocol` | a `BBIOx` other than `BBIO1` | use matching firmware |
| `Buspirate did not ack speed change` | the `0x07` handshake failed | use `speed normal`; check the link |
| `Buspirate did not reply as expected to the speed change command` | speed reply mismatch | use `speed normal` |
| `Unable to restart buspirate!` | `0x00` did not return `BBIO1` | press reset; the next session re-enters |
| `Some stale data from a previous connection was discarded.` | leftover terminal bytes | informational |

## Target and chain messages

These come from OpenOCD and the target config, not from the Bus Pirate.

| Message | Cause | Recovery |
|---|---|---|
| `JTAG scan chain interrogation failed: all zeroes` | TDO low: no power, open TDO, SRST low | power/ground; check TDO and SRST |
| `JTAG scan chain interrogation failed: all ones` | TDO high: TDI/TDO swapped or floating | check the signal map and TDO driver |
| `Target not examined yet` | a target command before examine | run `examine`, then retry |
| IDCODE found, then examine/flash fails | wrong target script or chain order | use the script for the exact part |
| unknown adapter driver at startup | OpenOCD built without the driver | use a build that includes it |

## Recovery

| Situation | Recovery |
|---|---|
| OpenOCD exited normally | board is at `HiZ>`; reopen the terminal or start a session |
| OpenOCD was killed between commands | board is in `OCD1`; send `0x00` (back to `BBIO1`), then `0x0F` |
| OpenOCD was killed mid-shift | the shift is still reading payload; press reset, or finish the shift with the remaining TDI/TMS bytes, then send `0x00` |
| entry fails after a previous session | port held elsewhere, or board still in a binary sub-mode; close or reset |
| chain probes but the part is wrong | fix the target config (IR length, IDCODE, chain order) |
| no clock or data in open-drain | fit 2–10 kΩ pull-ups, or set `buspirate mode normal` |
| target stays in reset | check SRST/TRST wiring and levels; the driver deasserts both |
| VREG enabled with the target's own supply | set `buspirate vreg 0`; never back-drive the target |
| pull-ups on with no Vpullup | no effect; feed Vpullup or use per-line pull-ups |
| programming fails after a good probe | wrong flash driver or image; check `flash banks` |

## v3 and unverified

- The OpenOCD path is present on both builds, but **v3 behaviour is
  build-only and not verified**.  The shift loop and the fast-link change
  differ between builds; the byte protocol is the same.
- The 8192-bit clamp, the v4 no-op speed change and the default delay are
  code-derived; their effect on a given target is **unverified**.
- OpenOCD-side details (driver default open-drain, no clock control,
  MinGW-excluded POSIX build) are properties of current OpenOCD releases and
  can change between versions.

## Related docs

| Doc | Why |
|---|---|
| [protocol/jtag/openocd.md](../../protocol/jtag/openocd.md) | the in-mode protocol and reply shapes |
| [protocol/jtag/edge-cases.md](../../protocol/jtag/edge-cases.md) | the JTAG set's failure table |
| [protocol/jtag/README.md](../../protocol/jtag/README.md) | exit rules and availability |
| [host-setup.md](host-setup.md) | options, defaults and reference configs |
| [workflow.md](workflow.md) | the end-to-end sequence and health checks |
| [../xsvf/limits-and-errors.md](../xsvf/limits-and-errors.md) | the sibling tool's limits |
