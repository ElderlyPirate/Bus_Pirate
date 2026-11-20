# XSVF limits, errors and recovery

Part of the XSVF set.  This page owns what the firmware player and `bp xsvf`
cannot do, what their messages mean, and how to recover.  The byte values and
their printed names are in [player.md](player.md#result-byte); the end-to-end
sequence is in [workflow.md](workflow.md).

## Firmware limits

| Limit | Value | What happens |
|---|---|---|
| longest single DR/IR shift | 400 bits (50 bytes) | over it: `XSVF_ERROR_DATAOVERFLOW` (`0x06`) |
| chunk size | 4096 bytes | a longer chunk is drained; only the first 4096 bytes are used, so the run is corrupt |
| chunk wait | about 5 s | the player substitutes `0xFF` and ends with `0x01`, `0x04`, `0x05` or `0x06` |
| busy byte | any byte during `XWAIT`/`XRUNTEST` | the run ends with `0x01` at the next command boundary |
| zero-length chunk | — | ignored; the player requests again |
| chain probe | 63 devices (`0xFC`) | a longer chain is truncated to the first 63 |
| TCK | fixed by the firmware | no host setting; a target that needs a slower clock cannot use this path |
| v3 | — | `0x18` is answered with `0x00`; no player |

The 400-bit shift, 4096-byte chunk and about 5 s stall behavior are
code-derived; they are **unverified on hardware**.

### The 400-bit shift buffer

The player holds the longest single shift in a 50-byte buffer.  A stream
whose largest `XSDRSIZE`, `XSIR` or `XSIR2` shift is over 400 bits cannot be
played, even if the file is otherwise valid.

| Device class | Largest shift | Fits |
|---|---|---|
| XC9500/XL/XV | about 256 bits | yes |
| CoolRunner/II | about 1035 bits | no |
| FPGA configuration | about 1024 bits | no |
| XC18V00/XCF00S | 4096-bit program, 8192-bit verify | no |

The 256-bit figure is one device; a multi-device chain adds shift length, so
check the largest `XSDRSIZE` in the listing even for XC9500 devices.

Use a smaller device or another host.  OpenOCD's own `svf` command plays the
original SVF (not XSVF) through its `0x06` path
([OpenOCD set](../openocd/README.md)).  The conversion and the shift check are
in [converter.md](converter.md).

### Chunk size

`bp xsvf` sends at most 4096 bytes per chunk; the last chunk is the
remainder.  A custom host must keep every chunk at or below 4096 bytes.  A
longer chunk is drained to keep the stream aligned, but only the first 4096
bytes are kept.  A zero-length chunk is not data: the player ignores it and
requests again, so answer with a non-zero length.

### Stalls, busy bytes and the result order

The player checks its stall flag at the start of each command.  A chunk-wait
stall therefore ends with `0x01` when it is seen at the next command boundary,
or at the end of an `XCOMMENT` string; otherwise the substituted `0xFF`
decides the result.

| Where the stall lands | Result |
|---|---|
| noticed at the next command boundary | `0x01` (`XSVF_ERROR_UNKNOWN`) |
| read as the command byte | `0x04` (`XSVF_ERROR_ILLEGALCMD`) |
| read as a TAP-state byte | `0x05` (`XSVF_ERROR_ILLEGALSTATE`) |
| read as an `XSDRSIZE`/`XSIR2` length over the buffer | `0x06` (`XSVF_ERROR_DATAOVERFLOW`) |
| inside an `XCOMMENT` string | `0x01` at the next command boundary |

You cannot see where the stall landed, so treat any of them as "the host
stopped feeding": fix the host or its timing, then replay after the board is
back at the terminal.  A byte sent while the player is in a timed wait
(`XWAIT` or `XRUNTEST`) is consumed and ends the run with `0x01` at the next
command boundary, unless an earlier error is already latched.

**A stall inside an `XCOMMENT` string.**  The comment loop exits on the
stalled flag.  The run returns `0x01` at the next command boundary, the player
loop is reachable, and the `0x0F` pair leaves; a board reset is not required.
A file made with the converter's `-xprintsrc` option can contain this text.

The player loop is reachable after every abort above, so the escape works
once the run has ended.

### Chain probe

The probe counts bypass bits after shifting 510 ones into the IR path, then
reads 32 bits per device.  The reply count is clamped to 63 devices because
it is one byte (`4 x 63 = 252 = 0xFC`); the count loop stops after about 250
devices.  A longer chain is truncated, not wrapped.

| Symptom | Meaning |
|---|---|
| count `00`, no IDCODE bytes | TDO stuck high, no target power, or an empty chain |
| count `FC`, all-zero bytes | TDO stuck low; the probe hit its 63-device cap |
| count `FC`, plausible IDCODEs | a real chain over 63 devices; only the first 63 are reported |
| plausible count, wrong IDCODEs | the reset instruction is not IDCODE, or the IR chain is over 510 bits |
| count not a multiple of 4, or over 252 | a desynchronised reply; the firmware cannot produce it |

A device that does not implement IDCODE is reported from whatever its DR
shifts out.  The probe is a wiring check, not a chain inventory.  `00` and
`FC` replies exit `0`; an incomplete or malformed reply exits `2`.

## Result actions

The printed names and byte values are in [player.md](player.md#result-byte).
A non-success result prints one line and exits `1`:

```
Error: End of operation reply: XSVF_ERROR_TDOMISMATCH (0x02); the captured TDO did not match the expected TDO
```

| Result | Action | Exit |
|---|---|---|
| `XSVF_ERROR_NONE` | nothing; the stream completed | 0 |
| `XSVF_ERROR_UNKNOWN` | host stall or busy byte; reach the terminal, then replay | 1 |
| `XSVF_ERROR_TDOMISMATCH` | check the file, the wiring, the ground and the wired VCC rail | 1 |
| `XSVF_ERROR_MAXRETRIES` | the same checks; the stream retried and still mismatched | 1 |
| `XSVF_ERROR_ILLEGALCMD` | the file is not XSVF 5.01, or the host stopped feeding | 1 |
| `XSVF_ERROR_ILLEGALSTATE` | regenerate the file | 1 |
| `XSVF_ERROR_DATAOVERFLOW` | use a stream that fits, or another host | 1 |

`XSVF_ERROR_LAST` (`0x07`) is an internal marker and is not expected.  An
unknown result byte is reported as `XSVF_ERROR_UNKNOWN`.

## Host messages

`bp xsvf` prints a host failure as an `Error:` line and exits `2`; a usage
error also prints the usage block.  The exact exception text after a colon
depends on the serial driver.

| Message | What to do |
|---|---|
| `Could not open serial port ...` | close other programs; check the port name and permissions |
| `No serial port specified; pass --port or set 'port' in the configuration file` | add `--port` or a `port` entry |
| `No file specified; pass --file, --scan, or --reset` | add one of the three operations |
| `Invalid value for '--file' / '-f': File '...' does not exist.` | fix the path; the command has not opened the port |
| `Invalid value for '--speed' / '-s': ... is not in the range x>=1.` | use a positive integer speed |
| `XSVF file ... is empty` | use a non-empty XSVF image |
| `Could not read XSVF file ...` | fix the path or permissions |
| `Bus Pirate did not enter XSVF mode; ...` | on v3 expected; on v4 flash the current image, then reset |
| `Expected BBIO1 banner, received ...` | reset; close the terminal; check the port and board state |
| `Expected XSV1 banner, received ...` | the player selection was not accepted; reset and retry |
| `Chain scan: incomplete reply (bounded timeout); no reply after 5 attempts` | check TDO, power and wiring |
| `Chain scan: failed to send the command: ...` | the serial write failed; check the port |
| `Chain scan: device count 0x... is not a multiple of four bytes` | desynchronised reply; reset and retry |
| `Chain scan: device count 0x... exceeds the 252-byte reply limit` | desynchronised reply; reset and retry |
| `Timed out waiting for the XSVF player data request...` | the player did not request data; reset and retry |
| `Timed out waiting for the XSVF player result...` | the player may still run; wait, check, reset |
| `End of file reached without a completion result` | the player wants more data; wait about 5 s, then recover |
| `Leaving XSVF player: missing reset acknowledgement...` | the leave pair did not finish; reset |
| `Leaving XSVF player: Bus Pirate reported a failed binary command` | reset; check the port |
| `Leaving XSVF player: Expected binary success status 0x01, received 0x...` | reset; check the board state |
| `Leaving XSVF player: could not read the terminal banner...` | reopen the terminal and press Enter |
| `Invalid configuration file ...` | fix the YAML file; `--config` must precede `xsvf` |

A failed leave prints no reset, scan, sent or success lines, because the
command reports those only after every step succeeds.  The file may already
have been played; check the target.

## Recovery

| Stop point | Board state | Action |
|---|---|---|
| success without `--leave` | player loop, VREG on | reset, or the raw `0x0F` pair |
| success with `--leave` | terminal | reopen the terminal; press Enter if needed |
| player result error (exit `1`) | player loop | reset, or the raw pair |
| host timeout waiting for a result | possibly still running | wait, check the target, press reset |
| EOF before a completion result | waiting for a chunk | wait about 5 s, then the raw pair or reset |
| v3 or wrong player banner | `BBIO1` | reset, or send `0x0F` from a raw host |
| failed leave | terminal, `BBIO1`, or the player | try the terminal; otherwise reset |

The reset button is the everyday exit and always works.  The raw-serial pair
is for a custom host or a script; it works only once the player is back at its
command loop.  Do **not** send `0x0F` while the player is waiting for a chunk
or busy in a timed wait: it is consumed as data or as a busy byte.

After a reset, the terminal can power the rails again with `m` (any mode)
then `W`; `W` also runs that mode's setup and is refused in `HiZ>`.  The
exact pair procedure is in
[workflow.md](workflow.md#6-leave-the-player-and-recover-the-terminal).

A second command cannot enter the binary layer while the board is still in
the player: the player ignores the host's NUL entry bytes.  Reset first.

## v3

- **v3 has no XSVF player.**  `0x18` is answered with `0x00`, and `bp xsvf`
  exits `2` with the "requires a Bus Pirate v4" message.  The board is left
  in `BBIO1`; press reset (or send `0x0F` from a raw host).  Use OpenOCD's
  `svf` command on v3 ([OpenOCD set](../openocd/README.md)).  The v3 path is
  build-only.

## Related docs

| Doc | Why |
|---|---|
| [player.md](player.md) | the firmware player, handshake and result bytes |
| [players.md](players.md) | options, output and exit status |
| [converter.md](converter.md) | producing the XSVF file and the shift check |
| [workflow.md](workflow.md) | the end-to-end sequence and exit |
| [wiring.md](wiring.md) | wires, rails, levels, reset lines |
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player's byte protocol |
| [protocol/jtag/edge-cases.md](../../protocol/jtag/edge-cases.md) | the JTAG set's failure modes |
