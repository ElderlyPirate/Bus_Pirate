# UART — binary (`ART1`) sub-protocol

Part of the UART documentation set (`docs/protocol/uart/`).  The
byte-oriented interface to UART mode for host programs.  The general binary
layer (entering BBIO, framing, shared commands) is documented in
`docs/protocol/binary/` (see the
[binary-layer overview](../binary/README.md)); this page owns the UART
command set.  The engine used is always **full duplex** — the binary path
does not expose the single-wire mode.

## Entry and exit

1. From the terminal, send exactly **20 × `0x00`**; the mode prints `BBIO1`.
   Extra NULs sent after entry re-print `BBIO1` (they are the "reset" command
   inside the binary layer, not a repeat of the entry count).
2. Send `0x03`; the mode prints `ART1` and the UART command loop starts.
3. Send `0x00` to leave UART (back to `BBIO1`); from BBIO, `0x0F` returns to
   the interactive terminal.

On entry the port is configured to **300 bps, 8/NONE, one stop bit, idle 1,
open drain, echo off, full duplex**.  Because the default output is open
drain, bring the board up before expecting traffic: on v4 send the VPU feed
and enable power + pull-ups (see the worked exchange).

## Commands

The high nibble of each request byte is the command group; the low nibble is
the operand.

| Request | Group | Meaning | Reply |
|---|---|---|---|
| `0x00` | action | leave UART, return to bitbang | `BBIO1` |
| `0x01` | action | report identifier | `ART1` |
| `0x02` | action | turn echo on (forward received bytes to the host) | `0x01` |
| `0x03` | action | turn echo off | `0x01` |
| `0x07` | action | set the baud divider (2 bytes follow, big-endian) | `0x01` ×3 |
| `0x0F` | action | enter the bridge (see the trap note below) | `0x01` |
| `0x1x` | bulk | transmit `x + 1` bytes (`x` = 0…15) | `0x01`, then one `0x01` per byte |
| `0x4x` | configure | shared group: power, pull-ups, AUX, CS (`0x4C` = supply + pull-ups) | `0x01` |
| `0x5x` | configure | shared group: on-board pull-up feed (VPU): `0x51` = 3.3 V, `0x52` = 5.0 V, any other low nibble = off (**v4 only**) | `0x01` = applied, `0x00` = not applied (a voltage was already on VPU, or the output is not open drain) |
| `0x6x` | configure | select a preset speed (low nibble 0–9) | `0x01` |
| `0x7x` | wait | shared group: wait for an edge on the AUX pin | shared 2-byte reply |
| `0x8x` | configure | set data bits / parity / stop / polarity / output | `0x01` |
| `0x9x` | configure | treated the same as `0x8x` | `0x01` |
| other | — | unrecognised | `0x00` |

`0x4x`, `0x5x` and `0x7x` are **shared groups**: their bit and payload
definitions are owned by the binary layer, in
[shared command groups](../binary/sub-protocols.md#shared-command-groups).
This page adds only the UART operands and replies.

- `0x01` is success/ack; `0x00` is failure.
- The bulk group sends exactly `x + 1` bytes: `0x10` sends one byte (the
  reply is one command ack plus one ack per byte, so 17 acks for 16 bytes),
  `0x1F` sends sixteen.
- On v3 the `0x5x` pull-up feed group is not present, so a `0x5x` byte is
  answered `0x00`.

**`0x0F` is a one-way bridge trap.**  It acknowledges, then forwards the byte
stream to the bridge and stops acting on commands.  On v4 press the
**NORMAL button** to return to the `ART1` loop (then `0x00` to leave it); on
v3 nothing in software breaks the loop — only a **hardware reset**, which
reboots to `HiZ>`.  Do not send it unless you can reach the button or reset.

### Setting the baud divider (`0x07`)

Two bytes follow, most significant first: the raw baud divider.  The
firmware acknowledges before each byte and after the change (three `0x01`
bytes).  The divider relationship is `BRG = 4,000,000 / rate − 1`, so
`0x01A0` (416) gives ≈ 9600 bps.  This is the raw form; the host does the
arithmetic.

### Preset speeds (`0x6x`)

The low nibble selects a preset from the firmware's table:

| Operand | Rate (bps) |
|---|---|
| `0` | 300 |
| `1` | 1200 |
| `2` | 2400 |
| `3` | 4800 |
| `4` | 9600 |
| `5` | 19200 |
| `6` | 38400 |
| `7` | 57600 |
| `8` | 115200 |
| `9` | 31250 (MIDI) |

Use only `0`–`9`.  The operands `0xA`–`0xF` are **out of range**: the
firmware maps them to index 10 (`0xA` directly, `0xB`–`0xF` clamped), which
is one past the ten-entry preset table, so the divider selected is undefined
— not a usable extra speed.

### Configuring the frame (`0x8x` / `0x9x`)

The low four bits set the frame, and bit 4 selects the output type:

| Bit | Field |
|---|---|
| bit 4 | output type: `0` = open drain (default), `1` = normal push-pull |
| bits 3–2 | data bits / parity: `0` = 8/NONE, `1` = 8/EVEN, `2` = 8/ODD, `3` = 9/NONE |
| bit 1 | stop bits: `0` = one, `1` = two |
| bit 0 | receive polarity: `0` = idle 1, `1` = idle 0 |

So `0x80` = open drain, 8/NONE, one stop, idle 1.  There is **no
read-config command**: the `0x9x` group writes config just like `0x8x`
(older references describing `0x9x` as "read config" are stale).

## Worked exchange

Host → Bus Pirate and Bus Pirate → host, with the peer echoing the bytes
back.  The bring-up (`0x00` ×20, `0x03`, `0x51`, `0x4C`, `0x07`) is the
pattern used in this exchange; `0x51`/`0x4C` are needed because the port
comes up open drain with the rail off.

```
host -> 0x00 x20         enter bitbang
host <- "BBIO1"
host -> 0x03             select UART
host <- "ART1"
host -> 0x51             v4: set the on-board 3.3 V pull-up feed (VPU)
host <- 0x01             (0x00 = not applied: powered, or not open drain)
host -> 0x4C             peripherals: power + pull-ups on
host <- 0x01
host -> 0x07 0x01 0xA0   set divider 416 (≈ 9600 bps)
host <- 0x01 0x01 0x01
host -> 0x02             echo on
host <- 0x01

; send 4 bytes; the peer echoes them back
host -> 0x13 0x41 0x42 0x43 0x44
host <- 0x01             command ack
host <- 0x01 0x01 0x01 0x01   one ack per written byte
host <- 0x41 0x42 0x43 0x44   the echoed bytes (interleaved with the acks)

host -> 0x00             leave UART
host <- "BBIO1"
host -> 0x0F             leave bitbang
host <- "HiZ>"
```
In the host stream, the per-byte acks and the incoming bytes **interleave**;
a host program must separate the `0x01` acks from the data (send payloads
that avoid the value `0x01`).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| action / bulk / speed / divider / bridge | present | present |
| pull-up feed (`0x5x`) | — | present |
| `0x0F` bridge return | hardware reset only (reboot to `HiZ>`) | NORMAL button → `ART1` loop |