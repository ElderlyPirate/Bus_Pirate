# JTAG — clocking and speed

Part of the JTAG documentation set.  This page covers the two independent
rates in a JTAG session — TCK on the target side and the host link — and
the settings that affect them.  TAP mechanics are in [tap.md](tap.md); the
byte commands are in [openocd.md](openocd.md) and [xsvf.md](xsvf.md).

## Two clocks

| Clock | What it times | Who sets it |
|---|---|---|
| TCK | the TAP and every shifted bit | the firmware shift loop |
| host link | USB CDC (v4) or UART (v3) bytes | the host and `0x07` |

They are independent: changing the link speed does not change TCK, and
changing the TCK delay does not change the link.

## OpenOCD path — TCK delay (`0x08`)

`0x08` is followed by a 16-bit big-endian delay value.  There is no reply.
The default at entry is `1`; `0` is accepted.

| Value | Effect |
|---|---|
| `0` | minimum delay; fastest TCK the loop allows |
| `1` | default |
| larger | slower TCK |

The value is a loop count, not microseconds, and the firmware does not
report a frequency.  The actual TCK rate depends on the board clock, the
build and the loop; measure it if a target needs a known rate.

Both builds apply the delay around each TCK edge: v3 in its interrupt-driven
shift loop, v4 in its polling loop.  The delay applies to every bit of a
`0x05` shift.

**Host note.**  OpenOCD's Bus Pirate driver does not send `0x08`; a stock
session uses the default.  `adapter speed` in a target script is not
forwarded to the board.  A custom host can send `0x08` before a shift to
slow TCK.  See [tools/openocd](../../tools/openocd/README.md).

## XSVF player — no TCK setting

The XSVF player has no clock command and no host-visible TCK rate.

- XSVF data shifts and `XSTATE` transitions write TCK directly; they add
  no delay beyond the instruction loop.
- The player's `0x01` reset and the `0x02` probe use fixed helpers with a
  **100 µs** TCK half-period (nominal 5 kHz) and a 20 µs data settle.  The
  probe's 510-clock IR fill alone is about 0.1 s.
- `XRUNTEST` and `XWAIT` are waits after or between shifts, not clock
  changes.

A run's data shifts are much faster than the `0x01` reset and `0x02`
probe.  Neither rate is measured or reported.  A target that needs a
slower TCK cannot be slowed from the XSVF path; use the OpenOCD path with
`0x08` and a custom host, or another programmer.

## Link speed (`0x07`)

`0x07` changes the host link, not TCK.  One selector byte follows, then the
host sends `0xAA` and `0x55`:

| Selector | Rate | Notes |
|---|---|---|
| `0x01` | fast | v3: about 1 Mbaud UART; v4: CDC rate unchanged |
| any other | normal | v3: 115200; v4: CDC rate unchanged |

The reply is two bytes, `0x07` then the selector.  It is **not** a success
code: the second byte repeats what you sent, except that a bad `0xAA 0x55`
confirmation forces `0x00` (normal).  So a failed confirmation answers
`0x07 0x00`, not a single `0x00`.

On v3 the rate changes as soon as the selector is read, **before** the
confirmation bytes.  Send `0x07` and the selector, switch the host port to
the new rate, then send `0xAA 0x55` in a separate write.  A failed
confirmation returns the firmware to normal, and the host must follow.  On
v4 the rate is host-controlled, so the write split is not required; the
command still replies `0x07` and the selector but changes no rate.

On v3 the rate is not reset by `0x06`; it stays at the last `0x07` setting
until changed or a board reset.  The terminal link starts at 115200.

## Scan time

A rough estimate for one `0x05` block:

```
time ≈ (bit count × TCK period) + (link bytes × link time)
```

- The bit count includes the state-walk bits.  A 32-bit IDCODE read is 43
  bits in the single-device example in [tap.md](tap.md#one-idcode-read),
  not 32.
- The link carries the 3-byte echo, the TDI/TMS payload (two bytes per
  group of up to 8 bits, four for 9–16 bits) and the TDO reply.
- A clamped shift is a special case: the firmware consumes at most 8192
  bits and the TDO reply is sized for the clamp.  If the host sends the
  full requested payload, the surplus is read as commands and desyncs the
  mode.  Split long scans instead.

No rate is measured or reported.  For a target with a known TCK limit,
start with a larger delay and reduce it once the chain is stable.  On the
XSVF path there is no adjustment; use a different programmer if the fixed
rate is too fast.

## Related

- [README.md](README.md) — overview and index.
- [entry.md](entry.md) — binary entry, settings and exit.
- [wiring.md](wiring.md) — pins, power, levels and pull-ups.
- [tap.md](tap.md) — TAP states, shifts and scans.
- [openocd.md](openocd.md) — the `0x06` command set.
- [xsvf.md](xsvf.md) — the `0x18` player.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
