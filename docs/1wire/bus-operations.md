# 1-Wire — bus operations

Part of the 1-Wire documentation set (`docs/1wire/`).  This page owns the bus
reset, device detection, the error codes, and the byte/bit operations.

## Scope

On-the-bus behaviour in 1-WIRE mode.  Shared prompt grammar — value entry,
repetition `:N`, width `;N`, the wrappers, `(n)` macros — is owned by
`docs/generic/` and only the 1-Wire effect is described here.

## Bus reset and device detection

A 1-Wire transaction starts with a **bus reset**: the Bus Pirate pulls the
data line low, releases it, and listens for the presence pulse that a device
sends back.  In this mode the reset is issued by `[` or `{` — both do the same
thing, and there is no separate stop (`]`/`}` have no bus effect here).  The
reset is also the device-detection mechanism: a device that is present pulls
the line low during the presence window.

| Input | Action |
|---|---|
| `[` | bus reset (then send/read as usual) |
| `{` | bus reset (start-with-read form; same effect) |
| `]` / `}` | no bus effect in 1-WIRE |

### Reset results

| Result | Firmware output |
|---|---|
| device present | `BUS RESET  OK` |
| no device answered | `BUS RESET Warning: *No device detected` |
| line held low (short, or no working pull-up) | `BUS RESET Warning: *Short or no pull-up` |

The failure strings are the firmware's own messages; the 1-Wire failure
path is unverified.

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
| `:N` | repeat the preceding value/command N times |

Writes echo `WRITE: <value>`; reads echo `READ: <value>`.  Values are printed
in the current display format (HEX by default).  The value entry, `:N` and
`;N` grammar is in [commands.md](../generic/commands.md).

A reset and the first byte are often sent as one line — for example `{0x3C` is
a reset immediately followed by the Overdrive Skip ROM byte, which is how a
device is put into overdrive (see [entry-and-speed.md](entry-and-speed.md)).

## Bit-level operations

| Input | 1-Wire effect |
|---|---|
| `-` | set the data state to 1 for the next `^` (no bus change) |
| `_` | set the data state to 0 for the next `^` (no bus change) |
| `.` | read the stored data state (no bus change) |
| `^` / `^:N` | clock N time slots using the stored data state |
| `!` / `!:N` | read N bits with clock |
| `/` | prints `CLOCK, 1`; **no bus effect** in 1-WIRE |
| `\` | prints `CLOCK, 0`; **no bus effect** in 1-WIRE |

The labels come from the base layer, but 1-Wire has no discrete clock line, so
`/` and `\` have no handler in this mode and only print their label.  `^` is
the real clock: it drives the stored data state for one time slot and samples
the line, which both sends and receives a bit.

The stored-state model was changed in firmware v5.2: `-` and `_` now only set
the value used by the **next** `^`; they no longer also send a bit.  A run of
four high bits is therefore `-^^^^`, not `-^^^`.

## Worked session

Full round trip: enter, engage, detect, read the ROM, put the device into
overdrive, toggle the host speed with `O`, read again, toggle back.  Abridged
to the on-bus commands:

```
1-WIRE>W
POWER SUPPLIES ON
Clutch engaged!!!
1-WIRE>e 2
3.3V on-board pullup voltage enabled
1-WIRE>P
Pull-up resistors ON
1-WIRE>{
BUS RESET  OK
1-WIRE>(51)
BUS RESET  OK
READ ROM (0x33): 0x01 0x23 0x45 0x67 0x89 0xAB 0x0C 0x7D
Unknown device
1-WIRE>{0x3C
BUS RESET  OK
WRITE: 0x3C
1-WIRE>O
1-WIRE speed: overdrive
1-WIRE>{
BUS RESET  OK
1-WIRE>(51)
BUS RESET  OK
READ ROM (0x33): 0x01 0x23 0x45 0x67 0x89 0xAB 0x0C 0x7D
Unknown device
1-WIRE>O
1-WIRE speed: standard
1-WIRE>
```

The `Unknown device` line is the family-code lookup on the first ROM byte
(`0x01`); the lookup table is in [macros.md](macros.md).

## v3 / v4

Reset, read/write, bit ops and the error strings are the same in both builds.
The v3 path is **not verified**.