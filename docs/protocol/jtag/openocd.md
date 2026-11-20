# JTAG — OpenOCD path (`0x06`)

Part of the JTAG documentation set.  This page is the byte contract for
binary command `0x06`, a raw JTAG TAP shifter for an OpenOCD host or any
host that speaks the same `OCD1` command set.  Entry and exit are in
[entry.md](entry.md); the TAP is in [tap.md](tap.md); clocking is in
[clock.md](clock.md); pins are in [wiring.md](wiring.md).

There is no terminal JTAG mode.  Host-side OpenOCD setup is a separate tool
set: [tools/openocd](../../tools/openocd/README.md).  OpenOCD performs the
binary entry and the `0x06` request itself; do not pre-enter the mode.

## Entry and exit

1. Enter `BBIO1` with 20 × `0x00`.
2. Send `0x06`; the path answers `OCD1` and enters its command loop.  The
   pins are high-Z inputs at this point.
3. Send `0x00` to leave; the bitbang layer resets the five BBIO pins and
   supplies, then prints `BBIO1`.  TRST keeps its last level.
4. From `BBIO1`, `0x0F` replies `0x01` and returns to the terminal (v3: a
   hardware reset).
5. Inside the mode, `0x06` re-prints `OCD1`.

The complete exit and recovery matrix is in [entry.md](entry.md#leaving).

## Commands

| Request | Followed by | Meaning | Reply |
|---|---|---|---|
| `0x00` | — | leave the path | `BBIO1` |
| `0x01` | mode byte | set the pin mode | — |
| `0x02` | feature, action | set a feature | — |
| `0x03` | — | read four ADC channels | 10 bytes |
| `0x04` | — | not a command here | `0x00` |
| `0x05` | count + TDI/TMS | TAP shift | 3-byte echo, then TDO |
| `0x06` | — | report the identifier | `OCD1` |
| `0x07` | selector, `0xAA 0x55` | set the link speed | `0x07` + selector |
| `0x08` | 2-byte delay | set the TCK delay | — |
| other | — | unrecognised | `0x00` |

`0x04` is the retired shift byte from the older protocol version; a current
host must use `0x05`.  Inside `OCD1` it is answered like any other unknown
command.  The same byte means something different in `BBIO1`:

| Byte | In `BBIO1` | In `OCD1` |
|---|---|---|
| `0x04` | enter the 1-Wire sub-mode | unknown; `0x00` |
| `0x05` | enter the raw-wire sub-mode | TAP shift |
| `0x06` | enter this OpenOCD path | report `OCD1` |
| `0x0F` | return to the terminal | unknown; `0x00` |

## Pin mode (`0x01`)

One byte follows; there is no reply.

| Mode byte | Setting |
|---|---|
| `0x00` | Hi-Z — all five lines are inputs; TDO is an input |
| `0x01` | JTAG — TDI/TMS/TCK/SRST/TRST push-pull outputs |
| `0x02` | JTAG open-drain — the same outputs are open-drain |
| other | behaves as `0x00` Hi-Z |

Entry is `0x00` Hi-Z: send `0x01 0x01` (push-pull) or `0x01 0x02`
(open-drain) before a shift.  A mode must be set before the first shift.
OpenOCD's Bus Pirate driver defaults to open-drain, so a stock setup needs
pull-ups unless push-pull is selected.  See
[tools/openocd](../../tools/openocd/README.md).

The mode byte drives the five outputs low before it applies the direction.
That is a real level change on SRST and TRST: selecting either JTAG mode
holds both lines low until a feature drives them high.  If they are wired
as active-low resets, release them after the pin mode:

| Line | Assert (usual active-low) | Release |
|---|---|---|
| TRST | `0x02 0x04 0x00` | `0x02 0x04 0x01` |
| SRST | `0x02 0x08 0x00` | `0x02 0x08 0x01` |

A later pin mode re-parks both lines low.  TDO is always an input.  A
`0x01 0x00` request makes TRST an input again, which releases it.

## Features (`0x02`)

Two bytes follow: the feature, then the action.

| Feature | Meaning | Action |
|---|---|---|
| `0x01` | mode LED | `0x00` off; any non-zero on |
| `0x02` | regulator (VREG) | `0x00` off; any non-zero on |
| `0x04` | TRST | `0x00` low; `0x01` high |
| `0x08` | SRST | `0x00` low; `0x01` high |
| `0x10` | on-board pull-ups | `0x00` off; any non-zero on |
| other | ignored | — |

For TRST and SRST the action is the **pin level**, not an "assert" flag,
and the firmware takes only its low bit: use exactly `0x00` or `0x01`.  An
even value such as `0x02` behaves as `0x00`, so it leaves the line low.
The LED, VREG and pull-up features treat any non-zero value as on.

Both reset lines are commonly active-low, so `0x00` asserts and `0x01`
releases.  The reset features only drive after `0x01` has made the pins
outputs; before that they change a latch that is not connected to the pin.

Feature `0x10` ties the lines to **Vpullup**, not to a per-line rail.  A
voltage must be present on Vpullup: feed it externally, because the v4
on-board source is not selectable from the JTAG path.  See
[wiring.md](wiring.md#pull-ups).

Feature `0x02` switches the same regulator that the XSVF entry turns on.
It drives the `3.3V` and `5.0V` rails.  Leave it off when the target has its
own supply.

## ADC read (`0x03`)

Replies with ten bytes:

| Bytes | Meaning |
|---|---|
| 1 | `0x03` — the command |
| 2 | `0x08` — eight data bytes follow |
| 3–4 | ADC/probe pin, big-endian |
| 5–6 | Vpullup |
| 7–8 | `3.3V` rail |
| 9–10 | `5.0V` rail |

The values are 10-bit ADC counts, not millivolts; the ADC set owns the
scale.  The firmware switches the ADC on for the read and off afterwards,
so read it on demand.  A rail that reads zero with VREG off is expected, not
a fault.

## TAP shift (`0x05`)

This is the only operation that moves the TAP.  The host supplies TDI and
TMS for every bit and reads TDO back.

| Part | Format |
|---|---|
| count | 16-bit big-endian, in bits |
| limit | 8192 bits per request (`0x2000`), including TAP moves |
| echo | 3 bytes: `0x05`, count high, count low |
| payload | groups of up to 16 bits, low byte first |
| reply | one TDO byte per 1–8-bit group, two per 9–16-bit group |

1. Send `0x05`, then the count high byte, then the low byte.
2. The firmware echoes the three header bytes.  The echo repeats the
   **requested** count even when the count is clamped.
3. Send the payload in groups of up to 16 bits:
   - 9–16 bits: TDI low, TMS low, TDI high, TMS high;
   - 1–8 bits: TDI low, TMS low.
4. Each bit sets TDI and TMS while TCK is low, pulses TCK high, and samples
   TDO.  The reply carries the sampled bits, low byte first, in the same
   group order.
5. After the last bit TCK is left high and TDI/TMS hold their last values.

In a final 9–16-bit group the unused high bits are ignored, but all four
payload bytes are still sent.  A final 1–8-bit group takes two payload
bytes.

There are no state shortcuts: the host walks the TAP, including any reset
sequence.  The firmware keeps no TAP state between commands; only the pin
levels and the settings persist.  The host must account for the 3-byte echo
before the TDO bytes.

### Bit order and grouping

- Data is shifted **least-significant bit first** in every byte.
- The count is the number of TCK pulses, not the number of bytes.
- A 9–16-bit group consumes four payload bytes and returns two TDO bytes.
- A 1–8-bit group consumes two payload bytes and returns one TDO byte.
- The last bit of a scan normally carries TMS high to leave the shift
  state; that bit is part of the count.

### Zero count

A zero count is a guarded no-op on both builds: the 3-byte echo is sent; no
payload is read and no TDO byte follows.  Do not send payload or wait for a
TDO byte.  OpenOCD never asks for zero bits.

### The 8192-bit clamp

A count above 8192 is clamped to 8192.  The echo still repeats the requested
count, so the clamp is not visible in the echo.  The TDO reply is sized for
the clamped count, so expect fewer TDO bytes than the requested count.

The clamp is not a clean truncation.  If the host sends payload for the full
requested count, the firmware consumes only the first 8192 bits; the surplus
bytes stay in the serial stream and are read as command bytes.  The next
command is then misread, which looks like a random mode failure.

Keep the whole request at or below 8192 bits, including its TAP moves, and
split longer scans.  OpenOCD batches queued scans and their TAP moves into
one request; it does not split one longer scan.  A scan of 8192 bits leaves
no room for those moves, so keep margin below the limit.

## Link speed (`0x07`)

One selector byte follows, then the confirmation bytes.

| Part | Value |
|---|---|
| selector `0x01` | fast |
| any other selector | normal (and echoed unchanged) |
| confirmation | `0xAA`, then `0x55` |
| reply | `0x07`, then the selector (or `0x00`) |

The reply is two bytes, not a success code.  A bad confirmation forces the
second byte to `0x00`, so the reply is `0x07 0x00`.  On v3 the rate changes
before the confirmation; send the selector and the two confirmation bytes
in separate writes and switch the host port in between.  On v4 the CDC rate
is host-controlled: the command still replies, but the rate does not change.
Details: [clock.md](clock.md).

## TCK delay (`0x08`)

Two bytes set the inter-bit delay as a 16-bit big-endian value; larger is
slower.  There is no reply and no read-back.  Default `1`; `0` is accepted.
Both builds apply it.  Details: [clock.md](clock.md).

## Unknown bytes

Any byte not in the table is answered with a single `0x00`, and the loop
keeps reading.  That includes `0x04`, `0x09`–`0x0F` and every byte above
`0x0F`.  `0x00` itself is the leave command, so match each request to the
reply you expect.

## Worked exchange

A real 6-bit TAP reset from a fresh entry: five clocks with TMS high leave
the chain in Test-Logic-Reset, and the sixth with TMS low moves to
Run-Test/Idle.  TDI is held low.

```
host -> 0x00 x20          enter binary bitbang
host <- "BBIO1"
host -> 0x06              select OpenOCD path
host <- "OCD1"
host -> 0x01 0x01         pin mode = JTAG (push-pull)
host -> 0x02 0x08 0x01    release SRST (only if wired)
host -> 0x02 0x04 0x01    release TRST (only if wired)
host -> 0x05 0x00 0x06    TAP shift, 6 bits
host <- 0x05 0x00 0x06    command echo
host -> 0x00 0x1F         TDI low, TMS low (five ones, then zero)
host <- 0x00              TDO byte
host -> 0x00              leave OpenOCD
host <- "BBIO1"
host -> 0x0F              leave bitbang
host <- 0x01              return-to-terminal ack
host <- "HiZ>"            (version banner omitted)
```

`0x01` is a raw byte, not the text "1".  The TDO byte is whatever the
device drove during the six clocks; do not expect a fixed value.  A VREG
feature is omitted because this example assumes a self-powered target.

## Limits and notes

- **TAP shifter, not a JTAG engine.**  No IR/DR descriptions, no scan-chain
  or IDCODE handling, no automatic TMS sequences.  The host does all of it.
- Shift count is 16-bit and clamped to 8192 bits per command; the echo
  reports the requested count and the TDO reply is sized for the clamp.
- The path is present on **both v3 and v4**.  On v3 the shift is
  interrupt-driven; on v4 it is a polling loop.  Both apply the `0x08`
  delay; the wire protocol is the same.  Other v3 behaviour is not verified.
- The pin mode must be set before a shift; until then the pins are inputs.
  A mode change drives SRST and TRST low.
- A host driver that speaks this `OCD1` command set is required — the
  shifter is not standard JTAG-over-serial by itself.  OpenOCD's driver is
  POSIX-only.
- Leaving with `0x00` does not reset the target TAP; the target only
  changes state if it sees TCK/TMS or TRST.  Reset the chain before leaving
  if the target must be left in a known state.
- The mode LED is on after binary entry and is not changed by `0x06`; the
  terminal reset at `0x0F` turns it off.

## Related

- [entry.md](entry.md) — entry, settings and exit.
- [tap.md](tap.md) — TAP states, shifts, IDCODE and BYPASS.
- [clock.md](clock.md) — TCK delay and link speed.
- [wiring.md](wiring.md) — pins, levels and pull-ups.
- [xsvf.md](xsvf.md) — the file-driven XSVF path.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
- [tools/openocd](../../tools/openocd/README.md) — host-side setup.
