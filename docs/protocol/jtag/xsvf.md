# JTAG — XSVF player (`0x18`, v4)

Part of the JTAG documentation set.  This page documents the binary bitbang
command `0x18`, the XSVF player **firmware sub-mode**, which plays a compiled
Xilinx XSVF file (Xilinx Serial Vector Format, the compressed binary form of
SVF) over JTAG to program or configure a CPLD/FPGA.  The pin map is in
[wiring.md](wiring.md); the binary-layer basics are in
[binary/README.md](../binary/README.md).

The XSVF player is **Bus Pirate v4 only** — it is not built into the v3 image.
There is no terminal XSVF mode in this firmware; the player lives entirely on
this binary path.  The host-side program is a separate tool, the **XSVF host
player** — see [tools/xsvf](../../tools/xsvf/README.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x18`.  The Bus Pirate switches on the on-board voltage regulator,
   prints `XSV1`, and enters the player command loop.  The regulator feeds the
   VCC pin — see [wiring.md](wiring.md) §Levels and voltage before powering
   the target externally.
3. Send a player command (below); `0x03` plays a stream.
4. Send `0x0F` to reset the chain and leave the player, back to the bitbang
   loop.  This return is **silent**: `BBIO1` is not re-printed, and the pins
   and regulator are **not** reset.
5. Send `0x0F` a second time to leave the bitbang layer for the terminal: it
   replies `0x01` and runs the terminal reset (regulator off, pins inputs).
   The reset button also leaves.

On v3, `0x18` is answered with a single `0x00` failure byte instead of
`XSV1`, and the mode stays in bitbang.  The exit rules for both JTAG paths are
summarised in [README.md](README.md) §Leaving the modes.

## Player commands

While in the player command loop, each byte selects an action.  Bytes that are
not listed are ignored — including `0x00`; use `0x0F` to leave.

| Byte | Action | Reply |
|---|---|---|
| `0x01` | reset the JTAG chain (TMS held high for ten TCK clocks) | — |
| `0x02` | probe the chain (see below) | count byte, then IDCODE bytes |
| `0x03` | play an XSVF stream (see below) | data handshake, then a result byte |
| `0x0F` | reset the chain and leave the player | back to the bitbang loop |
| other | ignored | — |

### Chain probe (`0x02`)

The player resets the chain, scans the instruction register, then counts the
devices on the chain and reads their IDCODEs.  The reply is a **one-byte
count** equal to four times the number of devices, followed by that many
IDCODE bytes (four per device, in chain order).

The device-count read loop stops after about 250 devices.  The reply is then
**clamped to 63 devices** because the count is one byte (4 × 63 = 252 =
`0xFC`).  A longer chain is truncated, not wrapped: the first 63 devices are
reported.  With TDO stuck low (unpowered or miswired target) the probe reaches
that cap and reports 63 devices of garbage IDCODEs, which a host can mistake
for a real chain.

### Playing a stream (`0x03`)

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

**Stalls and busy bytes.**  If the host sends nothing for about **5 seconds**
while the player waits for a chunk, the forced `0xFF` is read as the next
command: the run aborts with result `0x04` (illegal command) and the player
loop stays reachable.  Separately, a byte sent while the player is busy in a
timed wait (`XWAIT` or `XRUNTEST`) is consumed and aborts the run with result
`0x01` (unknown error).  A **zero-length chunk** is not data: the player
ignores it and requests again.  Send nothing except the next chunk while a run
is in progress.

### Result codes

The single byte returned at the end of a run:

| Byte | Meaning |
|---|---|
| `0x00` | no error (success) |
| `0x01` | unknown error (also the timed-wait busy-byte abort) |
| `0x02` | TDO mismatch — the captured TDO did not match the expected TDO |
| `0x03` | TDO mismatch after the maximum number of retries |
| `0x04` | illegal or unsupported XSVF command (also a chunk-wait stall) |
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
host -> ...              4096 XSVF bytes
host <- 0xFF              ready for the next chunk
host -> 0x04 0x00         next chunk length = 1024
host -> ...              that many bytes
host <- 0x00              result byte: no error
host -> 0x0F              leave the player (chain reset, no reply)
host -> 0x0F              leave bitbang
host <- 0x01              return-to-terminal ack
host <- "HiZ>"            (banner omitted here)
```

The v4 terminal reset prints the version banner before `HiZ>`; it is omitted
above so the exchange stays aligned.

## Host workflow

The XSVF host player and the SVF→XSVF converter are documented in
[tools/xsvf](../../tools/xsvf/README.md) — it owns the players, converter
options and examples.  Both shipped players enter `BBIO1` and `XSV1`
themselves; do not pre-enter the player.  This page owns the in-mode protocol
only.

## Limits

- **Shift buffer.**  The player holds the longest single shift in a 50-byte
  buffer, so a stream whose longest DR or IR shift exceeds **400 bits** returns
  the data-overflow result (`0x06`).  This covers XC9500/XL/XV-class files;
  larger CPLD/FPGA configuration streams may not fit.  **Unverified** on
  hardware.
- **Chunk buffer.**  A chunk larger than 4096 bytes is drained to keep the
  stream aligned but only the first 4096 bytes are kept, corrupting the run.
  Keep chunks ≤ 4096.
- **Chain probe count.**  Clamped to 63 devices (`0xFC`); the count loop stops
  after about 250 devices.  A longer chain is truncated, not wrapped.
- **Stream stall.**  About 5 s without host data during a chunk wait aborts
  the run with result `0x04` (the forced `0xFF` is read as an illegal
  command); a byte sent during a timed wait aborts it with `0x01`.  The player
  loop remains reachable, so `0x0F` still leaves.
- **v4 only.**  Not present on v3.

## Related

- [wiring.md](wiring.md) — pins, levels; the player needs no pull-up, does not
  use SRST/TRST, and its entry enables the regulator.
- [openocd.md](openocd.md) — the host-driven `0x06` TAP shifter.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
- [tools/xsvf](../../tools/xsvf/README.md) — the XSVF host player and the
  SVF→XSVF converter.
