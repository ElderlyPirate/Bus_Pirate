# JTAG — edge cases & failure modes

Part of the JTAG documentation set.  Each entry is a concrete trigger, the
observable symptom, and the recovery.

## Entry and mode

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Mode menu JTAG entry | none | binary-only: `20 × 0x00` → `BBIO1`, then `0x06`/`0x18` |
| `0x18` on a v3 build | `0x00`, not `XSV1` | XSVF is v4 only; use `0x06` on v3 |
| JTAG byte at the terminal prompt | nothing happens | no terminal commands; use a host |
| `0x00` to leave the XSVF player | player ignores it | send `0x0F` instead |

## Wiring and electrical

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| OpenOCD open-drain, no pull-ups | lines never high | fit 2–10 kΩ on TDI/TMS/TCK |
| OpenOCD entry, no pin mode | no clock or data | pins high-Z; send `0x01 0x01` first |
| Push-pull into a lower-voltage chain | contention, damage | use open-drain + pull-ups |
| TDI/TDO or TCK/TMS swapped | TDO mismatch every run | check the signal map and ground |
| Target not powered | TDO low or floating; no IDCODE | power the target before probing |
| SRST/TRST left asserted | chain held in reset | `0x02 0x08 0x00` / `0x02 0x04 0x00` |
| Applied voltage above the limit | damage risk | 5 V safe, maximum 5.5 V |
| Target powered externally during XSVF | regulator back-driven | remove the external supply |

## OpenOCD path

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `0x05` handshake not followed | stream desyncs | read the 3-byte echo, then TDI/TMS; read TDO |
| Shift count above the limit | only 8192 bits shift | split into ≤ `0x2000`-bit commands |
| `0x07` fast on v3, bad confirm | reverts to normal | resend `0xAA 0x55`; v4 is host-set |
| Expected standard JTAG-over-serial | custom `OCD1` set | use a compatible host |
| Unknown command byte | single `0x00` | check the table in [openocd.md](openocd.md) |

## XSVF player

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Longest shift > 400 bits | `0x06` data overflow | smaller file/device, or another tool |
| Wrong file or bad wiring | `0x02`/`0x03` TDO mismatch | check file, wiring, ground, power |
| Chunk larger than 4096 bytes | run corrupts (extra drained) | keep chunks ≤ 4096 |
| Host stops during a chunk wait | ~5 s, result `0x04`; loop returns | `0x0F` leaves |
| Byte sent while busy (`XWAIT`/`XRUNTEST`) | run aborts with `0x01` | send only the next chunk |
| Zero-length chunk | ignored and re-requested | answer `0xFF` with a non-zero length |
| Chunk length lost/reordered | stream desyncs | reset and replay the file |
| More than 63 devices | count clamped to 63 (`0xFC`) | first 63 reported; not wrapped |
| TDO stuck low (unpowered/miswired) | 63-device cap and garbage IDCODEs | check power and TDO |
| Expecting a terminal reply from `0x18` | byte protocol, not text | read the result byte |

## Self-test

There is no JTAG self-test, terminal or binary.  The generic self-test (`~`,
binary `0x10`/`0x11`) covers power, ADC, EEPROM and the header pins only; it
does not drive or probe a JTAG chain.

## Unverified / v3

- All v3 behaviour in this set is build-only — **not verified**.
- The XSVF shift-buffer limit (400 bits) and the chunk-buffer behaviour are
  **unverified** on hardware.
- The exact pin-mode and feature behaviour on v3 is **unverified**.
