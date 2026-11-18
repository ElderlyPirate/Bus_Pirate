# I2C — edge cases & failure modes

Part of the I2C documentation set (`docs/protocol/i2c/`). Each entry is a
concrete trigger, the observable symptom, the behaviour, and the recovery. The
main topics are in [entry-and-setup.md](entry-and-setup.md),
[wiring.md](wiring.md), [bus-operations.md](bus-operations.md),
[clock-stretching.md](clock-stretching.md), [macros.md](macros.md),
[sniffer.md](sniffer.md), [eeprom.md](eeprom.md) and [binary.md](binary.md).

## Wiring, pull-ups and supplies

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| No pull-up on SDA/SCL | the scanner reports a device at every address | fit 2–10 kΩ to Vpullup |
| A line is held low | `Warning: *Short or no pull-up`; no scan | clear the short or the target's hold |
| `P` with no voltage on Vpullup | `Warning: no voltage on Vpullup pin`; no idle high | `e2`/`e3` on v4, or feed Vpu externally |
| `e` with a voltage already on Vpullup | `Warning: already a voltage on Vpullup pin` plus a syntax error | remove the external source |
| VPU is 5 V on a 3.3 V target | the bus high level is 5 V | use `e2` or feed 3.3 V externally |
| `W` is skipped | the target has no supply; the bus can still be driven | run `W` unless the target is self-powered |
| `W` prints `VREG too low, is there a short?` | `POWER SUPPLIES OFF` follows | clear the short, then `W` again |
| A mode change or reset follows `W`/`P`/`e` | the supplies, pull-ups and VPU source are off again | re-issue the bring-up commands |
| The on-board pull-ups are expected to pull to the target rail | they pull to whatever is on Vpullup | feed VPU at the target's logic level |
| More than 5 V is applied to a line | damage risk | the interface is 5 V class; stay at or below 5 V |
| MISO or CS is jumpered | the terminal mode does not use them | leave them unconnected unless another function needs them |

## Entry and settings

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `m4 2 4` is typed | the driver and speed prompts run | the engine has three rates; use 1–3 |
| A settings value is missing or out of range | the prompts run; no error is printed | answer the prompts, or type valid numbers |
| The printed header is typed after `m4` | the prompts run | type menu numbers; at the `I2C>` prompt the header is a syntax error |
| The header shows `mod 1` after typing `2` | the header is zero-based | `1` in the header is Hardware, typed as `2`; Software is `0`, typed as `1` |
| The header shows `spd 0` after typing `1` | the header is zero-based | type the menu number; read the index |
| `m4` is used on a default v3 build | only the speed prompt appears | the default v3 build has no hardware engine |
| A v4 line (`m4 2 1`) is used on v3 | speed 2 is selected; the trailing digit is ignored | use `m4 <speed>` on v3 |
| `e` is typed on v3 | syntax error | v3 has no `e`; feed Vpullup externally |
| An output-type prompt is expected | it never appears | I2C is fixed open drain; the pull-up is mandatory |
| The `i` status header disagrees with the menu | `spd` and `mod` are printed zero-based | read the field mapping in [entry-and-setup.md](entry-and-setup.md#the-settings-header) |
| A v3 PIC is revision A3 or earlier with the optional hardware build | `WARNING: HARDWARE I2C is broken on this PIC! (REV A3)` | use Software |

## Bus operations and ACK handling

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Two reads in a row | `READ:  ACK <value>` | the ACK belongs to the first byte |
| A write after a read | `WRITE: <value> ACK …` | the first ACK is the read's; the next is the slave's |
| A start or stop after a read | `NACK` before the condition | the pending bit is NACKed |
| A read is left unresolved | no warning; the bus is held | send `]` or `[` to resolve it |
| `r:N` is used | one `READ: ` header; the first value has no ACK prefix | the ACK appears before bytes 2..N |
| A repeated write (`0x55:3`) | all replies on one line | expected; the repeat prints one line |
| A value above `0xFF` is written | the sent byte is the low 8 bits | send multiple bytes; change the width only for a bit-level transfer |
| `;N` is set to anything other than 8 | software: N clocked bits; hardware: 8 clocked bits, wide display | leave the width at 8 in I2C |
| The scanner is run with a width other than 8 | no hits; the addresses are N bits wide | send `;8` first |
| A bit-level command is typed | prefix + `ERROR: command has no effect here` + syntax error | I2C has no bit-level handler |
| `{` or `}` is used | it behaves exactly like `[` or `]` | I2C has no read-back flag |
| A second `[` is typed | `I2C START BIT` again, with no stop between | it is a repeated start; the transaction stays open |
| A write is NACKed | `WRITE: … NACK` | the slave refused the byte; check the address and device state |
| A slave NACKs its address | `WRITE: 0xNN NACK` | no such device, wrong address, or the device is busy |
| `]` is typed with no transaction open | `I2C STOP BIT` | a stop on an idle bus is harmless |

## Clock stretching and bus errors

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A slave holds SCL past the bound | `I2C bus error`; both pins released | release the slave or remove the short, then retry |
| A read times out | `READ: I2C bus error` then `0xFF` | the `0xFF` is not data; retry |
| A write times out | the error is appended to the `WRITE:` line | read the error text, not the columns |
| A timeout occurs in an ACK slot | `I2C bus error`, not `NACK` | the NACK text does not appear |
| The next command is sent after a timeout | it starts fresh; it does not retry the abandoned command, and times out again if SCL is still held | clear the cause first |
| The hardware engine aborts | module disabled, pins released, pending ACK dropped | a new START re-initialises; SDA held triggers the nine-clock clear |
| SDA is held after the nine clocks | the new START fails too | the slave or the short must release the line |
| `0x0A` is read after a fault | `0x00` | the next real bus operation clears it |
| A normal NACK is followed by `0x0A` | `0x01` | a NACK is not a bus fault |
| A bulk write faults mid-frame | the rest of the payload is NACKed without bus activity | read `0x0A`, then restart the frame |
| `0x04` returns `0xFF` | a real byte can also be `0xFF` | use `0x0A` to tell them apart |
| A pending read bit is left open | SCL is held between the byte and its ACK | resolve with a start or stop before rewiring |
| `W` is issued with a read ACK pending | the driver is re-initialised; the pending bit stays set | finish the read with `]` first |

## Scanner

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| No pull-up | every address is reported | fit the pull-up; the floating line reads as ACK |
| A line is low before the scan | header then `Warning: *Short or no pull-up`; no scan | clear the short and retry |
| The bus is empty | the header prints; no addresses follow | expected |
| A read address ACKs | the scanner reads one byte and NACKs it | expected; it ends the transaction cleanly |
| The display format is decimal | `160(80 W) 161(80 R)` | the scanner follows `o` |
| A read ACK is pending | `NACK`, `I2C STOP BIT`, then the scan | the scanner finishes the read first |
| `(3)` was run | the scan covers the internal EEPROM bus | that is the redirect working |
| The scan is slow | normal; it probes 256 addresses | use a higher profile if the bus allows |

## Sniffer

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A key is pressed to stop | the key is consumed; the next characters are commands | expected |
| The sniffer starts mid-transaction | no start marker until the next start or repeated start | start the sniffer before the traffic |
| The host stops reading on v4 | the sniffer stalls; edges are missed | keep the host draining |
| The v3 output ring fills | bytes are dropped silently; the MODE LED turns off | shorten the burst or use a logic analyzer |
| The display format is not hex | the sniffer still prints hex | expected; the sniffer ignores `o` |
| `(3)` was run | the sniffer shows external traffic, not the on-board bus | the sniffer is external-only |
| A register-level decode is expected | only bytes and ACK markers are shown | decode in the host |
| A fast bus is captured | edges may be missing | the maximum rate is unverified |
| The bus has no pull-up | the lines float and the capture is noise | fit the bus pull-up |

## Macros and the on-board EEPROM

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `(3)` in Software | `Onboard EEPROM requires hardware I2C` plus a syntax error | re-enter with `m4 2 <speed>` |
| `(4)` before `(3)` | nothing | run `(3)` first |
| The mode is left and re-entered after `(3)` | the external bus again | the redirect is per session |
| `(3)`/`(4)` on v3 | `Unknown macro, try ? or (0) for help` | v4 only |
| `(5)` or a larger macro | the unknown-macro message | macros are 0–4 (0–2 on v3) |
| A read ACK is pending before `(1)`/`(2)` | `NACK`, `I2C STOP BIT`, then the macro | the macro finishes the read first |
| `(4)` is run, then a mode change | the write protect is on again | repeat `(3)` and `(4)` |
| A write is followed by an immediate read | the address NACKs or old data is read | poll `0xA0` until ACK, then set the pointer |
| A write burst crosses a 32-byte page | the data wraps in the page | split the burst |
| The on-board part is expected on the external pins | it is not visible | use `(3)` to reach the internal bus |

## Binary

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `0x0F` is sent inside `I2C1` | the sniffer starts, not an exit | send `0x00` to `BBIO1`, then `0x0F` |
| `0x00` is sent mid-transaction | the pins are released without a stop | send `0x03` first |
| `0x0F` is sent mid-transaction | the pins are released for sniffing without a stop | send `0x03` first |
| `0x08` has a zero write count | `0x00`; no payload read, no bus traffic | send at least the address byte |
| `0x08` has a count above 4096 | `0x00`; no payload read, no bus traffic | split the transfer; 4096 is the limit on both builds |
| `0x08` is refused | the two count words were consumed; `0x0A` is unchanged | do not send the payload; the next byte is read as a new command |
| `0x08` payload is short | the command stalls waiting for bytes | send exactly the write count |
| `0x08` read count 0 | only `0x01` is returned; no data | expected; set the read count |
| `0x08` write count 1, read count > 0 | no repeated start and no read address is sent | send the read address as the single byte |
| `0x08` slave NACK | stop sent; `0x00` | start a new transaction |
| `0x08` bus timeout | pins released; no stop; `0x0A` = `0x00` | start fresh |
| `0x1x` per-byte reply | `0x00` = ACK, `0x01` = NACK | opposite sense to the command result |
| `0x1x` NACK mid-frame | the remaining bytes are still sent | stop and restart if the slave ignores them |
| `0x1x` fault mid-frame | the rest is NACKed; no bus activity | read `0x0A`, then restart |
| `0x09` unknown sub-command | `0x01 0x00` | the sub-result is zero; the `0x01` only means the command was accepted |
| `0x09 0x03` reads AUX | the pin is made an input first | re-drive it if needed |
| `0x5x` on v3 | `0x00` | the group is v4 only |
| `0x6x` is expected to select the engine | only the software profile changes | binary is software only |
| `0x6x` operand is `0x64`–`0x6F` | a profile is selected, not a failure | the operand is masked to two bits |
| `0x7x` low nibble is not 0 | `0x00` | only `0x70` is defined |
| `0x4x` turns the supply on | `0x01` even if the rail is shorted | there is no rail check; verify separately |
| A terminal `;N` is expected to apply | binary transfers stay 8 bits | the binary path forces 8 bits |
| The terminal speed is expected at entry | the binary entry uses `Maximum speed` | send `0x6x` if you need a slower profile |
| The terminal supply state is expected at entry | the supply and pull-ups are off | send `0x4x` / `0x5x` |
| `0x0A` is polled repeatedly | the value does not change | only the next bus operation clears it |
| A NACK is treated as a fault | `0x0A` stays `0x01` | a NACK is a normal reply |
