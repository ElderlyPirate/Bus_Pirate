# JTAG — edge cases & failure modes

Part of the JTAG documentation set (`docs/jtag/`).  Each entry is a concrete
trigger, the observable symptom, and the recovery.

## Entry and mode

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Looking for a JTAG entry on the mode menu | there is none | JTAG is binary-only: 20 × `0x00` → `BBIO1`, then `0x06` or `0x18` |
| `0x18` sent on a v3 build | a failure byte, not `XSV1` | the XSVF player is v4 only; use the `0x06` OpenOCD path on v3 |
| Sending a JTAG signal byte at the terminal prompt | nothing JTAG happens | there is no terminal JTAG command set; use a host program |
| Trapped in the XSVF player after a host abort | no terminal prompt returns | send `0x0F` (leave the player), then `0x0F` again (leave bitbang), or reset the board |

## Wiring and electrical

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| OpenOCD path, open-drain mode, no pull-ups | lines never reach high; TDO/TAP stuck low | fit 2 kΩ–10 kΩ pull-ups on TDI/TMS/TCK to the target rail; see [wiring.md](wiring.md) |
| OpenOCD path entered, no pin mode set | no clock or data appears | pins default to high-impedance inputs; send `0x01` with mode `1` or `2` first |
| Push-pull output into a lower-voltage chain | contention / possible target damage | use open-drain mode with pull-ups to the target's logic voltage |
| TDI and TDO swapped, or TCK/TMS swapped | every run fails (TDO mismatch) | check TDI→MOSI, TDO→MISO, TCK→CLK, TMS→CS, and common ground |
| Target not powered | TDO floats or stays low; no IDCODE | power the target chain before probing or playing |
| SRST or TRST left asserted (feature on) | chain held in reset; TDO idle | turn the feature off (`0x02 0x08 0x00` / `0x02 0x04 0x00`) |
| Applied voltage above the limit | risk of damage | the interface is 5 V safe, maximum **5.5 V** |

## OpenOCD path

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Host does not follow the `0x05` handshake | stream desyncs | the 3-byte header response is the echo; then send TDI/TMS bytes; read the 1–2 TDO bytes |
| Shift count larger than the limit | only 8192 bits are shifted | split the shift into commands of ≤ `0x2000` bits |
| Link speed raised, confirm bytes wrong | link reverts to normal speed | send `0x07` with the correct `0xAA 0x55` confirm; see [openocd.md](openocd.md) |
| Expected standard JTAG-over-serial | it is a custom `OCD1` command set | use a host that speaks this protocol |
| Unknown command byte | a single `0x00` byte is returned | check the command table in [openocd.md](openocd.md) |

## XSVF player

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| File's longest shift exceeds 400 bits | result byte `0x06` (data overflow) | use a smaller device/file, or a different programmer; see [xsvf.md](xsvf.md) |
| Wrong or corrupted file, or bad wiring | result byte `0x02` / `0x03` (TDO mismatch / max retries) | re-check the file, TDI/TDO, TCK/TMS, ground, and target power |
| A single chunk larger than 4096 bytes | the run corrupts (extra bytes drained) | keep chunks ≤ 4096 |
| Host stops answering the `0xFF` ready byte | the port hangs in the player | always answer each `0xFF` with a length-prefixed chunk, or reset |
| A chunk length byte is lost/reordered | stream desyncs; mismatches or nonsense commands | re-sync by resetting the board and replaying the file |
| More than 63 devices on the chain | the probe count byte wraps | probe in smaller chains or read IDCODEs another way |
| Expecting a terminal reply from `0x18` | you get the byte protocol, not text | read the result byte; see [xsvf.md](xsvf.md) |

## Self-test

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Running the generic self-test to check JTAG | JTAG is not exercised | the self-test covers power, ADC, EEPROM and header pins only; there is no JTAG self-test |
| Expecting a JTAG pin test in the binary self-test | none | same as above — no JTAG-specific self-test exists |

## Unverified / v3

- All v3 behaviour in this set is build-only — **not verified**.
- The XSVF shift-buffer limit (400 bits) and the chunk-buffer behaviour are
  **unverified** on hardware.
- The exact pin-mode and feature behaviour on v3 is **unverified**.