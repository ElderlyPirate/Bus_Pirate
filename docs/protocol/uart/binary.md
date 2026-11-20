# UART — binary (`ART1`) sub-protocol

Part of the UART documentation set (`docs/protocol/uart/`).  The
byte-oriented interface to UART mode for host programs.  The general binary
layer (entering BBIO, framing, shared commands) is documented in
`docs/protocol/binary/` — see the
[binary-layer overview](../binary/README.md); this page owns the UART
command set.  The engine used is always **full duplex** — the binary path
does not expose the single-wire mode.

**`0x0F` is a one-way bridge inside `ART1`, not a terminal return.**  It
replies `0x01`, then forwards bytes and stops acting on commands.  On v4 the
NORMAL button returns to the `ART1` loop; on v3 only a hardware reset leaves
it.  Do not send it unless you can reach the button or reset.  See
[§The `0x0F` bridge trap](#the-0x0f-bridge-trap).

## Entry and exit

1. From the terminal, send exactly **20 consecutive `0x00`** bytes; the mode
   prints `BBIO1`.  Extra NULs sent after entry re-print `BBIO1` (the in-layer
   `0x00` identifier command, not a repeat of the entry count).  A byte other
   than NUL does not continue the count.
2. Send `0x03`; the mode prints `ART1` and the UART command loop starts.
3. Send `0x00` to leave UART (back to `BBIO1`); from BBIO, `0x0F` returns to
   the interactive terminal.

On entry the port is configured to **300 bps, 8/NONE, one stop bit, idle 1,
open drain, echo off, full duplex**.  It does **not** inherit the terminal
UART settings.  Because the default output is open drain, bring the board up
before expecting traffic: on **v4** send the VPU feed and enable power +
pull-ups; on **v3** there is no feed command — feed `Vpu` externally and
enable power + pull-ups, or select normal output.  The bring-up table in
[§Worked exchange](#worked-exchange) shows both.

Leaving to the terminal differs by build: `0x0F` replies `0x01`, then on v4
prints the banner and `HiZ>`; on v3 it performs a reset, which reboots to
`HiZ>`.

### The same byte in BBIO1 and ART1

The same request byte means different things in the two states.  Check the
protocol state, not just the byte:

| Byte | In `BBIO1` | In `ART1` |
|---|---|---|
| `0x00` | re-print `BBIO1` (no state change) | leave UART; back to `BBIO1` |
| `0x01` | enter SPI | report `ART1` |
| `0x03` | enter UART | echo off |
| `0x07` | enter PIC | set the divider (two bytes follow) |
| `0x0F` | return to the terminal | enter the bridge (one-way trap) |

## Commands

The high nibble of each request byte is the command group; the low nibble is
the operand.

| Request | Group | Meaning | Reply |
|---|---|---|---|
| `0x00` | action | leave UART, return to bitbang | `BBIO1` |
| `0x01` | action | report identifier | `ART1` |
| `0x02` | action | turn echo on (forward received words to the host) | `0x01` |
| `0x03` | action | turn echo off | `0x01` |
| `0x07` | action | set the baud divider (2 bytes follow, big-endian) | `0x01` ×3 |
| `0x0F` | action | enter the bridge (see the trap note) | `0x01` |
| `0x1x` | bulk | transmit `x + 1` bytes (`x` = 0…15) | `0x01`, then one `0x01` per byte |
| `0x2x`, `0x3x` | — | not present in UART | `0x00` |
| `0x4x` | configure | shared group: power, pull-ups, AUX, CS (`0x4C` = supply + pull-ups) | `0x01` |
| `0x5x` | configure | **v4 only** — shared group: pull-up feed (VPU): `0x51` = 3.3 V, `0x52` = 5.0 V, any other low nibble = off | `0x01` = applied, `0x00` = refused |
| `0x6x` | configure | select a preset speed (low nibble 0–9) | `0x01` |
| `0x7x` | wait | shared group: wait for an edge on the AUX pin | shared 2-byte reply |
| `0x8x` | configure | set data bits / parity / stop / polarity / output | `0x01` |
| `0x9x` | configure | treated the same as `0x8x` (write) | `0x01` |
| other | — | unrecognised | `0x00` |

`0x4x`, `0x5x` and `0x7x` are **shared groups**: their bit and payload
definitions are owned by the binary layer, in
[shared command groups](../binary/sub-protocols.md#shared-command-groups).
This page adds only the UART operands and replies.  `0x4C` also asserts CS
in the shared group's bit 0; UART does not use CS.

- `0x01` is success/ack; `0x00` is failure.
- The bulk group sends exactly `x + 1` bytes: `0x10` sends one byte and
  `0x1F` sends sixteen.  The reply is one command ACK plus one ACK per byte
  (`0x1F` therefore gets 17 ACKs).
- A command's ACK and its per-byte ACKs are **kept together**.  Queued RX
  echo is serviced **between commands** (including between successive bulk
  writes), never inside that ACK group.
- Echo (`0x02`), the `0x0F` bridge and the bridge/monitor forward only the
  **low eight data bits** — the ninth bit and the error flags are not
  encoded into the forwarded stream.
- With echo off, received words are still drained from the ring and
  discarded; they are not queued for later.

Before each command the loop services queued RX up to the number of words
queued at that moment, so a continuously refilling stream cannot starve
command handling.  If the host is not ready to accept a byte, the service
leaves the rest in the 127-word ring; that is a liveness aid, not flow
control.

### The `0x0F` bridge trap

**`0x0F` is a one-way bridge trap.**  It replies `0x01`, then forwards the
byte stream to the bridge and stops acting on commands.  On v4 press the
**NORMAL button** to return to the `ART1` loop (then `0x00` to leave it); on
v3 nothing in software breaks the loop — only a **hardware reset**, which
reboots to `HiZ>`.  Do not send it unless you can reach the button or reset.

### Setting the baud divider (`0x07`)

Two bytes follow, most significant first: the raw baud divider.  The reply is
three `0x01` bytes: the command ACK, an ACK after the high byte, and an ACK
after the low byte and the new divider are applied.

The divider relationship is `rate = 4,000,000 / (divider + 1)`, so `0x01A0`
(416) gives about 9600 bps.  This is the raw form; the host does the
arithmetic.  Divider `0` is accepted as given and selects the fastest
divider (about 4 Mbaud at the 16 MHz core clock), not a default.  A divider
change disables and re-enables the port, which clears queued receive words.

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

Use only `0`–`9`.  The operands `0xA`–`0xF` are **out of range**: they are
answered `0x00` and the port is **not** reconfigured — the previous divider
stays.  A speed change clears queued receive words.

### Configuring the frame (`0x8x` / `0x9x`)

The low four bits set the frame, and bit 4 selects the output type:

| Bit | Field |
|---|---|
| bit 4 | output type: `0` = open drain (default), `1` = normal push-pull |
| bits 3–2 | data bits / parity: `0` = 8/NONE, `1` = 8/EVEN, `2` = 8/ODD, `3` = 9/NONE |
| bit 1 | stop bits: `0` = one, `1` = two |
| bit 0 | receive polarity: `0` = idle 1, `1` = idle 0 |

So `0x80` = open drain, 8/NONE, one stop, idle 1.  `0x90` = the same frame
with normal output.  `0x9F` = normal output, 9/NONE, two stop bits, idle 0.
The output-type bit applies to the TX line; open drain needs a pull-up.

There is **no read-config command**: the `0x9x` group writes config just like
`0x8x`.  A frame change reuses the current divider and clears queued receive
words.  A 9-bit frame does not make bulk writes 9-bit: `0x1x` carries one byte
per transfer, so the ninth bit is sent as zero.

## Worked exchange

Host → Bus Pirate and Bus Pirate → host, with the peer echoing the bytes
back.  The port comes up open drain with the rail off, so bring it up first:

| Build | Bring-up |
|---|---|
| v4 | `0x51` (VPU feed), then `0x4C` (power + pull-ups) |
| v3 | feed `Vpu` externally at the target's logic voltage, then `0x4C`; or, for a self-powered target, send `0x90` for normal output (8/NONE, one stop, idle 1) — `0x90` does not enable the target rail |

The exchange below uses the v4 sequence.  Send `0x51` before `0x4C` so the
feed is selected before the pull-up resistors connect.

```
host -> 0x00 x20         enter bitbang
host <- "BBIO1"
host -> 0x03             select UART
host <- "ART1"
host -> 0x51             v4: set the on-board 3.3 V pull-up feed (VPU)
host <- 0x01             (0x00 = refused: VPU powered, not open drain, or ADC busy)
host -> 0x4C             peripherals: power + pull-ups on (CS asserted)
host <- 0x01
host -> 0x07 0x01 0xA0   set divider 416 (about 9600 bps)
host <- 0x01 0x01 0x01
host -> 0x02             echo on
host <- 0x01

; send 4 bytes; the peer echoes them back
host -> 0x13 0x41 0x42 0x43 0x44
host <- 0x01             command ACK
host <- 0x01 0x01 0x01 0x01   one ACK per written byte
host <- 0x41 0x42 0x43 0x44   echo, before the next command

host -> 0x00             leave UART
host <- "BBIO1"
host -> 0x0F             leave bitbang
host <- 0x01             return ACK
host <- "HiZ>"
```

The command ACK and its per-byte ACKs arrive **together**.  Queued RX echo is
forwarded **between commands**, not inside that group.  For each command read
the expected number of `0x01` ACKs (one command ACK plus one per byte for a
bulk write); any additional bytes before the next command are RX data.  Echo
carries only the low eight data bits.  If pacing matters, note that the
127-word receive ring is finite — the host-readiness check keeps the loop
live but is not flow control.

## Limits

- **Full duplex only.**  There is no binary selection of the single-wire
  mode; entering `ART1` sets the port back to separate TX/RX pins.
- **Bulk is 8-bit.**  A 9-bit frame still gets a zero ninth bit on a bulk
  write.
- **No read-config.**  Track the last `0x8x`/`0x9x` write host-side; there is
  no read-back, and the terminal `i` header describes the terminal mode, not
  the `ART1` state.
- **Echo off discards RX.**  Received words are drained and dropped, not
  queued.
- **`0x0F` loses the command channel** until the button (v4) or a reset (v3).
- **No target flow control.**  The `0x0F` bridge shares the terminal bridge's
  limits.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| action / bulk / speed / divider / bridge | yes | yes |
| pull-up feed (`0x5x`) | — (answered `0x00`) | yes |
| `0x0F` bridge return | hardware reset only (reboot to `HiZ>`) | NORMAL button → `ART1` loop |
| `0x6x` operands `0xA`–`0xF` | `0x00`, no reconfigure | same |
| 9-bit bulk | ninth bit zero | same |
| host link | FTDI | CDC (host-controlled) |

## Related docs

- [binary-layer overview](../binary/README.md) — BBIO1 entry and framing.
- [shared command groups](../binary/sub-protocols.md#shared-command-groups) —
  `0x4x`, `0x5x`, `0x7x`.
- [entry-and-setup.md](entry-and-setup.md) — the terminal UART settings that
  `ART1` does not inherit.
- [errors.md](errors.md) — the receive ring and overruns.
