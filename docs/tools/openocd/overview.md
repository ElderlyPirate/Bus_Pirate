# OpenOCD path — what it is and how it differs from the XSVF player

Part of the OpenOCD adapter set.  This page explains what the firmware's
`0x06` path does, where the boundary between host and firmware lies, and how
the path compares with the XSVF player.  Entry and exit are in
[entry-and-exit.md](entry-and-exit.md); the byte protocol is in
[commands.md](commands.md); the TAP itself is
[protocol/jtag/tap.md](../../protocol/jtag/tap.md).

## Scope

- **Covered:** the job of the path, the host/firmware split, the naming, the
  comparison with the XSVF player, and availability.
- **Not covered:** TAP mechanics, the wiring detail, host configuration and
  the failure tables; each has its own page in this set or in the JTAG set.

## Where the work happens

| Layer | Owns | Example |
|---|---|---|
| host debugger | TAP state machine, IR/DR lengths, chain order, flash algorithms, GDB server | OpenOCD |
| `OCD1` protocol | pin modes, features, TAP-shift blocks, ADC, link speed, TCK delay | this set |
| firmware path | drives TCK, TMS and TDI; samples TDO; returns TDO bytes | `0x06` on both builds |
| target TAP | the scan chain itself | the part on the board |

The firmware shifts bits and nothing else.  It has no instruction-register
or data-register descriptions, no chain discovery, no IDCODE handling, no
automatic TMS reset pattern, and no knowledge of the target's flash.  The
host's target configuration supplies all of that.

The host must therefore:

1. Walk the TAP with TMS, including the reset sequence.
2. Know each device's IR length, DR length and chain position.
3. Read and check IDCODEs.
4. Build every scan and keep the whole `0x05` block, including its TAP
   moves, inside 8192 bits.
5. Decide when to use SRST and TRST.

## Naming

| Term | Means |
|---|---|
| OpenOCD path | the firmware mode selected by `0x06`; answers `OCD1` |
| OpenOCD adapter | the host side: OpenOCD's `buspirate` driver and this set |
| XSVF player | the other JTAG path: `0x18`, answers `XSV1`, v4 only |
| `AUX` | the header pin used for SRST; AUX target `A0` |
| `AUX1` | the v4 header pin used for TRST; AUX target `A1` |
| `PGD` | the v3 programming-header pin used for TRST |

There is no JTAG entry on the mode menu and no terminal command set.  The
path is reached from the binary layer only, and a host program is required.

## Compared with the XSVF player

Both paths drive the same four JTAG signals from the same header pins.  They
differ in who owns the TAP:

| Item | OpenOCD path | XSVF player |
|---|---|---|
| selector in `BBIO1` | `0x06` | `0x18` |
| identifier | `OCD1` | `XSV1` |
| builds | v3 and v4 | v4 only; v3 answers `0x00` |
| TAP owner | the host | the firmware player |
| shift source | the host, bit by bit through `0x05` | an XSVF file streamed by the host |
| longest shift | 8192 bits per `0x05` block | 400 bits per XSVF shift |
| pin drive | push-pull or open-drain, host-selected | 3.3 V push-pull only |
| VREG at entry | off; entry switches it off, and the host turns it on with the VREG feature | on; stays on after the player returns to `BBIO1` |
| SRST / TRST | optional; host-driven features | not driven |
| host software | OpenOCD's `buspirate` driver or a custom `OCD1` host | a bundled host player or a custom stream host |
| host OS | POSIX only | Windows or Linux |
| exit | `0x00`, then `0x0F` | `0x0F` from the player's command loop |
| typical use | halt, step, inspect, flash, any JTAG part | configure or program a CPLD/FPGA from a file |

`0x06` entry and the `0x00` return both run the BBIO pin reset, which
switches VREG and the on-board pull-ups off.  The 20-NUL binary entry runs
the same reset first.  `0x0F` then returns to the terminal (v3: a hardware
reset); it does not change the supply state.  The XSVF player switches VREG
on and leaves it on when it returns to `BBIO1`.

Consequences:

- A **v3** board can use the OpenOCD path; it cannot use the XSVF player.
- A **1.8 V or 2.5 V** target can use the OpenOCD path in open-drain mode
  with pull-ups to its own rail; the XSVF player cannot drive it.
- A **file-driven** CPLD/FPGA configuration is the XSVF player's job.  A
  board with no host debugger can still be configured that way.
- **Interactive debug** is the OpenOCD path's job: the XSVF player has no
  halt, step or memory access.
- OpenOCD can play an SVF file through the `0x06` path with its `svf`
  command; the XSVF player reads XSVF only.

## Availability

| Aspect | v3 | v4 |
|---|---|---|
| firmware path (`0x06`) | present, not verified on v3 | present |
| pin mode `0x01` push-pull | present, not verified on v3 | present |
| pin mode `0x02` open-drain | present, not verified on v3 | present |
| `0x03` ADC read | present, not verified on v3 | present |
| `0x05` TAP shift | present, not verified on v3 | present |
| `0x07` link speed fast | changes the UART to about 1 Mbaud | accepted; CDC rate unchanged |
| `0x08` TCK delay | applied | applied |
| SRST on `AUX` | yes | yes |
| TRST | `PGD` | `AUX1` |
| OpenOCD host driver | POSIX only | POSIX only |

The v3 build includes the path, but its hardware behaviour is **not verified
on v3**.

## Limits (summary)

- The firmware is a shifter, not a JTAG engine.
- One `0x05` block carries at most 8192 bits, including its TAP moves; the
  TDI and TMS lane buffers are 1024 bytes each.
- The host driver is POSIX-only.
- The path cannot select a rail voltage.
- No terminal mode, no self-test and no chain discovery.

## Related

| Doc | Why |
|---|---|
| [entry-and-exit.md](entry-and-exit.md) | the exact entry and exit bytes |
| [commands.md](commands.md) | every `OCD1` command |
| [wiring.md](wiring.md) | rails, pull-ups, resets |
| [../xsvf/README.md](../xsvf/README.md) | the XSVF tool set |
| [../../protocol/jtag/tap.md](../../protocol/jtag/tap.md) | the TAP reference |
| [../../protocol/jtag/README.md](../../protocol/jtag/README.md) | the JTAG set overview |
