# SPI — edge cases & failure modes

Part of the SPI documentation set (`docs/protocol/spi/`). Each entry is a
concrete trigger, the observable symptom, the behaviour, and the recovery.
The main topics are in [entry-and-setup.md](entry-and-setup.md),
[wiring.md](wiring.md), [bus-operations.md](bus-operations.md),
[macros.md](macros.md), [sniffer.md](sniffer.md) and [binary.md](binary.md).

## Bring-up and power

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Entered the mode but skipped `W` | writes echo `WRITE: 0xNN` but nothing is clocked; reads return `0xFF`; `v` shows CLK/MOSI/CS as inputs | run `W`; it runs the pin and module setup |
| `W` prints `VREG too low, is there a short?` | `POWER SUPPLIES OFF` follows and the setup is not applied | clear the short, then run `W` again |
| `W` succeeds with the target unpowered | the Bus Pirate rails are up; the target has no supply | `W` checks its own rails; power the target separately |
| Self-powered target, VCC wired to a Bus Pirate rail | back-drives the target or the rail | leave the 3.3 V/5 V pins off the target; `W` is still required |
| `w` is used | `Clutch disengaged!!!`, `POWER SUPPLIES OFF`; the pins are inputs | run `W` again; the SPI settings are retained |
| BASIC `MODE 5` is used | the bus is configured but the supplies stay off | use a self-powered target, or complete the terminal setup including `W` and do not use `MODE` in the program |
| A direct `m<N>` number outside the menu is typed | `Nonexistent protocol!`; the mode does not change | take the number from `m` |

## Wiring and electrical

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| No pull-up in open drain | CS/CLK/MOSI float; a stuck clock or asserted CS | fit 2–10 kΩ to the target rail, `P` with Vpullup fed, or use Normal output |
| Vpullup is set to 5 V for a 3.3 V target | the pull-ups put 5 V on CS/CLK/MOSI | use `e2` for 3.3 V, `e3` only for 5 V, or feed `Vpu` at 3.3 V |
| `P` with no voltage on Vpullup | `Warning: no voltage on Vpullup pin`; the pull-ups do not pull high | `e2`/`e3`, `W`, then `P` on v4; external `Vpu` on v3 |
| Normal output is used on a 5 V bus | the high level is 3.3 V; the target may not accept it | use open drain with Vpullup at 5 V |
| Active-high `CS` is used with open drain | the asserted level is high impedance and depends on the pull-up | fit the pull-up or use Normal output |
| MISO floats (target absent, not selected, or tri-stated) | `r` returns `0x00`, `0xFF` or noise | power and select the target; add the bus's normal MISO pull-up if needed |
| More than 5.5 V is applied to a signal | damage risk | the interface is 5 V safe, 5.5 V maximum |
| `v` shows `O` but the line is not driven high | open-drain high is high impedance | expected; the pull-up sets the high level |

## Chip select and the read-back flag

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `CS` was selected instead of `/CS` | CS is asserted with the opposite level; the target ignores the transaction | re-enter the mode and pick the other CS option |
| `CS ENABLED` was expected but `/CS ENABLED` appears | the `/` prefix is part of the `/CS` label | the prefix marks the active-low polarity; with `CS` the line has no prefix |
| A byte is written with no `[` or `{` first | the clocks are sent but CS keeps its previous state; the target may ignore them | assert CS with `[` (or `{`) first |
| `]` and `}` are expected to differ | both release CS and print the same line | they are identical; only `[`/`{` differ on the read-back flag |
| `{` was used and a later write also printed `READ:` | every write reports the byte clocked in | the flag persists across writes and across `]`/`}`; send `[` to clear it, or re-enter the mode |
| `w`/`W` is used after `{` | the read-back flag is still set | expected; only `[` or a mode re-entry clears it |
| Active-high `CS` is used in open drain | the asserted level is high impedance | the pull-up must be present for the target to see the assertion |

## Transfers

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A bit-level command (`/` `\` `^` `-` `_` `.` `!`) is typed | the no-effect error and a syntax error, possibly after the normal prefix | SPI has no bit-level handler; use byte writes and reads |
| `;N` is set above 8 | the printed value is wide but only the low byte is clocked | set `;8`; SPI is byte-wide |
| A value above `0xFF` is written | the printed value can be wider, but only the low byte reaches the bus | send one byte per value |
| `l`/`L` is used with `;N` other than 8 | the bit reversal window does not match the byte | use `;8` for a predictable byte reversal |
| A 16-bit transfer is expected | there is no word command; the value is truncated | send two bytes and assemble the word in the host |
| `r` returns `0xFF` | a real target byte can also be `0xFF` | there is no separate timeout error to tell them apart |
| `r` is used with CS released | the dummy byte is clocked with CS in its current state | assert CS first if the target needs it |
| `r:N` is used | `READ: ` is printed once, then N values | expected; the shared repeat grammar owns the count |

## Setup and settings

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A value is missing or out of range in the one-line form | the interactive prompts run; the typed values are discarded | type six integers: speed 1–12, then five values 1–2 |
| The settings header is typed back | the prompts run | type the menu numbers, not the `spd ckp …` labels |
| Six values are in the wrong positions | each value is accepted if in range; the settings are wrong | check the echoed header and re-enter the mode |
| `m5` is typed with no values | the six prompts run | answer them, or use the one-line form |
| `m` then `5` is used | after setup the menu form prints `Clutch disengaged!!!` and the finish prompt | the direct `m5` form prints only `Ready` |
| The output type is left at the default | the open-drain bus needs a pull-up | select `2` Normal, or fit a pull-up |
| A speed above the target's limit is selected | the clock is faster than the target supports | the rates are nominal; choose a lower entry |
| The header shows `hiz 1` | the output is open drain, not Normal | expected; `hiz` is the output-type field, `1` = open drain |
| The header shows `csl 0` | the CS polarity is `CS` (active-high) | `0` = `CS`, `1` = `/CS` |
| The header shows `spd 4` | the speed is menu option 4 (`1MHz`) | the header's `spd` is one-based |

## Macros

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `(3)` … `(9)` is used | `Unknown macro, try ? or (0) for help` | those macros do not exist; only 0–2 and 10–15 are defined |
| A macro is expected to change CS polarity, output type or speed | no such macro exists | re-enter the mode for CS polarity; speed/output type can be set in binary with `0x6x`/`0x8x` |
| A clock tweak is run mid-transaction | the header prints with the new value; the device may misread the current byte | change clock settings between transactions |
| A tweak is run, then `w`/`W` | the tweak is still in effect | expected; a mode re-entry restores the setup values |
| A tweak is run before `W` | the header prints; the bus is not engaged | run `W`; the stored value is applied then |
| `(0)` is run before `W` | the macro list prints | expected; `W` is still needed for the bus |

## Sniffer

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| The target is connected, then the sniffer is entered | the master configuration can drive CS/CLK/MOSI before the macro runs | enter the sniffer first, then connect |
| The sniffer is stopped while the external master is clocking | exit configures CLK/MOSI/CS as outputs; contention | stop only while the master is quiet |
| `(1)` is used on an active-high `CS` bus | the transaction is not captured; the gate follows the idle level | use `(2)` |
| The clock polarity or edge is wrong | bytes look shifted or inverted | match the master's polarity/edge; the sample phase is fixed at middle |
| CS markers are missing on short pulses | `[`/`]` can be absent | v3 polls and needs a wider pulse; v4 CN can merge edges |
| A long burst overflows (interactive) | `Couldn't keep up`; the sniffer restarts without the header | lower the rate, shorten the burst, or use a logic analyzer |
| A long burst overflows (binary) | one unescaped `0x00`, no more capture, the mode LED is off | send a byte to leave; that byte is the next command |
| Commands are typed while sniffing | they are not executed | press a key to exit, then type the command |
| The stopping byte in binary mode is expected to be dropped | it becomes the next command | account for it; `0x00` exits to `BBIO1` |
| `0x00` is sent to stop the binary sniffer | the sniffer stops and the next command is `0x00`, which exits SPI | send `0x01` instead to stay in `SPI1` |
| `0x01` is sent to stop the binary sniffer | the sniffer stops and `SPI1` is reported | the stopping byte becomes the next command; `0x01` is the identifier request |
| `0x0F` is sent to stop the binary sniffer | it is answered `0x00`, not a reset | stop with `0x00` to `BBIO1`, then use `0x0F` |
| The display is garbled | the capture text cannot be trusted | exit with any key, reset with `#` or power-cycle, re-enter |

## Binary

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `0x0F` is sent inside `SPI1` | `0x00`, no reset | send `0x00` to reach `BBIO1`, then `0x0F` |
| `0x18` is sent inside `SPI1` | a nine-byte bulk transfer, not the XSVF entry | entry bytes are only meaningful in `BBIO1` |
| `0x10` is sent inside `SPI1` | a one-byte bulk transfer, not the self-test | entry bytes are only meaningful in `BBIO1` |
| `0x5x` is sent on v3 | `0x00` | the group is v4 only |
| `0x6x` uses index 12–15 | `0x00` | use 0–11 |
| `0x04`/`0x05` both counts are zero | `0x01`, no bus traffic, CS unchanged | intended for initialisation |
| `0x04` after a `0x4x` released CS | not a failure: `0x04` drives CS low and makes it an output for the transfer, then releases it | no recovery needed |
| `0x03` after a `0x4x` released CS | CS released (input in open drain, driven high in Normal) | expected; use `0x02` to assert |
| `0x06` without CS | the AVR sees no chip select; the read is meaningless | first assert CS with `0x02` (it enables the output) |
| `0x06` bulk read succeeds | `0x01 0x01` before the data | account for both acks |
| `0x06` bulk read is refused | `0x01 0x00` | address ≤ `0xFFFF`, length ≤ `0xFFFF`, no word wrap |
| `0x06` length is odd | the last word returns only its low byte | expected |
| `0x06` is used without an AVR in ISP state | no valid flash data | put the AVR in its ISP flash-read state; the helper does not drive reset |
| `0x6x` or `0x8x` is sent mid-transfer | CS returns to its idle level | change speed/options between transfers |
| `0x8x` clears an option bit | the bit's own default applies, not the entry default | `0x82` keeps the entry edge; `0x80` selects idle→active |
| `0x5x` is sent after Normal output was selected | `0x00` | select open drain with `0x8x` first |
| `0x4x` turns the supply on | `0x01` even if the rail is shorted | there is no rail check; use the ADC or the terminal to verify |
| `0x1x` without CS | bytes clocked with CS in its current state | if the target needs CS, assert it with `0x02` (it enables the output) |
| `0x7x` uses a low nibble other than 0 | `0x00` | only `0x70` is defined |
| A streaming read/write command is expected | there is no such command | use `0x04`/`0x05` or bulk `0x1x` |
| A stalled SPI module | read bytes are `0xFF`; the reply length is unchanged | framing, not an error status |
| A read count is large | the data streams; there is no buffer bound | the host must read the promised bytes |
| The binary layer is expected to honour the terminal settings | it starts from its own defaults | configure speed and options explicitly |

## Unverified and caveats

- All v3 behaviour in this set is **not verified**.
- Whether an in-place clock tweak in the middle of a byte transfer is safe for
  a device that is already mid-transaction is **not verified**.
- The sniffer's maximum usable rate is not specified; an overflow-free
  capture is not a guarantee of lossless capture at an arbitrary rate.
- The conventional CPOL/CPHA table in [entry-and-setup.md](entry-and-setup.md)
  is a naming aid; the target datasheet is the authority.
