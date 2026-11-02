# JTAG — XSVF player (`0x18`, v4)

Part of the JTAG documentation set (`docs/jtag/`).  This page documents the
binary bitbang command `0x18`, which plays a compiled Xilinx XSVF file
(Xilinx Serial Vector Format, the compressed binary form of SVF) over JTAG to
program or configure a CPLD/FPGA.  The pin map is in [wiring.md](wiring.md);
the binary-layer basics are in [binary/README.md](../binary/README.md).

The XSVF player is **Bus Pirate v4 only** — it is not built into the v3 image.
There is no terminal XSVF mode in this firmware; the player lives entirely on
this binary path.

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x18`.  The Bus Pirate switches on the on-board voltage regulator,
   prints `XSV1`, and enters the player command loop.
3. Send a player command (below); `0x03` plays a stream.
4. Send `0x0F` to reset the chain and leave the player, back to the bitbang
   loop.  The bitbang identifier is not re-printed on this return, so send
   `0x0F` a second time to leave the bitbang layer for the terminal (or press
   the reset button).

On v3, `0x18` is answered with a single `0x00` failure byte instead of
`XSV1`, and the mode stays in bitbang.

## Player commands

While in the player command loop, each byte selects an action.  Bytes that are
not listed are ignored.

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
IDCODE bytes (four per device, in chain order).  The device count is capped at
250; because the count is a single byte, a chain of more than 63 devices wraps
the count field.

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

### Result codes

The single byte returned at the end of a run:

| Byte | Meaning |
|---|---|
| `0x00` | no error (success) |
| `0x01` | unknown error |
| `0x02` | TDO mismatch — the captured TDO did not match the expected TDO |
| `0x03` | TDO mismatch after the maximum number of retries |
| `0x04` | illegal or unsupported XSVF command |
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
host -> 0x0F              leave the player (chain reset)
host -> 0x0F              leave bitbang
host <- "HiZ>"
```

## The host workflow

XSVF files are produced from SVF with a converter.  The project ships a
converter, `svf2xsvf502`:

```
svf2xsvf502 -i design.svf -o design.xsvf
```

Two host players are shipped, and they differ in how they enter the player:

| Player | Entry behaviour |
|---|---|
| Windows build (`BP4_Full_bitbang_XSVFplayer_Win`) | opens the port, runs the binary entry (20 × `0x00` → `BBIO1`), sends `0x18`, waits for `XSV1`, then sends `0x03` and streams the file |
| cross-platform build (`BPXSVFplayer`) | opens the port and sends `0x03` directly — it does **not** enter the binary layer or send `0x18`, so the `XSV1` player loop must already be active (see the Entry steps above) |

Both take the same options: `-p <port>`, `-f <file>.xsvf`, `-s <speed>`,
plus `-x` (chain scan first) and `-r` (reset the chain first).  For example:

```
BP4_Full_bitbang_XSVFplayer_Win -p COM1 -f design.xsvf -s 115200 -x -r
BPXSVFplayer -p COM1 -s 115200 -f design.xsvf
```

Both stream the file chunk by chunk (answering each `0xFF`) and report the
result byte.

## Limits

- **Shift buffer.**  The player holds the longest single shift in a 50-byte
  buffer, so a stream whose longest DR or IR shift exceeds **400 bits** returns
  the data-overflow result (`0x06`).  This covers XC9500/XL/XV-class files;
  larger CPLD/FPGA configuration streams may not fit.  **Unverified** on
  hardware.
- **Chunk buffer.**  A chunk larger than 4096 bytes is drained to keep the
  stream aligned but only the first 4096 bytes are kept, corrupting the run.
  Keep chunks ≤ 4096.
- **Chain probe count.**  A single one-byte count; more than 63 devices wraps
  the field.
- **No timeout.**  The player blocks until the host answers `0xFF`; a host that
  stops mid-run leaves the port in the player until a reset.
- **v4 only.**  Not present on v3.

## Related

- [wiring.md](wiring.md) — pins, levels; the player needs no pull-up and does
  not use SRST/TRST.
- [openocd.md](openocd.md) — the host-driven `0x06` TAP shifter.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.