# The firmware player, from the host side

Part of the XSVF set.  This page is what a host author or an operator needs
to know about the v4 firmware sub-mode that plays the stream: who owns the
TAP, how a run is entered and left, the player commands, the chunk handshake
and the result byte.  The authoritative byte protocol is
[protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md); the host command is
[players.md](players.md).

## Scope

Covered: the player as seen from `bp xsvf` or another host.  Not covered: the
TAP state machine ([tap.md](../../protocol/jtag/tap.md)), the chain probe's
algorithm (the JTAG page), or the PC command ([players.md](players.md)).

## Who owns the TAP

JTAG runs only from the binary layer; there is no JTAG menu entry and no
terminal command.

| Path | Entry | TAP owner |
|---|---|---|
| OpenOCD path | `0x06` to `OCD1` | the host; the firmware only shifts bits |
| XSVF player | `0x18` to `XSV1` | the firmware player |

There is no standalone TAP engine.  In the XSVF player the firmware walks the
16 TAP states itself, following the file's commands, and drives TDI, TCK and
TMS.  That is why an XSVF file needs no host-side IR/DR description: the file
already contains the states, instructions and shift lengths.

## Availability

| | v3 | v4 |
|---|---|---|
| firmware player | not compiled | present in the default v4 build |
| hardware | — | required |
| host command | nothing to drive | `bp xsvf` |
| converter | PC-side tool | PC-side tool |

The player is the only part tied to the board.  An older or custom image may
not include it; `bp xsvf` then reports that the board did not enter XSVF
mode.  v3 has no player: the `0x18` selector is answered with `0x00`.

## Entry

1. Enter binary bitbang with 20 × `0x00`.  `bp xsvf` writes one leading
   `0x00` plus those twenty; the leading byte leaves most other binary
   sub-modes, not the player (which ignores `0x00`).  Extra NULs in `BBIO1`
   re-print the banner; drain it.
2. The host sends `0x18`.  The board switches on the on-board regulator
   (VREG), which drives both the `3.3V` and `5.0V` header rails, answers
   `XSV1`, and enters the player command loop.

`XSV1` is four bytes with no line ending.  There is no settings menu and no
per-session setting: entry fixes the wiring and the stream setup, and the
player has no TCK control.  `bp xsvf` performs both entry steps itself.

## Player commands

| Byte | Action | Reply |
|---|---|---|
| `0x01` | reset the chain (TMS high for ten TCK clocks), then Run-Test/Idle | none |
| `0x02` | probe the chain | count byte, then IDCODE bytes |
| `0x03` | play one XSVF stream | chunk handshake, then one result byte |
| `0x0F` | reset the chain and leave the player | none; back to `BBIO1` |
| other | ignored | none |

`0x00` is ignored; it does not leave the player.  The chain probe is one
count byte equal to four times the number of devices, then four IDCODE bytes
per device, least-significant byte first, nearest-TDO device first.  The
count is clamped to 63 devices.  `bp xsvf --scan` sends `0x02`.

## Playing a stream (`0x03`)

1. The player sends the ready byte `0xFF`.
2. The host sends a 16-bit **big-endian** chunk length, then that many bytes.
3. Steps 1 and 2 repeat whenever the player needs more data.
4. The stream ends at its `XCOMPLETE` command.
5. The player sends one result byte.

Keep every chunk at or below **4096 bytes**, the player's buffer.  A longer
chunk is drained to keep the stream aligned, but only the first 4096 bytes
are kept, so the run is corrupt.  A zero-length chunk is not data: the player
ignores it and requests again.

The initial TAP state is reset.  The player leaves the TAP where the file
leaves it, the last `XSTATE` or the `XENDIR`/`XENDDR` end state, and does not
add a final reset.  `--reset` is therefore optional before a file.

A minimal exchange, with the host already in `BBIO1`:

```
host -> 0x18           select the player
host <- XSV1
host -> 0x03           start a stream
host <- 0xFF           ready for data
host -> 0x10 0x00      chunk length 4096
host -> 4096 bytes     XSVF data
host <- 0xFF           ready for the last chunk
host -> 0x00 0x29      chunk length 41
host -> 41 bytes       XSVF data
host <- 0x00           result: no error
```

The lengths are big-endian.  `0xFF` is a byte, not the text "FF"; `XSV1` is
the four-byte banner.  The player may send `0xFF` again whenever it needs
more data; the host answers with the next length and chunk.

## Result byte

The result is one byte, sent after the stream, not after each chunk.

| Byte | Printed name | Meaning |
|---|---|---|
| `0x00` | `XSVF_ERROR_NONE` | the stream completed |
| `0x01` | `XSVF_ERROR_UNKNOWN` | unknown error; also a stall or a busy byte |
| `0x02` | `XSVF_ERROR_TDOMISMATCH` | captured TDO did not match the file |
| `0x03` | `XSVF_ERROR_MAXRETRIES` | still mismatched after the retries |
| `0x04` | `XSVF_ERROR_ILLEGALCMD` | illegal or unsupported command |
| `0x05` | `XSVF_ERROR_ILLEGALSTATE` | illegal TAP state in the stream |
| `0x06` | `XSVF_ERROR_DATAOVERFLOW` | a shift is larger than the buffer |
| `0x07` | `XSVF_ERROR_LAST` | internal end-of-list marker; not returned normally |

`0xFF` (`XSVF_READY_FOR_DATA`) is the ready byte, not a result.  A host that
receives an unknown byte should report `XSVF_ERROR_UNKNOWN`, as `bp xsvf`
does.

## Stall and busy bytes

If the host sends nothing for about **5 seconds** while the player waits for
a chunk, the player substitutes `0xFF` and the run ends with `0x01`, `0x04`,
`0x05` or `0x06`, depending on where that byte lands.  A byte sent while the
player is in a timed wait (`XWAIT` or `XRUNTEST`) is consumed and ends the
run with `0x01` at the next command boundary.  A stall inside an `XCOMMENT`
string exits the comment loop and returns `0x01`; the player loop remains
reachable.  The full mapping and the recovery are in
[limits-and-errors.md](limits-and-errors.md#stalls-busy-bytes-and-the-result-order).

## Exit

| Step | Reply | Effect |
|---|---|---|
| `0x0F` in the player | none | resets the chain and returns to `BBIO1` silently |
| `0x0F` in `BBIO1` | `0x01` | terminal reset; VREG off; banner, then `HiZ>` |
| board reset | — | terminal; VREG off |

The first step works only while the player is at its command loop.  During a
run it is consumed as data or as a busy byte.  It does not reset the pins and
does not switch VREG off, so the target stays powered between the two steps.
The pair requires a host that can write raw bytes; `bp xsvf` sends it only
with `--leave`, and only after a successful operation.  At the terminal,
press reset.  Both exits are in
[workflow.md](workflow.md#6-leave-the-player-and-recover-the-terminal).

## What the player does not do

- No TCK setting; the clock is fixed by the firmware.
- No SRST or TRST.
- No open-drain mode; the outputs are 3.3 V push-pull.
- No SVF input; convert first.
- No self-test.

## Related

| Doc | Why |
|---|---|
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the authoritative protocol and chain probe |
| [protocol/jtag/tap.md](../../protocol/jtag/tap.md) | TAP states, shifts, IDCODE, bypass |
| [protocol/jtag/entry.md](../../protocol/jtag/entry.md) | both JTAG paths' entry and exit rules |
| [players.md](players.md) | the host command and its options |
| [wiring.md](wiring.md) | wires, rails, levels |
| [limits-and-errors.md](limits-and-errors.md) | limits, result actions, recovery |
