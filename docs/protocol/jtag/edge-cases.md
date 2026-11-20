# JTAG — edge cases and failure modes

Part of the JTAG documentation set.  Each entry gives a trigger, the
observable symptom, and the recovery.  The full exit matrix is in
[entry.md](entry.md#leaving); this page covers the failures around it.

## Entry and mode

| Trigger | Symptom | Recovery |
|---|---|---|
| Looking for a JTAG menu entry | no menu entry | binary only: 20 × `0x00`, then `0x06` or `0x18` |
| `0x18` on a v3 build | `0x00`, not `XSV1` | XSVF is v4 only; use `0x06` on v3 |
| JTAG bytes at the terminal prompt | terminal errors or silence | no terminal commands; use a host |
| A non-NUL byte during entry | no `BBIO1` | send 20 consecutive `0x00` bytes |
| NULs sent inside a sub-mode | most sub-modes exit on `0x00` | leave it first, or prefix one NUL |
| Stuck in the XSVF player | no `BBIO1` on the NUL entry | player ignores `0x00`; press reset |
| Extra NULs after `BBIO1` | `BBIO1` printed again | drain the banners; not an error |

## Wiring and electrical

| Trigger | Symptom | Recovery |
|---|---|---|
| Open-drain, no pull-ups | lines never go high | fit 2–10 kΩ on TDI/TMS/TCK |
| Open-drain, SRST/TRST wired | resets never release | fit pull-ups on both lines too |
| OpenOCD entry, no pin mode | no clock or data | pins are high-Z; send `0x01 0x01` first |
| Push-pull into a lower-voltage chain | contention, damage risk | OpenOCD: open-drain + pull-ups |
| 3.3 V push-pull into a 5 V target | marginal high level | check the target's input threshold |
| TDI/TDO or TCK/TMS swapped | TDO mismatch, no IDCODE | check the signal map and the ground |
| Target not powered | TDO low or floating | power the target, or enable VREG if the board supplies it |
| TRST held low | test logic in reset; all-zero reads | release with action `1` |
| SRST held low | CPU held in reset | release with action `1` |
| Pin mode set while resets are wired | both resets asserted | release both after the pin mode |
| Target powered externally during XSVF | regulator back-driven | disconnect the target supply |
| Vpullup has no voltage | on-board pull-ups do nothing | feed Vpullup externally |
| Applied voltage above the limit | damage risk | 5 V safe, 5.5 V maximum |

## OpenOCD path

| Trigger | Symptom | Recovery |
|---|---|---|
| Echo not read | payload and TDO misaligned | account for the 3-byte echo before the TDO |
| Count over 8192 | 8192 bits shift; surplus becomes commands | split into shorter shifts |
| Clamped shift, full payload | short reply; next command misread | keep requests under the limit |
| Zero-bit shift | echo only; no payload read, no TDO | send no payload and read no TDO |
| `0x07` fast, bad confirm | reply `0x07 0x00` (two bytes) | resend `0xAA 0x55` |
| `0x07` fast on v4 | reply says fast; rate unchanged | set the host CDC rate yourself |
| `0x08` sent before a shift | no reply | do not wait; the next byte is a command |
| `0x0F` inside `OCD1` | single `0x00`; mode stays | send `0x00`, then `0x0F` |
| Unknown command byte | single `0x00` | check the command table |
| Expecting JTAG-over-serial | custom `OCD1` framing | use a compatible host |
| Leaving with `0x00` | target TAP keeps its state | reset the chain before leaving |
| `0x00` with TRST wired | TRST keeps its last level | send `0x01 0x00`, or finish the exit |

## TAP and scan

| Trigger | Symptom | Recovery |
|---|---|---|
| Wrong IR length | wrong instruction reaches a device | check the chain order and IR lengths |
| Wrong DR length | shifted IDCODE or register data | use 32 bits per IDCODE device |
| TDO stuck low | all-zero reads; probe reports 63 devices | check power, TDO and ground |
| TDO stuck high | all-one reads; probe reports zero devices | check TDO, ground and the target |
| TDO floating | random data | check power and ground; add a target-side pull |
| Chain longer than 63 on `0x02` | count clamped at `0xFC` | probe a shorter chain |
| Device without IDCODE | wrong count or IDCODE data | use an explicit scan |
| Chain IR longer than 510 bits | probe data wrong | use an explicit scan |
| TCK too fast for the target | intermittent mismatches | larger `0x08` delay from a custom host |
| Missing ground | intermittent or no data | connect a common ground |

## XSVF player

| Trigger | Symptom | Recovery |
|---|---|---|
| Longest shift over 400 bits | result `0x06` data overflow | use a smaller file or another host |
| Wrong file or bad wiring | result `0x02` or `0x03` mismatch | check file, wiring, ground, power |
| Chunk larger than 4096 bytes | run corrupts | keep chunks ≤ 4096 |
| Host stops in a chunk wait | ~5 s stall; result varies | `0x0F` leaves after the loop |
| Host stops inside `XCOMMENT` | run returns `0x01` at the next boundary | `0x0F` leaves; no reset |
| Byte sent while busy | run aborts with `0x01` | send only the next chunk |
| Extra bytes while not asked | stream desyncs | keep the stream request-driven |
| Zero-length chunk | ignored and re-requested | answer `0xFF` with a non-zero length |
| Chunk length lost or reordered | stream desyncs | reset and replay the file |
| More than 63 devices | count clamped to 63 | first 63 reported; not wrapped |
| `0x0F` during a run | consumed as data or busy byte | wait for the result, or press reset |
| Host tool stops mid-run | player may still be running | do not replay; wait, check, reset |
| Result byte not read | next reply is one byte behind | read one result byte per run |
| Expecting a text reply from `0x18` | byte protocol, not text | read the result byte |
| VREG short or rail missing | no entry error; no target power | check rails and regulator |

## Leaving and recovery

The full matrix is in [entry.md](entry.md#leaving).  For a failed host
command:

| Stop point | Action |
|---|---|
| host tool stopped between operations | try the terminal; otherwise reset |
| host tool stopped mid-run | wait, check the target, then reset |

The reset button always works.  `bp xsvf --leave` cannot recover a board
left in the player by an earlier command.

## Unverified

The v3 build contains only the OpenOCD path of the two JTAG paths; its
hardware behaviour is not verified on v3, including the open-drain pin
mode.

## Related

- [README.md](README.md) — overview and index.
- [entry.md](entry.md) — entry, settings and the exit matrix.
- [openocd.md](openocd.md) — the `0x06` command set.
- [xsvf.md](xsvf.md) — the `0x18` player.
- [wiring.md](wiring.md) — pins, levels and pull-ups.
