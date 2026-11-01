# I2C — edge cases & failure modes

Part of the I2C documentation set (`docs/i2c/`).  Each entry is a concrete
trigger, the observable symptom, and the recovery.

## Wiring and pull-ups

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| No pull-ups (or too weak) on SDA/SCL | the address scanner reports a device at **every** address | fit 2 kΩ–10 kΩ pull-ups to the VPU voltage, then re-scan |
| A line stuck low (short, or target holding it) | start or scan prints `Warning: *Short or no pull-up`; the scanner returns without scanning | clear the short / fit the pull-up, then retry |
| `P` with no voltage on VPU | pull-ups enable but print `Warning: no voltage on Vpullup pin`; the bus has no idle-high | on v4 select a source with `e2`/`e3` before `P`; on v3 feed VPU externally |
| `e` while a voltage is already on VPU | `Warning: already a voltage on Vpullup pin` (and a syntax error if the line is malformed) | remove the external source, or leave the source as it is |

## Read / ACK handling

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| A read byte followed by a start or stop | `NACK` is printed before the next condition | this is correct: the pending ninth bit is NACKed to end the read |
| Two reads in a row | the second prints `READ:  ACK <value>` — the ACK belongs to the **first** byte | expected; the ACK for a byte is sent with the next command |
| A read left unresolved (no following command) | the ninth bit stays pending silently — no warning is printed | send `]` (resolves as NACK and stops) or `[` before doing anything else |
| Expected a `*(N)ACK PENDING` banner | it never appears | that banner is from an older firmware; the current firmware shows the state only as the `ACK`/`NACK` prefix of the next line |
| A write issued while a read ACK is pending | the line prints `ACK` then `WRITE: … ACK/NACK` | expected: the write flushes the pending ACK first |

## Clock stretching

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Slave holds SCL low for a slow operation | the byte just takes longer; no error | handled: the driver waits per bit (see [clock-stretching.md](clock-stretching.md)) |
| Slave holds SCL low longer than the timeout (~25 ms) | the bit is abandoned; an acknowledge slot reports `NACK` | check the slave for a stuck clock; the Bus Pirate is not hung and recovers when the slave releases SCL |

## Macros

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `(4)` run before `(3)` | nothing happens, no message | run `(3)` first, then `(4)` |
| `(3)` used once, then the mode is left and re-entered | subsequent bus operations still target the on-board EEPROM | the redirect persists until a reset; reset the Bus Pirate to return to the external bus |
| `(3)`/`(4)` run on a v3 build | `Unknown macro, try ? or (0) for help` | those macros are v4 only |
| A number other than 0–4 as a macro | `Unknown macro, try ? or (0) for help` | use `(0)` to list valid macros |

## Setup and settings form

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Expected an `Open drain` / `Normal` prompt on entry | it is not asked | I2C is fixed open-drain; there is no output-type prompt |
| Typed a settings line echoing the header | rejected or taken as the wrong driver/speed | the header prints the driver and speed **zero-based** (`0` = Software, `0` = first speed); type the menu numbers instead (`1`–`2`, `1`–`4`) |
| Entering I2C on v3 hardware with an old PIC (rev A3 or earlier) | `WARNING: HARDWARE I2C is broken on this PIC! (REV A3)` | use the Software driver on that board |

## Binary interface

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `0x08` write-then-read with a count over the terminal buffer (4096 B on v4, 2048 on v3) | reply `0x00`; no bus traffic, no payload consumed | reduce the counts |
| `0x08` write-then-read with both counts zero | reply `0x00` | request at least one write or read byte |
| A device NACKs during an `0x08` write phase | reply `0x00` | the firmware aborts without sending a stop — send `0x03` (stop) before starting a new transaction if the bus may be left running |
| Sniffer (`0x0F`) stopped with a host byte | the stopping byte is consumed by the I2C binary loop, but the following bytes are processed normally | send the next command as usual |
| Assumed binary I2C follows the interactive driver choice | it never uses the hardware engine | binary I2C is always the software driver |

## Unverified / v3

- The on-board EEPROM macros, the hardware-driver branch, the address-scanner
  capture and the sniffer format/capture were not exercised on this bench; they
  are read from the firmware.
- All v3 behaviour in this set is build-only — **not verified on this bench**.