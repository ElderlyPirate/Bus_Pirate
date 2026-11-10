# 1-Wire — binary (`1W01`) sub-protocol

Part of the 1-Wire documentation set (`docs/1wire/`).  This page documents the
binary/host-programming interface to 1-Wire mode.

This page is written from the firmware's command definitions; it is
unverified.  The general binary layer (entering BBIO, framing, the shared
commands) is documented in `docs/generic/` (see
[commands.md](../../../generic/commands.md) §Leaving the CLI) and `docs/binary/` —
see [entering-and-framing.md](../../binary/entering-and-framing.md),
[commands.md](../../binary/commands.md) and
[sub-protocols.md](../../binary/sub-protocols.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x04` to select the 1-Wire protocol; the mode prints `1W01` and the
   1-Wire command loop starts.
3. Send `0x00` to leave 1-Wire; the `BBIO1` identifier is printed again.
   From BBIO, `0x0F` returns to the interactive terminal.

The `1W01` identifier stands for 1-Wire protocol version 1.

## Commands

The first nibble of each request byte selects the command group.

| Request | Group | Meaning | Reply |
|---|---|---|---|
| `0x00` | action | exit 1-Wire (back to BBIO) | `BBIO1` |
| `0x01` | action | version string | `1W01` |
| `0x02` | action | bus reset | `0x01` (success) |
| `0x04` | action | read one byte from the bus | one data byte |
| `0x08` | action | ROM search (macro `(240)`) | `0x01`, then device bytes, then 8 × `0xFF` |
| `0x09` | action | alarm search (macro `(236)`) | `0x01`, then device bytes, then 8 × `0xFF` |
| `0x1n` | bulk | write `n + 1` bytes (`n` = 0…15) | `0x01` after each byte |
| `0x2s` | configure | speed: `s` = 0 standard, 1 overdrive | `0x01` |
| `0x4v` | configure | peripherals (see below) | `0x01` |
| `0x5x` | configure | set the pull-up feed (v4 only) | `0x01` / `0x00` |
| `0x7x` | wait | wait for an edge on the AUX pin (shared) | shared reply |

- `0x01` is the success/ack value; `0x00` is failure.  An unrecognised request
  returns `0x00`.
- The bulk-write group sends exactly `n + 1` bytes: `n = 0` sends one byte and
  `n = 15` sends sixteen.

### Configure peripherals (`0x4v`)

The low four bits of `v` set the board's state:

| Bit | Mask | When set | When clear |
|---|---|---|---|
| 3 | `0x08` | power on | power off |
| 2 | `0x04` | pull-ups on | pull-ups off |
| 1 | `0x02` | AUX high | AUX low |
| 0 | `0x01` | CS high (released) | CS low (asserted) |

### Set the pull-up feed (`0x5x`, v4 only)

The low nibble selects the on-board source feeding the pull-up voltage (VPU);
the group is shared with the other binary modes (see
[../binary/sub-protocols.md](../../binary/sub-protocols.md) §Shared command
groups).

| Operand | Source |
|---|---|
| `0x51` | on-board **+3.3 V** |
| `0x52` | on-board **+5.0 V** |
| any other | both off |

The reply is `0x01` when the request was applied and `0x00` when it was refused
(the pins are not open-drain HiZ, or a voltage is already present on VPU).  The
group exists on v4 only; it is not in the v3 build.

## Worked exchange

The framing for a complete exchange:

```
host -> 0x04                 select 1-Wire
host <- "1W01"
host -> 0x02                 bus reset
host <- 0x01                 success
host -> 0x04                 read a byte
host <- <byte>
host -> 0x14                 bulk-write length 5
host <- 0x01                 ack, then five data bytes follow
host -> <byte 0> ... <byte 4>
host <- 0x01  (after each)
host -> 0x21                 configure speed = overdrive
host <- 0x01
host -> 0x00                 leave 1-Wire
host <- "BBIO1"
```

## v3 / v4

The action, bulk-write and configure commands are the same in both builds.
The pull-up feed (`0x5x`) is **v4 only**.  This sub-protocol is
unverified; both columns are from the firmware.