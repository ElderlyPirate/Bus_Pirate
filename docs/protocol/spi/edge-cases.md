# SPI — edge cases & failure modes

Part of the SPI documentation set (`docs/spi/`). Each entry is a concrete
trigger, the observable symptom, and the recovery.

## Chip select

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Chose `CS` instead of the default `/CS` | CS is asserted with the opposite level; the target ignores the transaction | CS polarity is fixed at setup — re-enter SPI mode (`m5`) and pick the other option |
| Expected a plain `CS ENABLED` line but see `/CS ENABLED` | the `/` prefix appears | the prefix marks the `/CS` polarity; with the `CS` polarity the line has no prefix (see [bus-operations.md](bus-operations.md)) |
| Wrote a byte with no `[` or `{` first | the clocks are sent but CS keeps its previous state; the target may ignore them | assert CS with `[` (or `{`) before the transaction |
| Expected `]` and `}` to differ | both release CS and print the same line | they are identical; `{`/`}` and `[`/`]` only differ on the read-back flag |

## The read-back form

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Used `{` and then a later write also printed `READ:` | every write reports the byte clocked in | the flag set by `{` persists across writes and across `]`/`}`; send `[` to clear it for a plain write |

## Setup and settings form

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Typed a settings line with `spd` above `4` | the interactive prompts run instead of the line being accepted | the one-line form only validates speeds 1–4; either use a speed ≤ 4 or answer the prompts |
| Typed the settings header back verbatim | the interactive prompts run instead of the line being accepted | the parser expects six bare integers right after the mode number; the header's labels and parentheses are not numbers, so no value is read and the prompts run. The header's `0`/`1` field values are also out of range (the two-option fields must be `1`/`2`) — see [entry-and-setup.md](entry-and-setup.md) |
| Expected the chain to need no pull-up by default | the default output type is open-drain, which needs a pull-up | select `2` (`Normal`) to run without a pull-up |

## Macros

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `(3)` … `(9)` | `Unknown macro, try ? or (0) for help` | those macros do not exist; only `0`–`2` and `10`–`15` are defined |
| Changed the clock with a tweak macro mid-transaction | the header prints with the new value; the device may mis-read the current byte | change clock settings between transactions, not inside one |
| Wanted to change CS polarity with a macro | no macro changes CS polarity | re-enter the mode; only macros 10–15 exist, and they change clock polarity/edge/sample only |

## Sniffer

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Connected the target, then entered the sniffer | risk of momentarily driving/grounding the target's clock or data | enter the sniffer **before** connecting the target; if the display is garbage, reset with the CS pin |
| Long burst overtakes the buffer (interactive) | `Couldn't keep up` is printed and the sniffer restarts | lower the bus rate / shorten the burst, or use a logic analyzer |
| Long burst overtakes the buffer (binary `0x0D`/`0x0E`) | the sniffer stops and the mode LED turns off | the host should stop; discard data captured after the cut |
| Expected the sniffer to honour the sample-phase setting | MISO/MOSI bytes may look inverted for some targets | the sniffer follows clock polarity and edge but not sample phase |
| Very short CS pulses on v3 | the `[`/`]` markers can be missing | v3 polls the CS edge; use a wider pulse, or v4 (hardware-latched CS) |
| Typed commands while sniffing | they are not executed — the loop is watching the bus | press a key to exit, then type the command |
| Expected a stopping byte to be dropped in binary mode | it is left as the next command | treat the stopping byte as the start of the next request |

## Buffer and binary

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| Sniffer output exceeds the terminal buffer | data is dropped; the interactive sniffer restarts, the binary sniffer stops | v4 buffer is 4096 bytes, v3 is 2048 — drain often, or use a logic analyzer |
| `0x5x` pull-up feed on v3 | replies `0x00` | that group is v4 only |
| `0x6x` with an index of 12 or more | replies `0x00` | use an index 0–11 |
| `0x04`/`0x05` with both counts zero | replies success with no bus traffic | intended: a client uses it at initialisation |
| Expected `0x06` to be absent | it is present in this build | the extended AVR command is compiled in; a build without the option answers `0x00` |
| Expected streaming read/write | not present | the streaming options are disabled in this build; use `0x04`/`0x05` or bulk `0x1x` |

## Unverified / v3

- The sniffer's maximum usable rate, the exact CS timing on v3, and the
  in-place clock tweaks' safety mid-transaction are **not verified**.
- All v3 behaviour in this set is build-only and **not verified**.