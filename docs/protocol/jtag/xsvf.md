# JTAG — XSVF player (`0x18`, v4)

Part of the JTAG documentation set.  This page documents the binary bitbang
command `0x18`, the XSVF player firmware sub-mode, which plays a compiled
Xilinx Serial Vector Format (XSVF) file over JTAG to program or configure a
CPLD/FPGA.  Entry and defaults are in [entry.md](entry.md); the TAP is in
[tap.md](tap.md); clocking is in [clock.md](clock.md); pins are in
[wiring.md](wiring.md).

The player is **Bus Pirate v4 only**; the v3 build answers `0x00`.  There is
no terminal XSVF mode.  The host-side program is the **XSVF host player**, a
separate tool set: [tools/xsvf](../../tools/xsvf/README.md).  That set calls
the firmware sub-mode the **firmware player**; it is this page's XSVF
player.  This page owns the in-mode firmware protocol only.

## Entry and exit

1. Enter `BBIO1` with 20 × `0x00`.
2. Send `0x18`.  The board switches on the on-board regulator (it drives the
   `3.3V` and `5.0V` header rails), prints `XSV1`, and enters the player
   command loop.
3. Send a player command (below); `0x03` plays a stream.
4. Send `0x0F` to reset the chain and return to `BBIO1`.  The return is
   **silent**: `BBIO1` is not re-printed, and the pins and regulator are not
   reset.  The TAP is left in Test-Logic-Reset.
5. Send `0x0F` a second time to leave `BBIO1` for the terminal: it replies
   `0x01` and runs the terminal reset (v3: a hardware reset), which switches
   the regulator off.

On v3, `0x18` is answered with a single `0x00` and the bitbang layer stays
active.  Entry and exit rules for both paths: [entry.md](entry.md).

## Player commands

While in the player command loop, each byte selects an action.  Bytes that
are not listed are ignored — including `0x00`; use `0x0F` to leave.

| Byte | Action | Reply |
|---|---|---|
| `0x01` | reset the chain (TMS high for ten TCK clocks), then Run-Test/Idle | — |
| `0x02` | probe the chain (see below) | count byte, then IDCODE bytes |
| `0x03` | play an XSVF stream (see below) | data handshake, then a result byte |
| `0x0F` | reset the chain and leave the player | back to the bitbang loop |
| other | ignored | — |

## Chain probe (`0x02`)

The player resets the chain, holds TDI high, shifts 510 ones into the IR
path (all-ones selects BYPASS), then counts the bypass bits until TDO goes
high.  It then resets and reads 32 bits per device.  This assumes the
chain's total IR length is at most 510 bits; a longer IR chain is not fully
loaded with ones.

The reply is a **one-byte count** equal to four times the number of devices,
followed by that many IDCODE bytes: four per device, least-significant byte
first, the device nearest TDO first.

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
2. The host replies with a **16-bit big-endian** chunk length, then that many
   XSVF bytes.  Keep every chunk at or below **4096 bytes** (see limits).
3. Steps 1–2 repeat until the stream contains the `XCOMPLETE` command, which
   ends the run.
4. The player sends one **result byte** (see below).

The initial TAP state is reset; the player walks the TAP itself according to
the XSVF commands.  The supported command set is the Xilinx XSVF 5.01 set,
including the XC9500/XL compression commands (`XSDRINC`, `XSETSDRMASKS`),
`XSIR2`, `XSTATE`, `XENDIR`/`XENDDR`, `XWAIT` and `XCOMMENT`.

At the end of a run the TAP is left where the file leaves it — the
`XENDIR`/`XENDDR` end state, or the last `XSTATE`; the player does not add
a reset.

**Stalls and busy bytes.**  If the host sends nothing for about **5 seconds**
while the player waits for a chunk, it substitutes `0xFF` for the missing
bytes.  The run ends with `0x01` when the stall is seen at the next command
boundary, `0x04` if the `0xFF` is read as the command byte itself, or
`0x05`/`0x06` if it is read as a TAP-state byte or a shift size.  Separately,
a byte sent while the player is busy in a timed wait (`XWAIT` or `XRUNTEST`)
is consumed and aborts the run with result `0x01` (unknown error).  A
**zero-length chunk** is not data: the player ignores it and requests again.

A stall inside an `XCOMMENT` string exits the comment loop on the stalled
flag.  The run returns `0x01` at the next command boundary, the player loop
is reachable, and `0x0F` leaves the player.

Send nothing except the next chunk while a run is in progress.  Extra bytes
sent while the player is not asking for data are read as the next chunk
length and desync the stream.

## Result codes

The single byte returned at the end of a run:

| Byte | Meaning |
|---|---|
| `0x00` | no error (success) |
| `0x01` | unknown error (also a busy-byte abort or a stall seen at the next command boundary) |
| `0x02` | TDO mismatch — the captured TDO did not match the expected TDO |
| `0x03` | TDO mismatch after the maximum number of retries |
| `0x04` | illegal or unsupported XSVF command (also a stall at a command byte) |
| `0x05` | illegal TAP state in the XSVF stream |
| `0x06` | data overflow — a shift is larger than the player's buffer |

## Worked exchange

A format illustration.  The player requests each chunk with `0xFF`; the host
answers with a big-endian length and the bytes.

```
host -> 0x00 x20          enter binary bitbang
host <- "BBIO1"
host -> 0x18              select the XSVF player
host <- "XSV1"
host -> 0x03              play an XSVF stream
host <- 0xFF              ready for data
host -> 0x10 0x00         chunk length = 0x1000 (4096)
host -> (4096 bytes)      XSVF data
host <- 0xFF              ready for the next chunk
host -> 0x04 0x00         next chunk length = 1024
host -> (1024 bytes)      XSVF data
host <- 0x00              result byte: no error
host -> 0x0F              leave the player (chain reset, no reply)
host -> 0x0F              leave bitbang
host <- 0x01              return-to-terminal ack
host <- "HiZ>"            (version banner omitted)
```

`0x01` is a raw byte, not the text "1".  The v4 terminal reset prints the
version banner before `HiZ>`; it is omitted above so the exchange stays
aligned.

## Host workflow

The XSVF host player and the SVF→XSVF converter are documented in
[tools/xsvf](../../tools/xsvf/README.md) — it owns the players, converter
options and examples.  Both shipped players enter `BBIO1` and `XSV1`
themselves; do not pre-enter the player.  The stock host tools close the
port without sending `0x0F`; they can stop on a result or on an early
error/EOF without one.  After a normal run the board is in the player loop;
a raw-serial host can send `0x0F` twice only once that loop is back.  If the
tool stopped mid-run, reset first.  The exit mapping is in
[tools/xsvf/players.md](../../tools/xsvf/players.md#exit-status-and-the-next-run).
This page owns the in-mode protocol only.

## Limits

- **Shift buffer.**  The player holds the longest single shift in a 50-byte
  buffer, so a stream whose longest DR or IR shift exceeds **400 bits**
  returns the data-overflow result (`0x06`).  This covers XC9500/XL/XV-class
  files; larger CPLD/FPGA configuration streams may not fit.  The limit is
  code-derived and **unverified** on hardware.
- **3.3 V push-pull only.**  The player has no open-drain mode and switches
  VREG on at entry, so a 1.8 V or 2.5 V target cannot use it.  A 5 V target
  may not see 3.3 V as a valid high.  Use the OpenOCD open-drain path with
  pull-ups to the target's own rail.
- **Chunk buffer.**  A chunk larger than 4096 bytes is drained to keep the
  stream aligned but only the first 4096 bytes are kept, corrupting the run.
  Keep chunks ≤ 4096.
- **Chain probe count.**  Clamped to 63 devices (`0xFC`); the count loop
  stops after about 250 devices.  A longer chain is truncated, not wrapped.
- **Stream stall.**  About 5 s without host data during a chunk wait ends the
  run with `0x01`, `0x04`, `0x05` or `0x06`, depending on where the
  substituted `0xFF` lands.  A byte sent during a timed wait aborts the run
  with `0x01`.  The player loop remains reachable after these aborts,
  including a stall inside an `XCOMMENT` string; the run returns `0x01` at
  the next command boundary, and `0x0F` leaves.
- **No clock control.**  The player has no TCK setting.  See
  [clock.md](clock.md).
- **v4 only.**  Not present on v3.  The player reads XSVF, not SVF.
- **Exit leaves the regulator on.**  `0x0F` returns to `BBIO1` without
  resetting the pins or switching VREG off; a raw-serial host can send it
  twice to return to the terminal, or use the reset button.

## Related

- [entry.md](entry.md) — entry, settings and exit.
- [tap.md](tap.md) — TAP states, shifts, IDCODE and BYPASS.
- [clock.md](clock.md) — TCK and link rates.
- [wiring.md](wiring.md) — pins, power and levels; the player needs no
  pull-up and does not use SRST/TRST.
- [openocd.md](openocd.md) — the host-driven `0x06` TAP shifter.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
- [tools/xsvf](../../tools/xsvf/README.md) — the XSVF host player and the
  SVF→XSVF converter.
