# JTAG — edge cases & failure modes

Part of the JTAG documentation set.  Each entry is a concrete trigger, the
observable symptom, and the recovery.

## Entry and mode

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Looking for a JTAG menu entry | no menu entry | binary only: `20 × 0x00` → `BBIO1`, then `0x06`/`0x18` |
| `0x18` on a v3 build | `0x00`, not `XSV1` | XSVF is v4 only; use `0x06` on v3 |
| JTAG byte at the terminal prompt | nothing happens | no terminal commands; use a host |
| A non-NUL byte during the 20-NUL entry | no `BBIO1` | resend 20 consecutive `0x00` bytes |
| `0x00` to leave the XSVF player | player ignores it | send `0x0F` instead |
| Entering binary from a mode | the header pins reset, VREG switches off and the on-board pull-ups switch off; AUX1/AUX2 keep their state | re-check wiring and target power; AUX1 needs a board reset |

## Wiring and electrical

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| OpenOCD open-drain, no pull-ups | lines never go high | fit 2–10 kΩ on TDI/TMS/TCK |
| OpenOCD open-drain, SRST/TRST used | resets never release | fit pull-ups on SRST/TRST too |
| OpenOCD entry, no pin mode | no clock or data | pins are high-Z; send `0x01 0x01` first |
| Push-pull into a lower-voltage chain | contention, damage risk | OpenOCD only: open-drain and pull-ups; the XSVF player cannot |
| 3.3 V push-pull into a 5 V target | marginal high level | check the target's input threshold |
| TDI/TDO or TCK/TMS swapped | TDO mismatch, no IDCODE | check the signal map and ground |
| Target not powered | TDO low or floating; no IDCODE | power the target, or enable VREG |
| TRST held low | test logic in reset; all-zero reads | release with action `1` (active-low) |
| SRST held low | CPU/system held in reset | release with action `1` (active-low) |
| OpenOCD pin mode set while SRST/TRST wired | resets held asserted | release both after pin mode |
| OpenOCD pin mode change | low pulse on SRST/TRST | re-initialise the target after the change |
| Target powered externally during XSVF | regulator back-driven | remove the external supply |
| Vpullup has no voltage | on-board pull-ups do nothing | feed Vpullup externally |
| Applied voltage above the limit | damage risk | 5 V safe, maximum 5.5 V |

## OpenOCD path

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Echo not read | payload/TDO misaligned | read the 3-byte echo before the TDO bytes |
| Shift count > 8192 | only 8192 bits shift; surplus becomes commands | split into ≤ `0x2000`-bit shifts |
| Zero-bit shift on v4 | no payload, no TDO byte | do not wait for a reply; v3 is unverified |
| `0x07` fast with a bad confirm | link reverts; reply `0x00` | resend `0xAA 0x55`, or use normal |
| `0x07` fast on v4 | reply says fast, rate unchanged | set the host CDC rate yourself |
| `0x08` sent before a shift | no reply | do not wait; the next byte is a command |
| `0x0F` inside OpenOCD | single `0x00`; mode stays | leave with `0x00`, then `0x0F` |
| Unknown command byte | single `0x00` | check the command table |
| Expecting standard JTAG-over-serial | custom `OCD1` framing | use a compatible host |
| Leaving with `0x00` | target TAP keeps its state | reset the chain before leaving if needed |

## TAP and scan

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Wrong IR length | wrong instruction reaches a device | check the chain order and IR lengths |
| Wrong DR length | shifted IDCODE/register data | check 32 bits per IDCODE device |
| TDO stuck low | all-zero reads; probe reports 63 devices | check power, TDO and ground |
| TDO stuck high | all-one reads; probe reports zero devices | check TDO, ground and the target |
| TDO floating | random data | check power and ground; add a target-side pull |
| Chain longer than 63 on `0x02` | count clamped at `0xFC` | probe a shorter chain |
| Device without IDCODE | wrong count/IDCODE data | check the chain; use an explicit scan |
| Chain IR longer than 510 bits | probe count/IDCODE data wrong | use an explicit scan |
| TCK too fast for the target | intermittent mismatches | use a larger `0x08` delay from a custom host |
| Missing ground | intermittent or no data | connect a common ground |

## XSVF player

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Longest shift > 400 bits | result `0x06` data overflow | use a smaller file/device |
| Wrong file or bad wiring | result `0x02`/`0x03` TDO mismatch | check file, wiring, ground, power |
| Chunk larger than 4096 bytes | run corrupts (surplus drained) | keep chunks ≤ 4096 |
| Host stops during a chunk wait | ~5 s, result `0x01`, `0x04`, `0x05` or `0x06` | `0x0F` leaves once the loop returns |
| Host stops inside an `XCOMMENT` string | player does not return; `0x0F` is ignored | reset the board |
| Byte sent while busy (`XWAIT`/`XRUNTEST`) | run aborts with `0x01` | send only the next chunk |
| Extra bytes while not asked | stream desyncs | keep the stream request-driven |
| Zero-length chunk | ignored and re-requested | answer `0xFF` with a non-zero length |
| Chunk length lost or reordered | stream desyncs | reset and replay the file |
| More than 63 devices | count clamped to 63 (`0xFC`) | first 63 reported; not wrapped |
| TDO stuck low | 63-device cap and garbage IDCODEs | check power and TDO |
| `0x0F` during a run | consumed as data or busy byte | wait for the result or press reset |
| Host tool stops mid-run | player may still be running | do not replay; wait, check, reset |
| Result byte not read | next reply is one byte behind | read one result byte after each run |
| Expecting a terminal reply from `0x18` | byte protocol, not text | read the result byte |
| VREG short | no entry error; target unpowered | check the `3.3V`/`5.0V` rails and the regulator |
| Target relies on VREG but rail missing | no entry error; probe fails | check the rail wiring and ground |

## Self-test

There is no JTAG self-test, terminal or binary.  The generic self-test (`~`,
binary `0x10`/`0x11`) covers power, ADC, EEPROM and the header pins only; it
does not drive or probe a JTAG chain.

## Unverified / v3

- All v3 behaviour in this set is build-only — **not verified**.
- The XSVF shift-buffer limit (400 bits) and the chunk-buffer behaviour are
  **unverified** on hardware.
- The exact pin-mode and feature behaviour on v3 is **unverified**.

## Related

- [README.md](README.md) — overview and index.
- [openocd.md](openocd.md) — the `0x06` command set.
- [xsvf.md](xsvf.md) — the `0x18` player.
- [wiring.md](wiring.md) — pins, levels and pull-ups.
