# SMPS — binary command reference

Part of the SMPS documentation set (`docs/smps/`).  Nothing here is verified
against the external board.

SMPS has no terminal mode.  It is driven through the **raw-wire** binary
command group, so the general binary layer still applies — entering `BBIO1`,
choosing the raw-wire mode, framing and leaving are owned by
[../generic/commands.md](../generic/commands.md) §Leaving the CLI and
[../binary/](../binary/).

## Entry and exit

1. Send 20 × `0x00` from a host program or script (a terminal cannot emit
   NULs; see [the README](README.md)); the Bus Pirate prints `BBIO1`.
2. Send `0x05` (raw wire); it prints `RAW1`.
3. Send SMPS requests (`0xF0`–`0xFF`) as below.
4. Send `0x00` to leave raw wire (prints `BBIO1`); from BBIO, `0x0F` returns
   to the interactive terminal.

Leaving raw wire does **not** stop the board — only `0xF1` does (see
[edge-cases.md](edge-cases.md)).

To send these bytes from a host — the raw-wire entry sequence, a minimal
request, and the read/stop bytes — see the quick-start recipe in
[README.md](README.md) §Sending it from a host.

On a build without SMPS support the group is still routed, but every request
is answered with the failure byte `0x00`.

## Command group `0b1111xxxx`

Every byte whose high nibble is `1111` (`0xF0`–`0xFF`) is an SMPS request.
Replies use the shared result bytes: `0x01` success, `0x00` failure.  A
request that reads a value returns the data instead of a result byte.

| Request | Meaning | Reply |
|---|---|---|
| `0xF0` | get the last output sample (raw counts) | 2 bytes, big-endian (high byte first); no result byte |
| `0xF1` | stop the board | `0x01` |
| `0xF2`–`0xFF` | start; one operand byte with the requested value follows (see below) | `0x01` |
| any, when the feature is not built | request refused | `0x00` |

`0xF0` and `0xF1` are fixed meanings.  Every value from `0xF2` upward is a
**start**.  There is no separate start opcode beyond the value encoding below:
the command byte itself carries the high nibble of the requested value.

## Requested-value encoding

For a start, the firmware takes the **low nibble of the command byte** as the
high nibble of a 16-bit value, then reads **one more byte** from the host as
the low byte:

```
requested = (low_nibble(command) << 8) | operand
```

So `0xF2` `0x58` requests `0x0258`; `0xF3` `0x58` requests `0x0358`; `0xFF`
`0x00` requests `0x0F00`.

Because `0xF0` and `0xF1` are already taken by get and stop, the **reachable
start values are `0x200`–`0xFFF`** (commands `0xF2`–`0xFF`).  A requested
value whose high nibble is `0` or `1` cannot be selected.  `Requires
verification`: the firmware does not define the unit of the value (volts or a
raw count).

## The PWM / ADC mechanism

As far as the firmware shows, starting the board:

1. Derives a PWM duty value from the request: `duty = 128 − (64000 /
   requested)`.  Over the reachable range this stays inside the 7-bit duty the
   firmware expects.
2. Derives the regulation target: `target = requested × 45 / 58`.
3. Routes the PWM output to the `AUX1` pin and sets the ADC to sample the
   `ADC` pin, enabling conversion-complete interrupts.
4. Sets the PWM period register to `0x7F` and enables the output-compare
   channel in edge-aligned PWM mode.  The firmware comment estimates a
   switching frequency around 125 kHz; the true frequency is `unverified`.

On every ADC conversion the firmware stores the raw sample as the last
reading and applies **bang-bang** regulation: if the sample is greater than
the target it forces the duty to 0, otherwise it re-applies the computed duty.
The last stored sample is what `0xF0` reports.

Stopping turns the ADC off, disables its interrupt and auto-sampling, sets the
duty and period to 0, disables the output-compare channel, and frees the
`AUX1` pin.  Stopping does **not** clear the last stored sample.

The ADC is a 10-bit converter; the values here are raw counts, not volts.

## Worked exchange

A format illustration (no board); bytes are shown in hex.

```
host -> 0x00 x20        enter binary bitbang
host <- "BBIO1"
host -> 0x05            select raw wire
host <- "RAW1"
host -> 0xF2 0x58       start; requested value 0x0258
host <- 0x01            success
host -> 0xF0            read the last output sample (raw counts)
host <- 0xNN 0xNN       big-endian 16-bit sample (high byte first)
host -> 0xF1            stop
host <- 0x01            success
host -> 0x00            leave raw wire
host <- "BBIO1"
```

## v3 / v4

SMPS is **v4 only**.  Enabling the feature on a v3 build is a compile-time
error, and a default v4 image has it switched off.  The command group is
routed on all builds but answers `0x00` when the feature is not built in.

## Limits

- The command group and the value encoding are **unverified**; so is the
  electrical result.
- `0xF0` on a feature-disabled build returns a single `0x00` failure byte, not
  the two-byte reading.
- There is no reply that proves the PWM is actually running — `0x01` only
  means the request was accepted.