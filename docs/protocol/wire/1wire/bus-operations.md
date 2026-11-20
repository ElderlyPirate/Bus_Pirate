# 1-Wire — bus operations

Part of the [1-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page owns the bus reset, device
detection, the error codes, and the byte/bit operations.

## Scope

On-the-bus behaviour in `1WIRE` mode.  Shared prompt grammar — value entry,
repetition `:N`, width `;N`, the wrappers, `(n)` macros — is owned by
[docs/generic/](../../../generic/README.md) and only the 1-Wire effect is
described here.

## Bus reset and device detection

A 1-Wire transaction starts with a **bus reset**: the Bus Pirate pulls the data
line low, releases it, and listens for the presence pulse that a device sends
back.  In this mode the reset is issued by `[` or `{` — both do the same
thing.
There is no stop handler: `]` and `}` are **refused**.

| Input | Action |
|---|---|
| `[` | bus reset, then continue |
| `{` | bus reset (start-with-read form; same effect) |
| `]` | refused: `ERROR: command has no effect here` + syntax error |
| `}` | refused: same as `]` |

The reset is also the device-detection mechanism: a device that is present
pulls the line low during the presence window.

### Reset results

The success line has **two spaces** between `RESET` and `OK`; the failure
lines share the prefix `BUS RESET Warning: `.

| Result | Firmware output |
|---|---|
| device present | `BUS RESET  OK` |
| no device answered (line high) | `BUS RESET Warning: *No device detected` |
| line held low (short, or no working pull-up) | `BUS RESET Warning: *Short or no pull-up` |

The firmware checks the line twice: first at the presence sample, then again
after the reset recovery.  A line that reads high at the first check reports
`*No device detected`; if it is then still low at the second check, the
`*Short or no pull-up` result replaces it.  The failure strings are the
firmware's own messages; the 1-Wire failure path is unverified.

How to tell them apart:

- **`*No device detected`** — nothing pulled the line low during the presence
  window.  Nothing is on the bus, the pull-up voltage is missing, or the
  device is unpowered.
- **`*Short or no pull-up`** — the line was still low when it should have been
  released to the pull-up.  The pull-up is missing or too weak, the data line
  is shorted to ground, or a device is holding the line.

### Recovery

- No device: check the pull-up voltage (`P` should not warn), check the target
  is powered and on the same ground, then retry the reset.
- Short or no pull-up: supply the VPU pin (`e2`/`e3` on v4, externally on v3),
  check for a solder bridge or a device stuck low, then retry.

## Reading and writing

1-Wire has no separate clock line; each byte is sent as eight time slots, and
the same slot timing is used to send and to receive.  The Bus Pirate therefore
uses one write path for both.

| Input | Action |
|---|---|
| `r` | read one byte (the host clocks `0xFF` and samples the line) |
| `r:N` | read N bytes |
| `0xNN` | write one hex byte (`0x3C`, `0xA`) |
| `0b1010` | write one binary byte (partial widths allowed) |
| `123` | write one decimal byte |
| `"text"` | write the literal bytes of the string |
| `:N` | repeat the preceding value/command N times |

Writes echo `WRITE: <value>`; reads echo `READ: <value>`.  Values are printed
in the current display format (HEX by default).  The value entry, `:N` and
`;N` grammar is in [values.md](../../../generic/values.md).

**The width setting does not change 1-Wire transfers.**  A read or write is
always eight time slots; `;N` only changes how the echoed or read number is
formatted.  `r;16`, for example, still reads one byte but prints it as a
16-bit value.

A reset and the first byte are often sent as one line — for example `{0x3C` is
a reset immediately followed by the Overdrive Skip ROM byte, which is how a
device is put into overdrive (see [entry-and-speed.md](entry-and-speed.md)).

### A plain byte exchange

A reset, a ROM command and one read, with no macro involved:

```
1WIRE>{
BUS RESET  OK
1WIRE>0x33
WRITE: 0x33
1WIRE>r
READ: 0x28
1WIRE>
```

`0x33` is READ ROM; `0x28` is the DS18B20 family code, and the value is an
example from a single-device bus.  `]` and `}` are refused, so the next
`[`/`{` is the transaction boundary.

## Bit-level operations

| Input | 1-Wire effect | Firmware output |
|---|---|---|
| `-` | set the data state to 1 for the next `^` (no bus change) | `DATA OUTPUT, 1` then ` *next clock (^) will use this value` |
| `_` | set the data state to 0 for the next `^` (no bus change) | `DATA OUTPUT, 0` then the same alert |
| `.` | read the stored data state (no bus change) | `DATA STATE: 0` or `1` |
| `^` / `^:N` | clock N time slots using the stored data state | `CLOCK TICKS: <n>` |
| `!` / `!:N` | read N bits with a clock | `READ BIT: <bits>  *pin is now HiZ` |
| `/` | no clock line: prints `CLOCK, 1` then refuses | `CLOCK, 1` + error + syntax error |
| `\` | no clock line: prints `CLOCK, 0` then refuses | `CLOCK, 0` + error + syntax error |

Notes on the outputs:

- The `-`/`_` alert string starts with a space; it is printed on its own line
  after `DATA OUTPUT, …`.
- The `^` count is printed in the current display format.  With the default
  HEX it is `CLOCK TICKS: 0x01`; with `o2` it is `CLOCK TICKS: 1`.
- `!` puts a trailing space after each bit and the suffix string begins with a
  space, so there are two spaces before `*pin`.
- `.` prints no trailing newline; the next prompt continues on the same line.
  `.` reports the **stored** value, not the bus level — use `!` to read the
  bus.
- `/` and `\` have no handler in this mode.  They print their base-layer label
  and are then refused with `ERROR: command has no effect here` and a syntax
  error.  `^` is the real clock.

`-` and `_` set the value used by the **next** `^`; they do not send a bit
themselves.  The stored state is not consumed, so a run of four high bits is
`-^^^^`, not `-^^^`.

## Worked session

Full round trip: enter, engage, source VPU, enable the pull-up, detect, read
the ROM, put the device into overdrive, toggle the host speed with `O`, read
again, toggle back, and reset to standard.  The ROM values are examples.

```
1WIRE>W
POWER SUPPLIES ON
Clutch engaged!!!
1WIRE>e 2
3.3V on-board pullup voltage enabled
1WIRE>P
Pull-up resistors ON
1WIRE>{
BUS RESET  OK
1WIRE>(51)
BUS RESET  OK
READ ROM (0x33): 0x28 0x23 0x45 0x67 0x89 0xAB 0x0C 0x7D
DS18B20 Prog Res Dig Therm
1WIRE>{0x3C
BUS RESET  OK
WRITE: 0x3C
1WIRE>O
1-WIRE speed: overdrive
1WIRE>{
BUS RESET  OK
1WIRE>(51)
BUS RESET  OK
READ ROM (0x33): 0x28 0x23 0x45 0x67 0x89 0xAB 0x0C 0x7D
DS18B20 Prog Res Dig Therm
1WIRE>O
1-WIRE speed: standard
1WIRE>{
BUS RESET  OK
1WIRE>
```

The `{0x3C` line sends a standard-speed reset and the Overdrive Skip ROM byte;
the parts switch to overdrive immediately, so `O` follows before the next
reset.  The later `O` back to standard plus the reset returns them to standard.

The family-name line is the lookup on the first ROM byte (`0x28`); the table
is in [macros.md](macros.md).
