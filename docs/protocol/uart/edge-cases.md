# UART — edge cases & failure modes

Part of the UART documentation set (`docs/protocol/uart/`).  Each entry is a
concrete trigger, the observable symptom, and the behaviour or recovery.

## Wiring and electrical

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| HDUART with no pull-up | the shared line idles low; no words cross | fit a 1 kΩ–10 kΩ pull-up to the target's logic voltage (3.3 V typical), or use `P` with a Vpullup source |
| HDUART peer is push-pull | the two ends drive against each other; the line is stuck or corrupt | use a peer that runs its USART open drain / wired-AND |
| Full duplex TX and RX not crossed | nothing is received; a loopback only works when TX and RX are tied | cross TX→target RX and target TX→BP RX; share ground; for a loopback use normal output or a pull-up |
| Open drain chosen, no pull-up on TX | the target sees no idle-high; no data | fit a pull-up, or choose `2. Normal` |
| Normal output on a shared/open-drain bus | contention when the target drives | use open drain |
| `P` with no VPU voltage | `Pull-up resistors ON`, then `Warning: no voltage on Vpullup pin` | select `e2`/`e3` first (v4), or feed Vpullup (v3); the resistors are still on but pull to nothing |
| `P` or `e` in Normal output | `WARNING: pins not open drain (HiZ)`; `e` is refused | choose open drain at setup, or do not use the on-board pull-ups |
| Target is an RS-232 port (±13 V) | damage risk | use a level converter; the Bus Pirate is logic-level |
| More than 5.5 V on a data line | damage risk | stay within 5 V logic; the limit is board-level (unverified) |

## On the bus

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Speed or frame mismatch | `-f` and/or `-p` flags on received words | match speed, data bits, parity and stop bits at both ends; see [errors.md](errors.md) |
| Sustained traffic faster than the host drains | the 127-word ring fills; words are dropped and the counter increments | pace the sender; use bridge `(1)` or monitor `(2)`; outside a bridge the drop is silent |
| Host stalls during a bridge/monitor | the loop stays live; queued words wait, but the ring is finite | readiness is a liveness aid, not flow control — pace the host |
| `r` with nothing waiting | `READ: FAILED, NO DATA0x00` | not a bus error; wait and read again; the trailing zero is the returned value |
| `r:N` with fewer than N words | values, then `FAILED, NO DATA0x00` for each missing word | send N words, or read as they arrive |
| Expected a `*Bytes dropped*` message | it never appears | overruns are silent outside a bridge; in bridge/monitor/`ART1` the MODE LED turns off |
| Expected `[` to discard and `{` to display | both start the same display | on this firmware `[` and `{` are the same; `]`/`}` stop only the display |
| Value wider than the frame | the display and the wire carry only the low bits | use `0`–`255` in 8-bit frames, `0`–`511` in 9-bit frames |
| 9-bit data through bridge/monitor | only the low 8 bits are forwarded | read 9-bit data in the terminal |
| `r;N` wider than 9 | only the display widens | UART frames 8 or 9 data bits |
| `W` was skipped | `WRITE:` still echoes, but nothing is transmitted; `r` fails; `v` shows `TxD` as `I` | run `W` |
| `W` prints `VREG too low, is there a short?` | `POWER SUPPLIES OFF` follows; the setup did not run | clear the fault, then `W` again; the new settings were not applied |
| `w` was used | the port is off and the rails are down | `W` re-engages with the stored settings and clears the ring |
| `r` before `W` (after entry or after `w`) | a stale word from an earlier session can still be returned | the port is disabled but the ring is not cleared until the next enable; `W` clears it |
| `W`, `(4)`, or a binary speed/frame change | words queued before the step are lost | read the queue before changing speed, or expect the loss |

## Half duplex (HDUART)

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Self-echo expected after a transmit | `r` prints `FAILED, NO DATA0x00`, not your own word | correct: the self-echo is suppressed; see [hduart.md](hduart.md) |
| Peer transmits while the Bus Pirate is transmitting | the peer's words are lost | there is **no collision detection**; serialise the two ends |
| Auto-baud used in HDUART | it measures the shared MOSI line, not MISO | it works if the peer transmits during the wait; otherwise set the speed manually |
| 115200 with a 16 MHz peer | `-f` framing errors | the peer's divider truncates (about 9 % mismatch); use a matched peer |
| MOSI and MISO jumpered "for one wire" | unnecessary wiring; the receiver already listens on MOSI | wire MOSI, GND and the pull-up only; MISO is unused |
| `v` in HDUART | the row still shows `RxD` on MISO | the row is static; the receiver is actually on MOSI |

## Setup and bring-up

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| The header's `spd` is used as the one-line number | header `4` is 9600 bps, but one-line `4` selects 4800 bps | use the menu number (`5` for 9600); the header is zero-based |
| The header line is pasted back | the entry falls back to the prompts | type the six menu values; the `brg` field shifts the rest |
| An incomplete one-line list | the prompts run and the typed values are discarded | give all fields through `output`; `duplex` may be omitted |
| Setup values separated by commas | the entry falls back to the prompts | use spaces; the setup parser consumes only spaces between fields |
| `spd 10` with the `brg` field missing or out of range | a missing `brg` consumes the next field; the entry then falls back to the prompts | always give `brg` with `spd 10`: v4 bps 62–65535, v3 raw 1–32767 |
| v3 one-line `brg` given in bps | the port runs at the wrong rate: `9600` loads divider 9600 (about 416 bps) | on v3 use the raw divider (`416` for about 9600 bps) |
| `Idle 0` plus half duplex | `Invalid choice, try again` at `Duplex:`; a one-line combination falls back to the prompts | use `Idle 1` for half duplex |
| A custom baud below 62 on v4 | `Invalid choice, try again` | use 62 or higher |
| MIDI expected in the speed menu | it is not listed | use `m3 10 127 …` (v3), `m3 10 31250 …` (v4), or binary `0x69` |
| Output type left at the default | the open-drain TX line needs a pull-up | choose `2. Normal`, or fit a pull-up |
| A quoted string contains `\r` | the characters `A`, `T`, `\` and `r` are sent, not a carriage return | use `0x0D`; quoted strings have no escape syntax |
| A 9-bit value above 511 | only the low 9 bits are on the wire | use `0`–`511` |

## Bridges and macros

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Answering the bridge prompt with a space | the bridge is not entered | type `y`/`Y`; any other byte cancels |
| **v3:** host opens the FTDI port at the wrong rate during a bridge | garbled exchange | set the Bus Pirate terminal rate with `b` (default 115200) and open the host port at that same rate, then start the bridge |
| **v4:** host opens the CDC port at any rate during a bridge | no effect — the rate is host metadata | only the target UART speed matters; `b` is a no-op |
| Exiting the v4 bridge without the button | it keeps forwarding | press the NORMAL button; on v3 only a hardware reset exits |
| Expecting flow control from `(3)` on v4 | none | `(3)` is identical to `(1)` on v4; flow control needs the v3 wiring |
| A number other than 0–4 as a macro | `Unknown macro, try ? or (0) for help` | use `(0)` to list the valid macros |
| `(4)` with words already queued | if the port is engaged, the setup re-run clears the receive ring | read the queue before measuring |
| `(4)` while the measurement timer is in use | `** Early Exit!`; no rate is reported | stop the other timer user, then retry |
| `(4)` above about 150 kbaud | `** Baud>16m: The BP cannot measure above 16000000, Done.` | the printed ceiling is the message text; the check is at 150 kbaud |
| `(2)` exit key | the key is consumed and not echoed | normal; the prompt returns |

## Binary interface

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Treating every reply byte as data | command ACKs are `0x01`; queued RX echo appears between commands | read the expected ACK count (one command ACK plus one per bulk byte), then treat extra bytes as RX |
| `0x5x` pull-up feed on v3 | answered `0x00` | that group is v4 only |
| `0x5x` in normal output | answered `0x00`; no source selected | set open drain first (`0x8x` with bit 4 clear) |
| `0x5x` with a voltage already on Vpullup | answered `0x00` | remove the external feed, or use it instead of the on-board source |
| `0x6x` operand `0xA`–`0xF` | answered `0x00`; the port is not reconfigured | use only `0`–`9` |
| `0x07` with divider `0` | about 4 Mbaud; not a default | the divider is used as given; do not send `0` unless intended |
| `0x07`, `0x6x` or `0x8x`/`0x9x` | queued receive words are lost | the port is re-enabled; expect the flush |
| Expecting a read-config command (`0x9x`) | it set the config instead | there is **no binary read-config**; track the last write host-side |
| Sent `0x0F` inside `ART1` | the byte stream becomes the bridge and commands stop | v4: NORMAL button returns to the `ART1` loop; v3: hardware reset only — do not send unless you can reach it |
| `0x03` inside `ART1` | it turns echo off, it does not re-enter UART | the byte meanings differ between BBIO1 and `ART1`; check the state |
| `0x00` inside `ART1` | it leaves to BBIO1; it does not reset the board | from BBIO1, `0x00` re-prints `BBIO1` |
| Forgetting the board bring-up | open-drain default means no idle-high | v4: send `0x51` then `0x4C`; v3: feed `Vpu` externally then `0x4C`, or send `0x90`; see [binary.md](binary.md) |
| 9-bit frame with a bulk write | the ninth bit is zero | bulk is byte-wide; use the terminal for 9-bit data |
| Terminal UART settings expected in `ART1` | the port comes up at 300 bps, 8/NONE, open drain, echo off | set speed, frame and echo explicitly |

## Related docs

- [entry-and-setup.md](entry-and-setup.md) — settings, defaults and `W`.
- [terminal-usage.md](terminal-usage.md) — the receive ring and display.
- [errors.md](errors.md) — flags, overruns and failed reads.
- [hduart.md](hduart.md) — the half-duplex failure modes in context.
- [binary.md](binary.md) — the binary command set and its limits.
