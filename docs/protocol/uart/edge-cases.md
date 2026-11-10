# UART — edge cases & failure modes

Part of the UART documentation set (`docs/uart/`).  Each entry is a concrete
trigger, the observable symptom, and the recovery.

## Wiring and electrical

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| HDUART with no pull-up | the shared line reads as continuously low; no bytes cross | fit a 1 kΩ–10 kΩ pull-up to 3.3 V; see [hduart.md](hduart.md) |
| HDUART peer is not open-drain capable | the two ends drive against each other; the line is stuck or corrupt | use a peer that runs its USART open-drain / wired-AND |
| Full duplex with TX and RX not crossed | nothing is received; the Bus Pirate sees its own output only if the lines are tied | cross TX↔RX, share ground |
| Open-drain output chosen, no VPU voltage | `P` prints `Warning: no voltage on Vpullup pin`; no idle-high | on v4 select `e2`/`e3` before `P`; on v3 feed Vpullup externally |
| Target is an RS-232 port (±13 V) | risk of damage | use a level converter; the Bus Pirate is logic-level, 5 V safe (max 5.5 V) |

## On the bus

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Speed / frame mismatch | `-f` (and sometimes `-p`) flags on received bytes | match speed, data bits, parity and stop bits at both ends; see [errors.md](errors.md) |
| Bursts faster than the host drains | bytes dropped **silently** (no `*Bytes dropped*` line) | use bridge `(1)` or live monitor `(2)`; see [errors.md](errors.md) |
| `r` with nothing waiting | `READ: FAILED, NO DATA` | not an error — wait and read again |
| Expected the old `*Bytes dropped*` message | it never appears | overruns are silent in this firmware; the bridge shows them only as the MODE LED going off |
| Expected `[` to discard and `{` to display | both open the display | on this firmware `[` and `{` are the same; see [terminal-usage.md](terminal-usage.md) |

## Half duplex (HDUART)

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Self-echo expected after a transmit | `r` prints `FAILED, NO DATA`, not your own byte | correct: the self-echo is suppressed; see [hduart.md](hduart.md) |
| Peer transmits while the Bus Pirate is transmitting | the peer's bytes are lost | there is **no collision detection**; serialise the two ends |
| Auto-baud `(4)` used in HDUART | it samples the unused MISO input, which is not the shared line | auto-baud is for full duplex; set the speed manually |
| 115200 with a 16 MHz peer | `-f` framing errors | the peer's divider truncates (≈ 9 % mismatch); use a matched peer |

## Setup and the settings form

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Typed a settings line echoing the header | taken as the wrong value | the header prints `spd`/`dup` **zero-based** (`0` = first speed / full duplex); type the menu numbers instead |
| Supplied an incomplete one-line list | the mode ignores it and asks every prompt | give all fields, or none; see [entry-and-setup.md](entry-and-setup.md) |
| Expected MIDI in the speed menu | it is not listed | use a custom/raw baud or the binary preset for 31250 bps |
| Expected an output-type prompt on I2C-like terms | UART **does** ask for an output type | answer `1` for open drain (needs a pull-up) or `2` for normal |

## Bridges and macros

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Answering the bridge prompt with a space | the bridge is not entered | type `y`/`Y`; the old "space continues" note is wrong |
| Host opens the virtual serial port at the wrong rate during a bridge | garbled exchange | set the host rate first with `b` (default 115200), then start the bridge; see [bridge.md](bridge.md) |
| Trying to exit the bridge on v4 without the button | it keeps forwarding | press the NORMAL button; on v3 only a hardware reset exits |
| Expected flow control from `(3)` on v4 | none | `(3)` is v4-identical to `(1)`; flow control needs the v3 wiring |
| A number other than 0–4 as a macro | `Unknown macro, try ? or (0) for help` | use `(0)` to list the valid macros |

## Binary interface

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Treating every reply byte as data | `0x01` ACKs interleave with the incoming bytes | separate `0x01` acks from data; send payloads that avoid `0x01` |
| `0x5x` pull-up feed on v3 | answered `0x00` | that group is v4 only |
| Expecting a read-config command (`0x9x`) | it set the config instead | only `0x8x`/`0x9x` **write** config; read it from the settings header instead |
| Forgetting the board bring-up | open-drain default means no idle-high | on v4 send the VPU feed and enable power + pull-ups first; see [binary.md](binary.md) |

## Unverified / v3

- All v3 behaviour in this set is build-only — **not verified**.
- The v4 `-f`/`-p` output on a deliberately mismatched speed, the binary
  preset-speed path (`0x6x`) and the `0x5x` pull-up feed are **unverified**.