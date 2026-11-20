# 3-Wire — bus operations

Part of the [3-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page owns the on-bus commands: CS
control, byte read/write, the clock edges, and the bitwise controls.  Shared
prompt grammar — value entry, repetition `:N`, width `;N`, the wrappers, `(n)`
macros, user macros `<n>` — is owned by
[docs/generic/](../../../generic/README.md); only the 3-Wire effect is
described here.

## Chip select

The CS polarity is chosen at entry (see
[entry-and-setup.md](entry-and-setup.md)); the settings header prints it as the
`csl` field (`0` = `CS`, `1` = `/CS`).

| Input | Meaning | Sets the read-back flag |
|---|---|---|
| `[` | enable CS | clears it |
| `{` | enable CS, read-back form | sets it |
| `]` | disable CS | clears it |
| `}` | disable CS (same effect as `]`) | clears it |

`[` and `{` both enable CS; they differ only in the **read-back flag**:

- `[` clears it — a plain write sends its value and reports nothing back.
- `{` sets it — every subsequent write also clocks a value in from MISO and
  reports it (see [Writing a value](#writing-a-value)).

`]` and `}` are identical, and both **clear** the read-back flag (the stop path
resets it).  This differs from SPI, where the flag survives a `]`.

### Which level is driven

The level follows the chosen polarity: with a plain `CS` choice the signals
are active-high; with `/CS` (the default) they are active-low.

| CS chosen at entry (`csl`) | `[` / `{` drives | `[` / `{` prints | `]` / `}` drives | `]` / `}` prints |
|---|---|---|---|---|
| `CS` (`1`, `csl` = 0) | CS **high** | `CS ENABLED` | CS **low** | `CS DISABLED` |
| `/CS` (`2`, default, `csl` = 1) | CS **low** | `/CS ENABLED` | CS **high** | `/CS DISABLED` |

So with the **default `/CS`**, `[`/`{` assert CS low and print `/CS ENABLED`;
`]`/`}` release it high and print `/CS DISABLED`.  In normal output "high"
means driven to 3.3 V; in open drain it means released to the pull-up.

If the target never selects, check the polarity: choose `CS` at entry only if
the target needs CS asserted **high** (active-high).  See
[edge-cases.md](edge-cases.md).

CS is controlled only by the four wrapper commands; the bitwise controls below
act on CLK and the data lines, not on CS.

## Writing a value

Any number written in the mode is clocked out on MOSI as the current width
(8 bits by default; see [Width setting (N)](#width-setting-n)), MSB-first
unless LSB-first is selected, while MISO is sampled on the same clocks:

| Input | Meaning |
|---|---|
| `0xNN` | hex value (`0x01`, `0xA` — partial widths allowed) |
| `0b…` | binary value (partial widths allowed) |
| `NNN` | decimal value (no prefix) |
| `"text"` | the literal bytes of the string |

The reply echoes the value:

```
3WIRE>0x01
WRITE: 0x01
3WIRE>0xA5
WRITE: 0xA5
```

When the read-back flag is set (after `{`), the write also reports the value
clocked in from MISO on the same line.  This example uses the default `/CS`
polarity; the values are examples and the read byte is whatever the target
returns:

```
3WIRE>{
/CS ENABLED
3WIRE>0x06
WRITE: 0x06 READ: 0x00
3WIRE>0x00
WRITE: 0x00 READ: 0x00
3WIRE>}
/CS DISABLED
```

The `WRITE:` and `READ:` values are printed in the current display format (`o`),
hex by default.  Because the mode drives MOSI and samples MISO on the same
clock, a plain write is still useful for driving a "read" device: writing
all-ones (`0xFF`, or the `r` command) releases MOSI in open drain (in normal
output it drives MOSI high), and MISO is sampled either way.

## Reading a value

`r` reads one value by clocking out the dummy byte `0xFF` and capturing the
value returned on MISO:

| Input | Meaning |
|---|---|
| `r` | read one value (`READ: <value>`) |
| `r:N` | read N values on one header line |

```
3WIRE>r
READ: 0x00
3WIRE>r:2
READ: 0x00 0x00
```

With nothing driving MISO the value is a floating level, not a device value.

## Width setting (N)

`;N` sets the number of bits for the value, from **1 to 16**; a width above 16
is a syntax error.  The width is remembered until changed or until the mode is
left.  Both reads and writes use it:

```
3WIRE>r;16
READ: 0xFFFF
3WIRE>1;16
WRITE: 0x0001
```

## Clock edges

At `W` the firmware puts CLK and MOSI low and CS at its inactive level; MISO is
an input.  The first `^` therefore starts with a rising edge.

| Operation | Edge order |
|---|---|
| write one bit | set MOSI (settle) → CLK high (delay) → sample MISO → CLK low (delay) |
| read one bit | drive `0xFF` on MOSI, same clock, sample MISO |
| `^` tick | CLK high (delay) → CLK low (delay) |
| `/` / `\` | set CLK high / low and hold (settle delay) |
| `!` bit read | CLK high (delay) → sample MISO → CLK low (delay) |

A target should present its MISO data before or at the rising edge; the
sample is taken after the clock-high delay, while CLK is high.  A target that
changes MISO only on the falling edge is still sampled correctly on the next
rising edge, but hand-built frames may need `\`/`/` to place the edge
explicitly.

## Bitwise controls

3-Wire's distinguishing feature is that the clock and both data directions
can be driven a bit at a time.  The grammar and firmware wording are shared
with the base layer (see [commands.md](../../../generic/commands.md)
§Transaction wrappers and bit ops); the 3-Wire effect is:

| Input | Action | Firmware output |
|---|---|---|
| `/` | drive CLK high | `CLOCK, 1` |
| `\` | drive CLK low | `CLOCK, 0` |
| `^` / `^:N` | one clock tick (N ticks) | `CLOCK TICKS: <n>` |
| `-` | drive the data-out line (MOSI) high | `DATA OUTPUT, 1` |
| `_` | drive the data-out line (MOSI) low | `DATA OUTPUT, 0` |
| `.` | read the data-in line (MISO), no clock | `DATA STATE: 0` or `1` |
| `!` / `!:N` | read N bits from MISO, each with a clock | `READ BIT: <bits>  *pin is now HiZ` |

- `/` and `\` set the clock **level** (they do not pulse); `^` pulses the
  clock one full cycle per tick.  At engagement CLK idles low, so the first
  `^` starts with a rising edge.
- `-` and `_` set the **data-out** level on MOSI; `.` samples the **data-in**
  level on MISO.  The two are separate lines, so `.` never reports what `-`/`_`
  just drove.
- `!` clocks MISO while sampling: one read per clock tick.  The
  ` *pin is now HiZ` suffix is a shared string carried over from the 2-Wire
  wording; on 3-Wire the MISO input line is not driven in the first place.
  As in 2WIRE, the trailing space after each bit plus the suffix's leading
  space gives two spaces before `*pin`.
- `.` prints no trailing newline; the next prompt continues on the same line.
- The `^` count uses the current display format (HEX by default), so one tick
  prints `CLOCK TICKS: 0x01`.
- `/`, `\`, `-` and `_` each carry the **selected speed profile's** settle
  delay; `^` carries the profile's clock delay.

These primitives are what you use to hand-build a custom three-wire frame —
bit order, idle levels, setup/hold — when a protocol is not one the mode
already knows.

## No macros

3-Wire has **no macros, by design**.  `(0)` does not print a macro menu; the
mode's macro handler deliberately does nothing, so it reports an error.  The
blank line between the error and the syntax line is real:

```
3WIRE>(0)
ERROR: command has no effect here

Syntax error at char 3
```

If you expected macros (as in 2-Wire's ISO 7816-3 ATR, or SPI's sniffer), use
one of these instead:

- build your sequence from the **user macros** `<n>` in the base layer (see
  [macros.md](../../../generic/macros.md)), or
- drive the bus from a **host script** over the binary `RAW1` path (see
  [binary.md](binary.md)) or the terminal, or
- switch to the **SPI** mode if the target is a real SPI device — 3-Wire's
  primitive-only design is intentional.

## Worked session

A format illustration of a small transaction — enable CS, send a byte, read a
byte, disable CS.  The `0xA5` / `0x00` values are examples; on real hardware
the read byte is whatever the target returns.  This example uses the default
`/CS` polarity and normal output:

```
3WIRE>[
/CS ENABLED
3WIRE>0xA5
WRITE: 0xA5
3WIRE>r
READ: 0x00
3WIRE>]
/CS DISABLED
3WIRE>
```

With the default `/CS`, the first line drives CS low (asserted) and the last
drives it high (released) — see
[§Which level is driven](#which-level-is-driven).
