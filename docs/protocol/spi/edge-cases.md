# SPI — edge cases & failure modes

Part of the SPI documentation set (`docs/protocol/spi/`). Each entry is a
concrete trigger, the observable symptom, and the recovery.

## Bring-up

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Entered the mode but skipped `W` | writes echo `WRITE: 0xNN`, but nothing is clocked and reads return `0xFF` | `W` runs the pin and module setup; run it even when the target is self-powered |
| `W` prints `VREG too low, is there a short?` | `POWER SUPPLIES OFF` follows and the settings are not applied | clear the short, then run `W` again |

## Chip select

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Chose `CS` instead of the default `/CS` | CS is asserted with the opposite level; the target ignores the transaction | re-enter SPI mode (`m5`) and pick the other CS option |
| Expected `CS ENABLED` but see `/CS ENABLED` | the `/` prefix appears | the prefix marks the `/CS` polarity; with `CS` the line has no prefix — see [bus-operations.md](bus-operations.md) |
| Wrote a byte with no `[` or `{` first | the clocks are sent but CS keeps its previous state; the target may ignore them | assert CS with `[` (or `{`) before the transaction |
| Expected `]` and `}` to differ | both release CS and print the same line | they are identical; `{`/`}` and `[`/`]` only differ on the read-back flag |

## The read-back form

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Used `{` and a later write also printed `READ:` | every write reports the byte clocked in | the flag persists across writes and across `]`/`}`; send `[` to clear it |

## Setup and settings form

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Typed an out-of-range value in the one-line form | the interactive prompts run | six integers: speed 1–12, then five values 1–2 ([entry-and-setup.md](entry-and-setup.md)) |
| Typed the settings header back verbatim | the interactive prompts run | the labels and parentheses are not numbers; type the six menu numbers, e.g. `m5 4 1 2 1 2 2` |
| Expected no pull-up by default | the default output type is open-drain, which needs a pull-up | select `2` (`Normal`) to run without a pull-up |

## Macros

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `(3)` … `(9)` | `Unknown macro, try ? or (0) for help` | those macros do not exist; only `0`–`2` and `10`–`15` are defined |
| Changed the clock with a tweak macro mid-transaction | the header prints with the new value; the device may mis-read the current byte | change clock settings between transactions, not inside one |
| Wanted to change CS polarity with a macro | no macro changes CS polarity | re-enter the mode; macros 10–15 change clock polarity/edge/sample only |

## Sniffer

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Connected the target, then entered the sniffer | risk of momentarily driving/grounding the target's clock or data | enter the sniffer **before** connecting the target |
| Stopped the sniffer while the external master was still clocking | exit restores CLK/MOSI/CS as outputs and can contend with the master | stop only while the master is quiet |
| Display looks like garbage | the capture text cannot be trusted | exit with any key, then reset with `#` or power-cycle the Bus Pirate |
| Long burst, CS marker ring or FIFO overflow (interactive) | `Couldn't keep up` is printed and the sniffer restarts | lower the bus rate / shorten the burst, or use a logic analyzer |
| Long burst or overflow (binary `0x0D`/`0x0E`) | one unescaped `0x00` byte, then no more capture; the mode LED is off | send a byte to leave; that byte is the next command |
| CS markers missing on short pulses | `[`/`]` can be absent | v3 polls the CS edge; on v4 CN can merge adjacent edges |
| Expected the sniffer to honour the sample phase | MISO/MOSI bytes may look inverted for some targets | the sniffer follows clock polarity and edge but not sample phase |
| Typed commands while sniffing | they are not executed — the loop is watching the bus | press a key to exit, then type the command |
| Expected a stopping byte to be dropped in binary mode | it is left as the next command | treat the stopping byte as the start of the next request |

## Binary

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `0x5x` pull-up feed on v3 | replies `0x00` | that group is v4 only |
| `0x6x` with an index of 12 or more | replies `0x00` | use an index 0–11 |
| `0x04`/`0x05` with both counts zero | replies success with no bus traffic | intended: a client uses it at initialisation |
| Expected `0x06` to be absent | it is present in this build | the extended AVR option is enabled; a build without it answers `0x00` |
| A stalled SPI module | remaining read bytes are `0xFF`; the reply length is unchanged | the fill is framing, not an error status — see [binary.md](binary.md) §Transfer faults |
| `r`/`r:N` returns `0xFF` | a real target byte can also be `0xFF` | there is no separate timeout error to tell them apart |
| Expected streaming read/write | not present | there are no streaming commands; use `0x04`/`0x05` or bulk `0x1x` |
| AVR bulk read refused | replies `0x00` | address is a word address ≤ `0xFFFF`, length is bytes, and address + ceil(length/2) ≤ `0x10000` |

## Unverified / v3

- The sniffer's maximum usable rate, the exact CS timing on v3, and the
  in-place clock tweaks' safety mid-transaction are **not verified**.
- All v3 behaviour in this set is build-only and **not verified**.
