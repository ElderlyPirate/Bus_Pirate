# JTAG — XSVF player (`0x18`, v4)

Part of the JTAG documentation set.  This page documents binary command
`0x18`, the XSVF player firmware sub-mode, which plays a compiled Xilinx
Serial Vector Format (XSVF) file over JTAG to program or configure a
CPLD/FPGA, and the `bp xsvf` host command that drives it.  Entry and exit
are in [entry.md](entry.md); the TAP is in [tap.md](tap.md); clocking is in
[clock.md](clock.md); pins are in [wiring.md](wiring.md).

The player is **Bus Pirate v4 only**; the v3 build answers `0x00`.  There is
no terminal XSVF mode.  The host command is **`bp xsvf`**; installation,
options, conversion and the end-to-end workflow belong to
[tools/xsvf](../../tools/xsvf/README.md).  This page owns the firmware
protocol and the host's role in it.

## Entry and exit

1. Enter `BBIO1` with 20 × `0x00`.  `bp xsvf` writes one leading `0x00`
   plus those twenty; the leading byte leaves most other binary sub-modes.
   It does **not** leave the player, which ignores `0x00`.
2. Send `0x18`.  The board switches VREG on (it drives the `3.3V` and
   `5.0V` header rails), prints `XSV1`, and enters the player command loop.
3. Send a player command; `0x03` plays a stream.
4. Send `0x0F` to reset the chain and return to `BBIO1`.  The return is
   silent: `BBIO1` is not re-printed, and the pins and regulator are not
   reset.  The TAP is left in Test-Logic-Reset.
5. Send `0x0F` again to leave `BBIO1` for the terminal: it replies `0x01`
   and runs the terminal reset, which switches VREG off.

On v3, `0x18` is answered with a single `0x00` and the binary layer stays
active.  The full exit matrix is in [entry.md](entry.md#leaving).

## Player commands

While in the player command loop, each byte selects an action.  Bytes that
are not listed are ignored — including `0x00`; use `0x0F` to leave.

| Byte | Action | Reply |
|---|---|---|
| `0x01` | reset the chain, then Run-Test/Idle | — |
| `0x02` | probe the chain | count byte, then IDCODE bytes |
| `0x03` | play an XSVF stream | data handshake, then a result byte |
| `0x0F` | reset the chain and leave | none; back to `BBIO1` |
| other | ignored | — |

`0x01` holds TMS high for ten TCK clocks, then drives the chain to
Run-Test/Idle.  The player keeps no settings between commands.

## Chain probe (`0x02`)

The player resets the chain, holds TDI high, shifts 510 ones into the IR
path (all-ones selects BYPASS), then counts the bypass bits until TDO goes
high.  It then resets and reads 32 bits per device.  This assumes the
chain's total IR length is at most 510 bits; a longer IR chain is not fully
loaded with ones.

The reply is a **one-byte count** equal to four times the number of devices,
followed by that many IDCODE bytes: four per device, least-significant byte
first, the device nearest TDO first.

```
host -> 0x02
host <- 0x04              count: four bytes, one device
host <- 93 40 60 59       IDCODE 0x59604093, least-significant byte first
```

The count loop stops after about 250 devices; the reply is clamped to 63
devices because the count is one byte (4 × 63 = 252 = `0xFC`).  A longer
chain is truncated, not wrapped.  With TDO stuck low the probe reaches the
cap and reports 63 devices of garbage IDCODEs; with TDO stuck high it
reports zero devices and no IDCODE bytes, which a host can mistake for an
empty chain.

The probe assumes the reset-default instruction is IDCODE on every device.
A device that does not implement IDCODE, or that defaults to another
instruction, is reported from whatever its DR shifts out.  The OpenOCD path
has no equivalent command; the host scans the chain itself.

## Playing a stream (`0x03`)

After `0x03` the player reads the XSVF byte stream in **length-prefixed
chunks**.  It pulls a chunk whenever it needs more data:

1. The player sends the ready byte `0xFF`.
2. The host replies with a **16-bit big-endian** chunk length, then that
   many XSVF bytes.  Keep every chunk at or below **4096 bytes**.
3. Steps 1–2 repeat until the stream contains the `XCOMPLETE` command,
   which ends the run.
4. The player sends one **result byte**.

```
host -> 0x03              start a stream
host <- 0xFF              ready for data
host -> 0x10 0x00         chunk length 4096
host -> (4096 bytes)      XSVF data
host <- 0xFF              ready for the next chunk
host -> 0x04 0x00         chunk length 1024
host -> (1024 bytes)      XSVF data
host <- 0x00              result: no error
```

The initial TAP state is reset; the player walks the TAP itself according
to the XSVF commands.  The supported command set is the Xilinx XSVF 5.01
set, including the XC9500/XL compression commands (`XSDRINC`,
`XSETSDRMASKS`), `XSIR2`, `XREPEAT`, `XSTATE`, `XENDIR`/`XENDDR`, `XWAIT`
and `XCOMMENT`.

At the end of a run the TAP is left where the file leaves it — the
`XENDIR`/`XENDDR` end state, or the last `XSTATE`; the player does not add a
reset.

A **zero-length chunk** is not data: the player ignores it and requests
again.  A chunk larger than 4096 bytes is drained to keep the stream
aligned, but only the first 4096 bytes are kept, so the run is corrupt.

Send nothing except the next chunk while a run is in progress.  Extra bytes
sent while the player is not asking for data are read as the next chunk
length and desync the stream.

## Result codes

The single byte returned at the end of a run.  `bp xsvf` prints the
symbolic name; the mapping is fixed.

| Byte | Printed name | Meaning |
|---|---|---|
| `0x00` | `XSVF_ERROR_NONE` | the stream completed |
| `0x01` | `XSVF_ERROR_UNKNOWN` | unknown error; also a stall or busy byte |
| `0x02` | `XSVF_ERROR_TDOMISMATCH` | captured TDO did not match the file |
| `0x03` | `XSVF_ERROR_MAXRETRIES` | still mismatched after the retries |
| `0x04` | `XSVF_ERROR_ILLEGALCMD` | illegal or unsupported command |
| `0x05` | `XSVF_ERROR_ILLEGALSTATE` | illegal TAP state in the stream |
| `0x06` | `XSVF_ERROR_DATAOVERFLOW` | a shift is larger than the buffer |
| `0x07` | `XSVF_ERROR_LAST` | internal end-of-list marker; not returned normally |

`0xFF` is the ready byte, not a result.  A host that receives an unknown
byte should report `XSVF_ERROR_UNKNOWN`, as `bp xsvf` does.

## Stalls and busy bytes

If the host sends nothing for about **5 seconds** while the player waits for
a chunk, it substitutes `0xFF` for the missing bytes.  The run ends with
`0x01` when the stall is seen at the next command boundary, `0x04` if the
`0xFF` is read as the command byte itself, or `0x05`/`0x06` if it is read
as a TAP-state byte or a shift size.

| Where the stall lands | Result |
|---|---|
| noticed at the next command boundary | `0x01` (`XSVF_ERROR_UNKNOWN`) |
| read as the command byte | `0x04` (`XSVF_ERROR_ILLEGALCMD`) |
| read as a TAP-state byte | `0x05` (`XSVF_ERROR_ILLEGALSTATE`) |
| read as a shift size over the buffer | `0x06` (`XSVF_ERROR_DATAOVERFLOW`) |
| inside an `XCOMMENT` string | `0x01` at the next command boundary |

A byte sent while the player is busy in a timed wait (`XWAIT` or
`XRUNTEST`) is consumed and aborts the run with `0x01` at the next command
boundary.  A stall inside an `XCOMMENT` string exits the comment loop on the
stalled flag, so the run returns `0x01` and the player loop stays
reachable; `0x0F` leaves.

## Worked sessions

The host command enters `BBIO1` and `XSV1` itself; do not pre-enter either.
A scan needs no file and leaves the chain untouched:

```
$ bp xsvf -p COM5 --scan --leave
Opening Bus Pirate on COM5 at 115200 bps.
Chain Scan Result: 04 93 40 60 59
Returned to the Bus Pirate terminal.
Thank you for playing! :-)
```

A file run:

```
$ bp xsvf -p COM5 -f design.xsvf -r --leave
File is 53289 bytes.
Opening Bus Pirate on COM5 at 115200 bps.
Performing Reset.. Done
Sent 53289 bytes in 14 chunk(s).
End of operation reply: XSVF_ERROR_NONE
Success!
Returned to the Bus Pirate terminal.
Thank you for playing! :-)
```

The success check is `XSVF_ERROR_NONE` followed by `Success!`; the last
line alone only says the host stopped.  Without `--leave` the board stays in
the player with VREG on.  `--leave` sends the `0x0F` pair only after every
requested operation succeeds, and cannot rescue a board left in the player
by an earlier command.  See [tools/xsvf/players.md](../../tools/xsvf/players.md).

## Limits

| Limit | Value | What happens |
|---|---|---|
| Shift buffer | 400 bits longest DR/IR | data overflow (`0x06`); single-device XC9500/XL/XV fits |
| Output | 3.3 V push-pull only | no open-drain; a 1.8 V or 2.5 V target cannot use it, and a 5 V target may not see a valid high |
| Chunk | 4096 bytes max | a larger chunk is drained and truncated |
| Chain probe | 63 devices (`0xFC`) | the count loop stops after about 250 devices |
| Stream stall | about 5 s | returns `0x01`, `0x04`, `0x05` or `0x06`; a byte during a timed wait returns `0x01` |
| Clock | fixed | no host TCK setting ([clock.md](clock.md)) |
| Reset lines | SRST/TRST not driven | release leftover levels before a run |
| Board | v4 only | `0x18` answers `0x00` on v3; the player reads XSVF, not SVF |
| Exit | VREG stays on | `0x0F` returns to `BBIO1`; send it twice or press reset |

## Related

- [entry.md](entry.md) — entry, settings and exit.
- [tap.md](tap.md) — TAP states, shifts, IDCODE and BYPASS.
- [clock.md](clock.md) — TCK and link rates.
- [wiring.md](wiring.md) — pins, power, levels; no pull-up needed.
- [openocd.md](openocd.md) — the host-driven `0x06` TAP shifter.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
- [tools/xsvf](../../tools/xsvf/README.md) — the host command and conversion.
