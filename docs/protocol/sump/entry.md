# SUMP — entry and exit

Part of the SUMP set (`docs/protocol/sump/`).  This page owns the hidden entry
sequence, the boundary with the binary layer, what entry changes, and the exit
back to the terminal.  The host side is in [client.md](client.md); the command
bytes are in [commands.md](commands.md).

## The entry contract

The analyzer is not on the mode menu.  A host enters it by sending, on the
serial port, **five to nineteen `0x00` (NUL) bytes immediately followed by one
raw `0x02` (STX) byte**.  This is exactly the port initialisation a SUMP/OLS
client already performs: five reset bytes, then the `0x02` identification
request.

The terminal counts NULs as they arrive.  The count survives further NULs but
any other byte cancels it.  A `0x02` that arrives with fewer than five NULs is
handled as a left-arrow and clears the count at the same time.  When the count
is five or more, the `0x02` is not treated as a cursor key: it enters SUMP.  The
device then sends its four-byte identification, `1ALS`, and starts the analyzer
loop.  There is no prompt while SUMP is running.

| Sequence from the terminal | Result |
|---|---|
| fewer than five NULs, then `0x02` | left-arrow: the cursor moves left, or the board beeps at the start of the line |
| five to nineteen NULs, then `0x02` | SUMP is entered; `1ALS` is sent |
| twenty NULs | the binary layer is entered at the twentieth; `BBIO1` is sent |
| five to nineteen NULs, then any other byte | no entry; the count is cancelled and the byte is handled by the terminal |

### Why five and why twenty

Five is the SUMP reset count: a host sends five `0x00` bytes when the receiver
state is unknown, because a stalled five-byte command can still be waiting for
four parameters.  The `0x02` that follows is the ordinary SUMP identification
request.  The terminal reuses that initialisation as its hidden entry, which is
why a client does not need a Bus Pirate-specific open sequence.

Nineteen is the largest prefix that still reaches the `0x02`.  At twenty, the
NUL that would complete the prefix has already switched the port to the binary
layer, so the `0x02` is read there.

## The boundary rule

- Five to nineteen NULs, then the raw `0x02` — SUMP.
- Twenty NULs — `BBIO1`, never SUMP.

A host that sends its reset run as twenty NULs gets `BBIO1` and, if it follows
with `0x02`, the binary I2C mode.  Nothing is damaged, but no capture starts.
Leave the binary layer, then send the five-to-nineteen-NUL sequence again:

```
host -> 00 x20                  twenty NULs: binary entry, not SUMP
host <- "BBIO1"
host -> 02                      binary I2C entry
host <- "I2C1"
host -> 00                      leave I2C
host <- "BBIO1"
host -> 0f                      binary return
host <- 01                      acknowledged
host <- <version banner>
host <- "HiZ>"
```

`0x0F` inside a binary sub-mode is not the return; send `0x00` first, then
`0x0F` from `BBIO1`.  On v3 that return performs a hardware reset, so discard
stray bytes until the banner.

The `0x02` means "enter SUMP" only at the terminal.  Inside `BBIO1` or a binary
sub-mode it is that layer's command; return to the terminal first.  The entry
count exists only in the terminal input loop, so binary and SUMP sessions do
not see it.

## What entry changes

| Item | After entry |
|---|---|
| probe pins | all inputs; the internal weak pull-ups on them are off |
| edge detection | off; enabled only while a capture waits for its trigger |
| sampling timer | stopped |
| MODE LED | off |
| analyzer state | reset: 4096-sample read count, 1601-tick default timer period (about 9.994 kHz), no trigger |
| identification | `1ALS` sent immediately, once |

Entry does **not** change the target supplies (`3.3V`, `5.0V`) or the `P`
pull-up array left on by the previous terminal session; the exit reset turns
those off.  If a previous session left a rail or the pull-ups on, turn them off
before connecting a target.

The pins are inputs, so the target must drive them.  Do not exceed 5.5 V on any
probe pin (5 V safe).  A floating probe pin is not "low": it is an undefined
input and can produce edges, which matters when it is in the trigger mask.

## Wiring

| Signal | v3 header | v4 header | Direction | Notes |
|---|---|---|---|---|
| AUX2 | — | `AUX2` | input | v4 only; sample bit 0 |
| AUX1 | — | `AUX1` | input | v4 only; sample bit 7 |
| AUX | `AUX` | `AUX` | input | sample bit 4 on v3, bit 5 on v4 |
| CS | `CS` | `CS` | input | |
| MISO | `MISO` | `MISO` | input | |
| CLK | `CLK` | `CLK` | input | |
| MOSI | `MOSI` | `MOSI` | input | |
| GND | `GND` | `GND` | ground | common ground with the target required |

**No pull-up is required and no supply is required from the Bus Pirate.**  The
probe pins are inputs; the internal weak pull-ups on them are off, and the
target drives every line.  The interface is 5 V safe, 5.5 V maximum.

The v4 header is labelled
`GND 5.0V 3.3V VPU ADC AUX2 AUX1 AUX CS MISO CLK MOSI` (pin 1 is `MOSI`); the
v3 header carries the same signal names without `AUX2` and `AUX1`.  Connect by
the header label.  See [generic usage.md](../../generic/usage.md#v-in-hiz) for
the `v` pin table.

## Leaving

SUMP has no exit command in its own protocol, so the firmware also accepts
three exit bytes at the command level:

| Byte | Result |
|---|---|
| `0x0F` | leave SUMP for the terminal |
| `#` (`0x23`) | leave SUMP for the terminal |
| `$` (`0x24`) | leave SUMP for the terminal; the bootloader is **not** entered |
| `0x00` | reset the analyzer and stay in SUMP |

All three exit bytes stop the analyzer, park the board in `HiZ`, print the
version banner and the `HiZ>` prompt, and run the terminal reset (pins to
inputs, target supplies and pull-ups off, display back to HEX).  On both boards
the SUMP exit is this software reset; it is not the v3 binary `0x0F` hardware
reset.  The prompt is printed once, immediately after the banner.

An exit byte sent while the analyzer is at the command parser is a command.
A byte sent while the sampler waits for a trigger, or while a timer-paced
capture runs, aborts the capture and is then handled as a command; `#` both
stops the wait and leaves SUMP.  Inside a five-byte command an exit byte is a
parameter: the command completes and **takes effect** (unless its opcode is
unknown).  Use the one-second gap or five `0x00` resets to discard a partial
command.  A fast capture's short post-trigger window is the exception; see
[readout.md](readout.md#aborts-and-bytes-during-a-reply).

A terminal user can leave by typing `#`; the client must be closed first,
because only one program can own the port:

```
(no prompt)                     SUMP is running
#                               typed; the first byte leaves SUMP
<version banner>
HiZ>                            the terminal is back
                                the Enter after # gives a blank line and a
                                second prompt
HiZ>
```

The second prompt is normal: the `#` has already left SUMP, so the carriage
return that follows is handled by the terminal as an empty command.

## Recovery

| Situation | Recovery |
|---|---|
| no prompt, board seems deaf | SUMP has no prompt; send `0x0F`, `#` or `$` |
| a client closed the port without exiting | open a terminal on the port and send `#` |
| the board answered `BBIO1` | it is in the binary layer; send `0x0F` to return to the terminal, then retry with five to nineteen NULs |
| the board answered a sub-mode identifier (`I2C1`, `SPI1`, …) | send `0x00`, read `BBIO1`, then send `0x0F`; then retry with five to nineteen NULs ([why](../binary/edge-cases.md#0x0f-inside-a-mode-does-not-return-to-the-terminal)) |
| a script sent five NULs and a stray `0x02` | the board is in SUMP; send an exit byte |
| the board is unresponsive to everything | power-cycle it |

Whether a particular client sends exactly five reset bytes, and the exact
driver label in a given sigrok/PulseView release, are **unverified**.  The
firmware contract is the five-to-nineteen-NUL range above; the binary layer
begins at the next NUL.
