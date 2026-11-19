# XSVF limits, errors and recovery

Part of the XSVF host-tool set.  This page owns what the firmware player and
the host players cannot do, what their messages mean, and how to recover.
The byte values and their printed names are in
[player.md](player.md#result-byte); the end-to-end sequence is in
[workflow.md](workflow.md).

## Scope

Firmware limits (shift buffer, chunk buffer, stall, chain count), the result
actions, the host players' messages and quirks, and the recovery steps.  The
in-mode protocol itself is in
[protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md).

## Firmware limits

| Limit | Value | What happens |
|---|---|---|
| longest single DR/IR shift | 400 bits (50 bytes) | over it → `XSVF_ERROR_DATAOVERFLOW` (`0x06`) |
| chunk size | 4096 bytes | a longer chunk is drained; only the first 4096 bytes are used, so the run is corrupt |
| chunk wait | about 5 s | the player substitutes `0xFF` and the run ends with `0x01`, `0x04`, `0x05` or `0x06` |
| busy byte | any byte during `XWAIT`/`XRUNTEST` | the run ends with `0x01` at the next command boundary |
| zero-length chunk | — | ignored; the player requests again |
| chain probe | 63 devices (`0xFC`) | longer chains are truncated to the first 63 |
| TCK | fixed by the firmware | no host setting; a target that needs a slower clock cannot use this path |
| v3 | — | `0x18` is answered with `0x00`; no player |

### The 400-bit shift buffer

The player holds the longest single shift in a 50-byte buffer.  A stream
whose largest `XSDRSIZE`, `XSIR` or `XSIR2` shift is over 400 bits cannot be
played, even if the file is otherwise valid.

| Device class | Largest shift | Fits |
|---|---|---|
| XC9500/XL/XV | about 256 bits | yes |
| CoolRunner/II | about 1035 bits | no |
| FPGA configuration | about 1024 bits | no |
| XC18V00/XCF00S | about 8192 bits | no |

Use a smaller device or another host.  OpenOCD's own `svf` command can play
the original SVF (not XSVF) through its `0x06` path
([OpenOCD set](../openocd/README.md)).  The limit is code-derived and
**unverified** on hardware.

### Chunk size

The host players send up to 4096 bytes per chunk; the last chunk is the
remainder.  A custom host must keep every chunk at or below 4096 bytes; a
longer one is drained to keep the stream aligned but only the first 4096
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
| inside an `XCOMMENT` string | `0x01` at the next command boundary (the comment loop exits on the stalled flag) |

You cannot see where the stall landed, so treat any of them as "the host
stopped feeding": fix the host or its timing, then replay.  A byte sent while
the player is in a timed wait (`XWAIT` or `XRUNTEST`) is consumed and ends the
run with `0x01` at the next command boundary, unless an earlier error is
already latched.

**A stall inside an `XCOMMENT` string.**  The comment loop exits on the
stalled flag.  The run returns `0x01` at the next command boundary, the player
loop is reachable, and `0x0F` leaves; a board reset is not required.  A file
made with the converter's `-xprintsrc` option can contain this text.

The player loop is reachable after every abort above, so `0x0F` still leaves
once the run has ended.

### Chain probe

The probe counts bypass bits after shifting 510 ones into the IR path, then
reads 32 bits per device.  The reply count is clamped to 63 devices because
it is one byte (`4 × 63 = 252 = 0xFC`); the count loop stops after about 250
devices.  A longer chain is truncated, not wrapped.

| Symptom | Meaning |
|---|---|
| count `00`, no IDCODE bytes | TDO stuck high, no target power, or an empty chain |
| count `FC`, all-zero bytes | TDO stuck low; the probe hit its 63-device cap |
| count `FC`, plausible IDCODEs | a real chain over 63 devices; only the first 63 are reported |
| plausible count, wrong IDCODEs | the reset instruction is not IDCODE, or the IR chain is over 510 bits |

A device that does not implement IDCODE is reported from whatever its DR
shifts out.  The probe is a wiring check, not a chain inventory.

## Result actions

The printed names and their byte values are in
[player.md](player.md#result-byte).

| Result | Action |
|---|---|
| `XSVF_ERROR_NONE` | nothing; the stream completed |
| `XSVF_ERROR_UNKNOWN` | host stopped feeding, sent a byte during a wait, or stalled inside an `XCOMMENT`; replay |
| `XSVF_ERROR_TDOMISMATCH` | wrong file, wiring, ground, or the target's VCC rail |
| `XSVF_ERROR_MAXRETRIES` | the same, with a retrying stream; check the chain again |
| `XSVF_ERROR_ILLEGALCMD` | the file is not XSVF 5.01, or the host stopped feeding |
| `XSVF_ERROR_ILLEGALSTATE` | regenerate the file |
| `XSVF_ERROR_DATAOVERFLOW` | a smaller device, another host, or OpenOCD's `svf` |

If the host prints `XSVF_READY_FOR_DATA` with
`End of file reached without a completion result.`, no result byte was read
and the run returns `2`; use that message's row below.

## Host messages and states

The messages and statuses below are for the current source builds.  The
tracked legacy prebuilt EXEs differ
([players.md](players.md#sources-vs-prebuilt-binaries)).

| Message or state | What to do |
|---|---|
| `Error opening serial port` | close other programs; check the port name |
| `No serial port specified` | add `-p` |
| `No file specified. Need an input xsvf file` | add `-f`, `-x` or `-r`; only a run with none exits `2`; reset or raw-serial `0x0F` twice to leave |
| `Error opening file` | fix the `-f` path; both source builds return `2` before the player start; reset or raw-serial `0x0F` twice |
| `Error sizing nonempty input file` | empty or unsizable `-f` file; both source builds return `2` before the player start; reset or raw-serial `0x0F` twice |
| `Unable to activate ... binary mode` | reset; close the terminal; check the port and board state |
| `Unable to activate ... XSVF player mode` | v4 only; board stays in the binary layer — press reset (or raw-serial `0x0F`) |
| `Buspirate did not respond correctly` / `Too many tries in serial read!` | reset; check the port and the board state |
| `Chain scan: incomplete reply (bounded timeout)` | count or payload did not arrive within five attempts; returns `2`; check TDO, power and wiring |
| `Chain scan: failed to send the command` | the `0x02` write failed; the run returns `2`; check the port |
| `Invalid argument` | use the listed options; run with no arguments for the help |
| `End of file reached without a completion result.` | run returns `2`; the player waits for a chunk: wait ~5 s, then raw-serial `0x0F` twice; or reset |
| `No reply.... Quitting.` | the player gave no result (it may still be running); the run returns `2`: do not replay; wait and check, then press reset |
| long `XRUNTEST` / `XWAIT` | wait and check, then reset before replaying; a raw-serial `0x0F` during the wait aborts the run |
| tool exited normally | board stays in the player with regulator on — press reset, or raw-serial `0x0F` twice |
| a second run cannot enter binary | the player ignores the host's NUL entry bytes; reset first |
| chain-scan line looks short | legacy prebuilt only; source builds read the count and payload; run `-x` alone then reset, or rebuild |
| `-s` | host port speed only; it does not change the Bus Pirate's rate |
| target powered externally | remove the external supply; entry switches VREG on |
| chain held in reset (SRST/TRST) | player does not drive them; reset, then `c`+`A` (AUX) or `k`+`A` (AUX1) |
| target on the wrong rail | VCC goes to one rail only, `3.3V` or `5.0V`, never both |

The exact "Unable to activate" wording differs between the builds; the
meaning is the same.  The tracked legacy prebuilt EXEs print
` Got no reply for a Chain scan` instead of the two `Chain scan:` messages.

| Build | Entry failure | Player-mode failure |
|---|---|---|
| Windows | ` Unable to activate BP4 Binary mode` | ` Unable to activate XSVF player mode` |
| cross-platform | ` Unable to activate the BP binary mode` | ` Unable to activate the XSVF player mode` |

## v3 and unverified

- **v3 has no XSVF player.**  The `0x18` byte is answered with `0x00`, and
  both source builds stop with an "Unable to activate … XSVF player mode"
  message and return `2`.  The board is left in the binary layer; press reset
  (or send `0x0F` from a raw-serial host) to get the terminal back.  Use
  OpenOCD's `svf` command on v3 ([OpenOCD set](../openocd/README.md)).
- The 400-bit shift, the 4096-byte chunk behaviour and the stall results are
  code-derived and **unverified** on hardware.

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
