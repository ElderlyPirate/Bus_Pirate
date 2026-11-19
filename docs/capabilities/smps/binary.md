# SMPS - binary command reference

Part of the SMPS set (`docs/capabilities/smps/`).  This page owns the command
group, the requested-value encoding, the reply shapes and the worked
exchanges.  Entry, mode selection, the shared result bytes and leaving are
owned by [the binary layer](../../protocol/binary/README.md); `RAW1` framing is
owned by [the shared raw-wire page](../../protocol/wire/binary.md).  The
electrical result is `unverified`, and the firmware defines no unit for the
requested value.

## Entry and exit

1. Send 20 x `0x00` from a host program or script; the Bus Pirate prints
   `BBIO1`.  A terminal cannot send NULs.
2. Send `0x05` (raw wire); it prints `RAW1`.
3. Send SMPS requests (`0xF0`-`0xFF`) as below.
4. Send `0x00` to leave raw wire (prints `BBIO1`); from `BBIO1`, `0x0F`
   returns to the terminal with `0x01`, the banner and the `HiZ>` prompt.

Notes:

- Entering `RAW1` does **not** start the SMPS.  Until a start is accepted the
  loop is idle, and `0xF0` reports the stored sample (zero after power-up).
- `0x0F` is a `BBIO1` request.  In `RAW1` it is not a return; leave with
  `0x00` first ([exit.md](exit.md)).
- The group exists only in `RAW1`.  In `BBIO1`, every `0xFx` byte is a
  pin-state byte, not an SMPS request.
- The mode is shared with 2-Wire and 3-Wire.  The wire count does not change
  the SMPS group.

## Command group `0b1111xxxx`

Every byte whose high nibble is `1111` (`0xF0`-`0xFF`) is routed to the group.
`0xF0` and `0xF1` are fixed requests; every byte from `0xF2` upward is a
**start**.  There is no separate start opcode: the command byte itself carries
the top nibble of the requested value.

| Request | Meaning | Operand | Reply |
|---|---|---|---|
| `0xF0` | read the last feedback sample (raw counts) | none | 2 bytes, high byte first; no result byte |
| `0xF1` | stop the SMPS and park `AUX1` | none | `0x01` |
| `0xF2`-`0xFF` | start with a requested value | 1 byte | `0x01` |
| any, when the feature is not built | refused | a start still consumes one byte | `0x00` |

The sample is the ADC-node count, not a calibrated output voltage.  Replies use
the shared result bytes: `0x01` success, `0x00` failure.  A request that reads
a value returns the data instead of a result byte.  There is no frame marker,
length byte or checksum; the host must know each reply shape to keep the
stream aligned.

## Requested-value encoding

For a start, the firmware takes the **low nibble of the command byte** as the
top nibble of a 12-bit value (stored in 16 bits), then reads **one more byte**
from the host as the low byte:

```
requested = (low_nibble(command) << 8) | operand
```

| Host sends | Requested value |
|---|---|
| `0xF2` `0x00` | `0x200` |
| `0xF2` `0x58` | `0x258` |
| `0xF3` `0x58` | `0x358` |
| `0xFF` `0x00` | `0xF00` |
| `0xFF` `0xFF` | `0xFFF` |

Because `0xF0` and `0xF1` are already taken by read and stop, the reachable
start values are **`0x200`-`0xFFF`**.  The values `0x000`-`0x1FF` cannot be
requested.  Every reachable value is accepted, including values above the range
where the firmware cutoff can act; the firmware does not validate or clamp the
request.  The operand may be any byte.  The unit of the value (volts, a board
count or something else) is not defined by the firmware (`unverified`).

## Reply shapes

| Request | Reply | Shape | Notes |
|---|---|---|---|
| `0xF0` | the stored feedback sample | exactly 2 bytes, high byte first | 10-bit count, so a valid high byte is `0x00`-`0x03`; no acknowledgement or error byte |
| `0xF1` | `0x01` | 1 byte | repeated stops, and a stop before any start, also reply `0x01` |
| start | `0x01` | 1 byte | after the operand is consumed and the new setup is applied |
| `0xF0` before any start | `0x00` `0x00` | 2 bytes | no sample stored since power-up |
| `0xF0` after stop | the last sample from the run | 2 bytes | stop does not clear the stored sample |
| `0xF0` after a terminal reset, before the next start | the previous run's sample | 2 bytes | the reset stops the loop but does not clear the stored sample |
| `0xF0` after a new start | possibly the previous run's sample | 2 bytes | no freshness flag; discard the first read or poll until it changes |
| any, feature not built | `0x00` | 1 byte | for a start, the operand is still consumed first |

There is no readback of the requested value, the duty cycle or whether the
loop is running.  `0x01` only means the request was accepted.

## Worked exchanges

### A complete session

Bytes are shown in hex.  The sample value is an example, not a measurement.

```
host -> 0x00 x 20        enter binary bitbang
host <- "BBIO1"
host -> 0x05             select raw wire
host <- "RAW1"
host -> 0xF2 0x00        start; lowest reachable request 0x200
host <- 0x01             accepted
host -> 0xF0             first read; discard it
host <- 0x00 0x00        example: the pre-start value, not a new sample
host -> 0xF0             read again after at least one conversion
host <- 0x01 0xD3        example feedback sample: 467 counts
host -> 0xF1             stop
host <- 0x01             accepted
host -> 0x00             leave raw wire
host <- "BBIO1"
host -> 0x0F             return to the terminal
host <- 0x01
                         version banner and HiZ> follow
```

The first `0xF0` after a start can return zero, the previous run's sample, or
an already-fresh value.  There is no freshness flag; treat that first read as
stale and use a later read.

### Read on a fresh power-up, and after stop

```
host -> 0xF0
host <- 0x00 0x00        no sample stored since power-up
host -> 0xF2 0x00        start
host <- 0x01
host -> 0xF1             stop
host <- 0x01
host -> 0xF0
host <- 0x01 0xD3        the last sample from the stopped run
```

### A build without SMPS support

The group is still routed.  `0xF2 <operand>` replies `0x00`, consumes the
operand, and **stays in `RAW1`**.  The next byte is the next `RAW1` request:

```
host -> 0xF2 0x58        start on a build without SMPS
host <- 0x00             refused; 0x58 was consumed as the operand
host -> 0x01             next request
host <- "RAW1"           still in raw wire
```

The operand is consumed even when it is `0x00`; a zero operand is not an exit
request on this path.  `0xF2 0x00` replies `0x00`, consumes the `0x00`, and
the layer is still in `RAW1`.

`0xF0` on a build without SMPS returns a **single** `0x00`, not the two-byte
sample.  A host that reads two bytes will consume the next reply as the low
byte and lose alignment.

### A host sequence

A minimal Python host, assuming the Bus Pirate appears as `COM3`.  Adjust the
port name; the v4 USB CDC link does not use the baud rate.  Start from an idle
terminal prompt and drain any pending output before the NULs.

```python
import serial

port = serial.Serial("COM3", 115200, timeout=2)

def expect(count):
    data = port.read(count)
    if len(data) != count:
        raise TimeoutError(data)
    return data

port.write(b"\x00" * 20)
expect(5)                        # BBIO1
port.write(b"\x05")
expect(4)                        # RAW1
port.write(bytes((0xF2, 0x00)))  # start, lowest request 0x200
assert expect(1) == b"\x01"
port.write(b"\xF0")              # first read: discard it
expect(2)
port.write(b"\xF0")              # read after a conversion
sample = int.from_bytes(expect(2), "big")
assert 0 <= sample <= 1023
port.write(b"\xF1")              # stop
assert expect(1) == b"\x01"
port.write(b"\x00")              # leave raw wire
expect(5)                        # BBIO1
port.write(b"\x0F")              # return to the terminal
assert expect(1) == b"\x01"      # banner and HiZ> follow
```

## Starting again while running

A new start (`0xF2`-`0xFF` plus its operand) is accepted while the loop runs.
The start routine re-runs in place: it disables the old conversion and PWM,
recomputes the duty and target, re-routes `AUX1` and re-enables the feedback.
No stop is needed and no stop happens.  The stored sample is not cleared, so a
`0xF0` read immediately after the new start can still return the previous
sample; the freshness rule above applies.  Stop details are in
[exit.md](exit.md).

## No SMPS support

The group is routed on every build, but the handler refuses every request when
SMPS support is not compiled in.  That covers every v3 image and the default
v4 image.  The reply table is in the command group section above; the
alignment rules are:

| Situation | Result |
|---|---|
| `0xF0` | one `0x00`; not a sample |
| `0xF1` | `0x00`; no stop happens |
| `0xF2`-`0xFF` | `0x00`; one operand byte is still consumed |
| if the layer is unclear | in `RAW1`, send `0x01` to reprint `RAW1`; from `BBIO1`, send `0x05` first |

The operand is consumed on the disabled path so that the following request
stays aligned.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| SMPS build | not available; enabling is a compile-time error | optional; off in the default image |
| command group | routed; answers `0x00` (not verified on v3) | routed; active when built in |
| `AUX1` target | absent from the v3 firmware | present |
| terminal mode | none | none |

The request framing and encoding above are the same on both builds; only
whether the feature is active differs.  There is no v3 hardware path to
verify, because the build cannot enable the feature.

## Limits

- The request and the sample are raw numbers; the firmware never converts them
  to volts.  The sample is the feedback-node count, not a calibrated output
  voltage.
- The firmware cutoff can act only for requests `0x200`-`0x526`; whether the
  add-on can reach the target is unknown.  No request is a promise of a
  regulated output.  See [feedback-loop.md](feedback-loop.md).
- `0x01` only means the request was accepted.  There is no proof that the PWM
  reaches the add-on, that the add-on is present, or that its output is live.
- `0xF0` never triggers a conversion and never clears the stored sample.  The
  first read after a start can be stale; discard it or poll until it changes.
- The loop has no feedback presence test.  With the feedback wire open, the
  duty follows the floating reading and the output is uncontrolled; a zero
  sample is not proof of 0 V output
  ([edge-cases.md](edge-cases.md#no-add-on-or-no-feedback-connected)).
- Leaving raw wire with `0x00` does not stop the loop; only `0xF1` in `RAW1`
  or a v4 terminal reset does ([exit.md](exit.md)).
