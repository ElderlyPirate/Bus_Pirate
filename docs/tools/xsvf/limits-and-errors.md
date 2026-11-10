# XSVF limits, errors and recovery

Part of the XSVF host-tool set (`docs/tools/xsvf/`).  This page owns what the
firmware player and the host tools cannot do, what their messages mean, and
how to recover.  The in-mode protocol is defined in
[protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md); this page is the
host-side reading of it.

Naming: the **firmware player** is the Bus Pirate's `0x18` sub-mode; the
**host tools** are the two PC programs that stream the file.

## Scope

Firmware limits (shift buffer, chunk buffer, stall, chain count), the result
messages the host tools print, tool-side quirks, and the recovery steps.  It
does not repeat the byte-level command tables; follow the link for those.

## Firmware limits

| Limit | Value | What happens |
|---|---|---|
| longest single DR/IR shift | 400 bits (50 bytes) | over it → `0x06` (`XSVF_ERROR_DATAOVERFLOW`) |
| chunk size | 4096 bytes | larger is drained; only the first 4096 bytes are kept, so the run is corrupt |
| chunk wait | about 5 s | the run aborts; see the stall rows below |
| busy byte | any byte during `XWAIT`/`XRUNTEST` | the run aborts with `0x01` |
| zero-length chunk | — | ignored; the firmware player requests again |
| chain probe | 63 devices (`0xFC`) | longer chains are truncated to the first 63; the probe stops after about 250 |
| v3 | — | `0x18` is answered with `0x00`; no player |

### The 400-bit shift buffer

The firmware player holds the longest single shift in a 50-byte buffer.  A
stream whose largest `XSDRSIZE` (or `XSIR`/`XSIR2` length) is over 400 bits
cannot be played, even if the file is otherwise valid.  This covers
XC9500/XL/XV-class streams (largest shift about 256 bits); CoolRunner/II
(about 1035 bits), FPGA configuration streams (about 1024 bits) and
XC18V00/XCF00S (about 8192 bits) do not fit.  Use a smaller device or another
host player; OpenOCD's own `svf` command can play the original SVF through
its `0x06` path ([OpenOCD set](../openocd/README.md)), but the Bus Pirate
XSVF player cannot read SVF.

The limit is code-derived and **unverified** on hardware.

### Chunk stalls and busy bytes

The firmware player requests each chunk with `0xFF` and waits about 5 s for
the host.  If nothing arrives, it substitutes `0xFF` for the missing input.
A chunk-wait stall normally ends with `0x04`; if it lands inside a command the
result can be `0x01` (or `0x06` if a length was read as `0xFF`):

| Firmware-side stall location | Result | Tool message |
|---|---|---|
| waiting for a chunk | `0x04` | `Unknown XSVF command: XSVF_ERROR_ILLEGALCMD` |
| inside a multi-byte command | `0x01` | `Unknown error: XSVF_ERROR_UNKNOWN` |
| a length read as `0xFF` | `0x06` | `Error, data overflow: XSVF_ERROR_DATAOVERFLOW` |

All three mean the host stopped feeding the stream: fix the host or its
timing, then replay.  A user cannot see where the stall landed, so treat any
of them as this case after a host-side stop.

A byte sent while the firmware player is busy in `XWAIT` or `XRUNTEST` is
consumed and aborts the run with `0x01`.  Send nothing except the next chunk
while a run is in progress.  The firmware player loop stays reachable after
an abort, so `0x0F` still leaves — once the player is back in its command
loop.

### Chunk size

The host tools send 4096-byte chunks, exactly the firmware buffer.  A custom
host must keep every chunk at or below 4096 bytes; a longer one is drained to
keep the stream aligned but only the first 4096 bytes are used, corrupting
the run.  A zero-length chunk is not data: the firmware player ignores it and
asks again, so answer with a non-zero length.

## Result messages

The result byte values are defined in the
[JTAG page](../../protocol/jtag/xsvf.md#result-codes).  The host tools print
these names:

| Message | Byte | Meaning |
|---|---|---|
| `XSVF_ERROR_NONE` + `Success!` | `0x00` | the stream completed |
| `XSVF_ERROR_UNKNOWN` | `0x01` | unknown error, or a busy/stalled byte |
| `XSVF_ERROR_TDOMISMATCH` | `0x02` | TDO did not match the file |
| `XSVF_ERROR_MAXRETRIES` | `0x03` | TDO still mismatched after the retries |
| `XSVF_ERROR_ILLEGALCMD` | `0x04` | illegal/unsupported command, or a chunk-wait stall |
| `XSVF_ERROR_ILLEGALSTATE` | `0x05` | illegal TAP state in the stream |
| `XSVF_ERROR_DATAOVERFLOW` | `0x06` | a shift is longer than the 400-bit buffer |
| `XSVF_ERROR_LAST` | `0x07` | enum sentinel; not returned |
| `XSVF_READY_FOR_DATA` | `0xFF` | the ready byte; not a result |

`0x07` is the end-of-enum marker (`XSVF_ERROR_LAST`), not a result the
firmware player returns.  `0xFF` is the ready byte the firmware player sends
when it wants the next chunk; the host tool labels the read after the file as
`End of operation reply`.

What to check, by result:

| Result | Action |
|---|---|
| `0x00` | nothing; the stream completed |
| `0x01` | send only chunks; check the stream and the host timing |
| `0x02` / `0x03` | wrong chain, wiring, ground, or the target's VCC |
| `0x04` | file not XSVF 5.01, or the host stopped feeding |
| `0x05` | regenerate the file |
| `0x06` | a smaller device, another host player, or OpenOCD's `svf` |

If the host tool prints `XSVF_READY_FOR_DATA` with `End of file reached.`, no
result byte was read; use the `End of file reached.` row below.

## Messages, states and recovery

One table for the host tools' messages and states.  A result byte is covered
by the table above; this one is about the run around it.

| Message or state | What to do |
|---|---|
| `Error opening serial port` | close other programs; check the port name |
| `No serial port specified` | add `-p` |
| `No file specified. Need an input xsvf file` | add `-f`; the help calls it optional, but it is required |
| `Error opening file` | fix the `-f` path |
| `Unable to activate ... binary mode` | reset; close the terminal; check the port |
| `Unable to activate ... XSVF player mode` | v4 only; the board stays in the binary layer — send `0x0F` or reset |
| `End of file reached.` | the firmware player waits for a chunk: press reset, or wait for the ~5 s stall, then `0x0F` twice |
| `No reply.... Quitting.` | the firmware player may still be running: do not replay; wait/check, then press reset |
| long `XRUNTEST` / `XWAIT` | wait/check, then reset before replaying; `0x0F` during the wait aborts the run |
| `Got no reply for a Chain scan` | check TDO, power and wiring; the scan is optional; portable `-x` retries forever on Linux |
| `Buspirate did not respond correctly` / `Too many tries in serial read!` | reset; check the port and the board state |
| `Invalid argument` | use the listed options; run with no arguments for help |
| tool exited normally | host closed the port without `0x0F`; the board stays in the player with VREG on — press reset |
| a second run cannot enter binary | the firmware player recognises `0x00` only at its command loop; leave it first (reset) |
| chain-scan line looks short | the read is not sized by the count byte; the line is informational |
| `-s` | host port speed only; keep 115200 — it does not change the Bus Pirate's baud |
| target powered externally | remove the external supply; the firmware player feeds VCC |
| chain held in reset (SRST/TRST) | the firmware player does not drive them; release them externally |

The exact wording of the "Unable to activate" lines differs between the two
builds (`BP4 Binary mode` / `XSVF player mode` on Windows, `the BP binary
mode` / `the XSVF player mode` on the portable build); the meaning is the
same.

## v3 and unverified

- **v3 has no XSVF player.**  The `0x18` byte is answered with `0x00`, and
  both host tools stop with an "Unable to activate … XSVF player mode"
  message.  On v3, use OpenOCD's own `svf` command through the `0x06` path
  ([OpenOCD set](../openocd/README.md)).
- The 400-bit shift and 4096-byte chunk behaviour are **unverified** on
  hardware.
- The chain-probe truncation at 63 devices is code-derived; a TDO stuck low
  also produces 63 devices of garbage, which can look like a real chain.

## Related docs

| Doc | Why |
|---|---|
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player, stream framing, result codes |
| [protocol/jtag/edge-cases.md](../../protocol/jtag/edge-cases.md) | the JTAG set's failure modes |
| [players.md](players.md) | player options and output |
| [converter.md](converter.md) | the 400-bit shift check |
| [workflow.md](workflow.md) | the end-to-end sequence and exit |
| [../openocd/README.md](../openocd/README.md) | the host-driven TAP alternative |
