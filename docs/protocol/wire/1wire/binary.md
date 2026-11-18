# 1-Wire — binary (`1W01`) sub-protocol

Part of the [1-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page documents the
binary/host-programming interface to 1-Wire mode.  It is **unverified**.

The general binary layer (entering BBIO, framing, the shared commands) is in
[docs/protocol/binary/](../../binary/README.md) — see
[entering-and-framing.md](../../binary/entering-and-framing.md),
[commands.md](../../binary/commands.md) and
[sub-protocols.md](../../binary/sub-protocols.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x04` to select the 1-Wire protocol; the mode prints `1W01` and the
   1-Wire command loop starts.
3. Send `0x00` to leave 1-Wire; the `BBIO1` identifier is printed again.
   From BBIO, `0x0F` returns to the interactive terminal.

Entry applies its own state, regardless of the terminal session that preceded
it: **standard speed**, 8-bit width, MSB-first, open-drain high.  It does not
inherit a terminal `O` (overdrive) setting.  The `1W01` identifier stands for
1-Wire protocol version 1.

## Commands

The first nibble of each request byte selects the command group.

| Request | Group | Meaning | Reply |
|---|---|---|---|
| `0x00` | action | exit 1-Wire (back to BBIO) | `BBIO1` |
| `0x01` | action | version string | `1W01` |
| `0x02` | action | bus reset | `0x01` (ack only) |
| `0x03` | action | reserved | `0x00` |
| `0x04` | action | read one byte from the bus | one data byte |
| `0x05`–`0x07` | action | reserved | `0x00` |
| `0x08` | action | ROM search | `0x01`, then 8 bytes per device, then 8 × `0xFF` |
| `0x09` | action | alarm search | `0x01`, then 8 bytes per device, then 8 × `0xFF` |
| `0x0A`–`0x0F` | action | reserved | `0x00` |
| `0x1n` | bulk | write `n + 1` bytes (`n` = 0…15) | `0x01` before, then `0x01` after each byte |
| `0x2x`, low bits `00` (for example `0x20`) | configure | speed = standard | `0x01` |
| `0x2x`, low bits `01` (for example `0x21`) | configure | speed = overdrive | `0x01` |
| `0x2x`, low bits `10` or `11` | configure | reserved speed values | `0x00`, speed unchanged |
| `0x4v` | configure | peripherals (see below) | `0x01` |
| `0x5x` | configure | set the pull-up feed (v4 only) | `0x01` / `0x00` |
| `0x70` | wait | wait for an edge on AUX (shared) | result byte + level byte |
| `0x71`–`0x7F` | wait | not defined | `0x00`, no payload read |
| other | — | unrecognised | `0x00` |

- `0x01` is the success/ack value; `0x00` is failure.
- The bulk-write group sends exactly `n + 1` bytes: `n = 0` sends one byte and
  `n = 15` sends sixteen.
- **`0x02` (bus reset) always replies `0x01`.**  The reply only acknowledges the
  request; it does not report presence, an absent device, or a short.  For the
  reset result use the terminal reset messages (`BUS RESET …`), or read the
  bus to detect a device.
- **The speed group uses only the low two bits.**  `0x20` and `0x21` are the
  expected forms; values whose low two bits are 2 or 3 are answered `0x00`
  without changing the speed.

### Configure peripherals (`0x4v`)

The low four bits of `v` set the board's state:

| Bit | Mask | When set | When clear |
|---|---|---|---|
| 3 | `0x08` | power on | power off |
| 2 | `0x04` | pull-ups on | pull-ups off |
| 1 | `0x02` | AUX high | AUX low |
| 0 | `0x01` | CS high (released) | CS low (asserted) |

The AUX bit also forces the AUX pin to an output.  The pull-up bit switches
the resistors on; it does not choose their voltage source.  Select the source
with `0x5x` first.

### Set the pull-up feed (`0x5x`, v4 only)

The low nibble selects the on-board source feeding VPU:

| Operand | Source |
|---|---|
| `0x51` | on-board **+3.3 V** |
| `0x52` | on-board **+5.0 V** |
| any other | both off |

The reply is `0x01` when the request was applied and `0x00` when it was refused
(the pins are not open-drain HiZ, or a voltage is already present on VPU).  The
group is v4 only; on v3 the request returns `0x00`.

## Search stream

`0x08` (ROM search) and `0x09` (alarm search) reply `0x01`, then stream the
found devices:

```
host -> 0x08
host <- 0x01                 accepted
host <- <8 bytes>            device 1 ROM
host <- <8 bytes>            device 2 ROM
...
host <- 0xFF × 8             end of search
```

Each device is exactly eight bytes.  The binary search does **not** print the
family lookup and does **not** fill the terminal roster.  With no devices on
the bus, the stream is the eight `0xFF` bytes immediately after the `0x01`.

## Terminal vs binary

| Terminal | Binary | Notes |
|---|---|---|
| `[/{` reset | `0x02` | ack only; no presence/short result |
| `r` | `0x04` | one byte |
| a value | `0x1n` | n + 1 bytes; one ack before, one after each |
| `(240)` search | `0x08` | stream only; no roster, no family names |
| `(236)` alarm search | `0x09` | stream only |
| `O` toggle | `0x20` / `0x21` | entry resets to standard |
| `W`/`w` | `0x4x` bit 3 | supply on/off |
| `P`/`p` | `0x4x` bit 2 | pull-ups on/off; `0x5x` selects the VPU source |
| `e2`/`e3`/`e1` | `0x5x` | v4 only |
| `-` / `_` / `^` / `!` / `.` | — | no bit-level binary commands |
| `;N` | — | fixed at 8 bits |
| `(85)` / `(204)` / `(51)` | — | build from reset + bulk write + read |

There is no binary equivalent of the 1-Wire bit operations (`^`, `!`, `-`,
`_`, `.`), the ROM command macros other than the two searches, the roster
recall, or the width setting.

## Worked exchange

The framing for a complete exchange.  Device bytes are illustrative.

```
host -> 0x04                 select 1-Wire
host <- "1W01"
host -> 0x02                 bus reset
host <- 0x01                 ack only (no presence/short result)
host -> 0x14                 bulk-write length 5
host <- 0x01                 accepted
host -> 0x33                 data byte 1
host <- 0x01                 after byte 1
host -> 0x28                 data byte 2
host <- 0x01                 after byte 2
host -> 0x23                 data byte 3
host <- 0x01                 after byte 3
host -> 0x45                 data byte 4
host <- 0x01                 after byte 4
host -> 0x67                 data byte 5
host <- 0x01                 after byte 5
host -> 0x04                 read a byte
host <- <byte>
host -> 0x21                 configure speed = overdrive
host <- 0x01
host -> 0x00                 leave 1-Wire
host <- "BBIO1"
```

## Limits and edge cases

- **`0x02` is an ack, not a result.**  It returns `0x01` whether a device
  answered, the line was shorted, or nothing was on the bus.
- **The speed group rejects `0x22`/`0x23`.**  The previous speed stays in
  force.
- **The search stream has no roster side effect.**  The terminal macros
  `(1)`–`(10)` are not available over the binary link.
- **The 8 × `0xFF` terminator is not self-delimiting inside a ROM.**  Read
  groups of eight bytes; a ROM that contains `0xFF` bytes is still part of a
  device record.
- **The bulk group consumes exactly `n + 1` bytes.**  A command byte sent
  early is written to the bus as data.
- **No width, bit-op, ROM-macro or roster commands exist.**
- The `0x5x` group is v4 only; on v3 it returns `0x00`.
