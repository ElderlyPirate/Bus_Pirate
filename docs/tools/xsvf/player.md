# The firmware player, from the tool side

Part of the XSVF host-tool set.  This page is the host tool's view of the
firmware player: who owns the TAP, how a run is entered and left, the four
player commands, the chunk handshake, and the result byte.  The authoritative
byte-level protocol is [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md); the
TAP is [tap.md](../../protocol/jtag/tap.md); wiring is [wiring.md](wiring.md).

## Scope

Covered: the byte sequence a host must produce after it reaches `BBIO1`, the
result byte, and the failure results a tool must handle.  Not covered: the
TAP state machine and shift rules ([tap.md](../../protocol/jtag/tap.md)), the
probe internals (the JTAG page), or the PC programs
([players.md](players.md)).

## Who owns the TAP

JTAG runs only from the binary layer; there is no JTAG menu entry and no
terminal command.

| Path | Entry | TAP owner |
|---|---|---|
| OpenOCD path | `0x06` → `OCD1` | the host; the firmware only shifts bits |
| XSVF player | `0x18` → `XSV1` | the firmware player |

There is no standalone TAP engine.  In the XSVF player the firmware walks the
16 TAP states itself, following the file's commands, and drives TDI, TCK and
TMS.  For the duration of a run it is the TAP engine; between runs it only
holds the state the file left.  That is why an XSVF file needs no host-side
IR/DR description: the file already contains the states, the instructions and
the shift lengths.

## Availability

| | v3 | v4 |
|---|---|---|
| firmware player | not compiled | present in the default build |
| hardware | — | required |
| host players | nothing to drive | Windows or Linux host |
| SVF→XSVF converter | host-side tool | host-side tool |

The player is the only part tied to the board; the converter and the host
players are PC programs.

## Entry

1. Send 20 consecutive `0x00` bytes; the layer answers `BBIO1`.
2. Send `0x18`.  The board switches on the on-board regulator (VREG), which
   drives both the `3.3V` and `5.0V` header rails, and answers `XSV1`.
3. Send a player command.

`XSV1` is four bytes with no line ending.  The host players send NUL bytes
until they see `BBIO1` rather than a fixed count.

There is no settings menu and no per-session settings.  Entry fixes the
wiring and the stream setup, and the player has no TCK setting.

## Exit

| Step | Reply | Effect |
|---|---|---|
| `0x0F` in the player | none | resets the chain and returns to `BBIO1` silently |
| `0x0F` in `BBIO1` | `0x01` | terminal reset; VREG off; version banner, then `HiZ>` |
| board reset | — | terminal; VREG off |

The first `0x0F` is a player command and works only while the player is at
its command loop.  During a run it is consumed as data or as a busy byte.
That step does not reset the pins and does not switch VREG off, so the target
stays powered between the two steps.  The two steps need a host that can
write raw bytes; the shipped players close the port and cannot send them.  At
the terminal, press reset.  Both exits are in
[workflow.md](workflow.md#6-leave-the-player).

`0x00` is ignored by the player — it does not leave.  Any other byte that is
not a player command is ignored too.

## Player commands

| Byte | Action | Reply |
|---|---|---|
| `0x01` | reset the chain, then Run-Test/Idle | none |
| `0x02` | probe the chain | count byte, then IDCODE bytes |
| `0x03` | play one XSVF stream | chunk handshake, then one result byte |
| `0x0F` | reset the chain and leave the player | none |
| other | ignored | none |

`0x01` holds TMS high for ten TCK clocks.  `0x02` is a convenience scan: one
count byte equal to four times the number of devices, then four IDCODE bytes
per device, least-significant byte first, **nearest-TDO device first**.  The
count is clamped to 63 devices.  The probe algorithm, its 510-bit IR
assumption and its failure shapes are in
[protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md#chain-probe-0x02).

## Playing a stream (`0x03`)

1. The player sends the ready byte `0xFF`.
2. The host sends a 16-bit **big-endian** chunk length, then that many bytes.
3. Steps 1–2 repeat whenever the player needs more data.
4. The stream ends at its `XCOMPLETE` command.
5. The player sends one result byte.

Keep every chunk at or below **4096 bytes**, the player's buffer.  A longer
chunk is drained to keep the stream aligned, but only the first 4096 bytes
are used, so the run is corrupt.  A zero-length chunk is not data: the player
ignores it and requests again.

The initial TAP state is reset.  The player leaves the TAP where the file
leaves it — the last `XSTATE`, or the `XENDIR`/`XENDDR` end state — and does
not add a final reset.

## Result byte

The result is one byte, sent after the stream, not after each chunk.  The
host players print the `XSVF_ERROR_*` name for it.

| Byte | Printed name | Meaning |
|---|---|---|
| `0x00` | `XSVF_ERROR_NONE` | the stream completed |
| `0x01` | `XSVF_ERROR_UNKNOWN` | unknown error; also a stall or a busy byte |
| `0x02` | `XSVF_ERROR_TDOMISMATCH` | captured TDO did not match the file |
| `0x03` | `XSVF_ERROR_MAXRETRIES` | still mismatched after the retries |
| `0x04` | `XSVF_ERROR_ILLEGALCMD` | illegal or unsupported command |
| `0x05` | `XSVF_ERROR_ILLEGALSTATE` | illegal TAP state in the stream |
| `0x06` | `XSVF_ERROR_DATAOVERFLOW` | a shift is larger than the buffer |

`0x07` (`XSVF_ERROR_LAST`) is the end-of-list marker in the firmware's error
list and in the host players' name table; it is never returned.  `0xFF`
(`XSVF_READY_FOR_DATA`) is the ready byte, not a result.

The stall-related checks run in this order: the stall flag at the command
boundary (`0x01`), the command byte (`0x04`), then a state or size inside the
command (`0x05` or `0x06`).  The first one reached ends the run and is the
byte the host reads.

## Where a stalled stream ends

The player gives up on a chunk after about **5 seconds** without host data and
substitutes `0xFF` for the missing bytes.  The result depends on where that
`0xFF` lands:

| Where the stall lands | Result |
|---|---|
| noticed at the next command boundary | `0x01` |
| read as the command byte | `0x04` |
| read as a TAP-state byte | `0x05` |
| read as an `XSDRSIZE`/`XSIR2` length over the buffer | `0x06` |

The player checks its stall flag at the start of each command, so `0x01` wins
whenever a command completes and the flag is set.  A byte sent while the
player is in a timed wait (`XWAIT` or `XRUNTEST`) is consumed the same way:
the run ends with `0x01` at the next command boundary, unless an earlier
error (a TDO mismatch, for example) is already latched.

**One exception: a stall inside an `XCOMMENT` string.**  The player reads the
comment until a zero byte, and a stalled read returns `0xFF` forever, so the
loop never ends and no result is sent.  A converted file can contain comment
text (the converter's `-xprintsrc` option).  If the host dies there, the
player cannot be reached again: **reset the board** — that is the only exit.

## What the player does not do

- No TCK setting; the clock is fixed by the firmware.
- No SRST or TRST.
- No open-drain mode; the outputs are 3.3 V push-pull.
- No SVF input; convert first.
- No self-test.

## Related

| Doc | Why |
|---|---|
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the authoritative player protocol and probe |
| [protocol/jtag/tap.md](../../protocol/jtag/tap.md) | TAP states, shifts, IDCODE, bypass |
| [protocol/jtag/entry.md](../../protocol/jtag/entry.md) | both JTAG paths' entry and exit rules |
| [players.md](players.md) | the host-player builds and options |
| [wiring.md](wiring.md) | wires, rails, levels |
| [limits-and-errors.md](limits-and-errors.md) | limits, messages, recovery |
| [workflow.md](workflow.md) | the end-to-end run |
