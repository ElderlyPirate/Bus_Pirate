# 3WIRE — bus operations

Part of the [3-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page owns the on-bus commands: CS
control, byte read/write, and the bitwise controls.  Shared prompt grammar —
value entry, repetition `:N`, width `;N`, the wrappers, `(n)` macros, user
macros `<n>` — is owned by `docs/generic/`; only the 3-Wire effect is
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
(8 bits by default; see [Width — `;N`](#width--n)), MSB-first unless LSB-first
is selected, while MISO is sampled on the same clocks:

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

## Width — `;N`

`;N` sets the number of bits for the value, from **1 to 16**; a width above 16
is a syntax error.  The width is remembered until changed or until the mode is
left.  Both reads and writes use it:

```
3WIRE>r;16
READ: 0xFFFF
3WIRE>1;16
WRITE: 0x0001
```

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
| `.` | read the data-in line (MISO), no clock | `DATA STATE: <0\|1>` |
| `!` / `!:N` | read N bits from MISO, each with a clock | `READ BIT: <bits> *pin is now HiZ` |

- `/` and `\` set the clock **level** (they do not pulse); `^` pulses the
  clock one full cycle per tick.  At engagement CLK idles low, so the first
  `^` starts with a rising edge.
- `-` and `_` set the **data-out** level on MOSI; `.` samples the **data-in**
  level on MISO.  The two are separate lines, so `.` never reports what `-`/`_`
  just drove.
- `!` clocks MISO while sampling: one read per clock tick.  The
  ` *pin is now HiZ` suffix is a shared string carried over from the 2-Wire
  wording; on 3-Wire the MISO input line is not driven in the first place.
- `/`, `\`, `-` and `_` each carry the **selected speed profile's** settle
  delay; `^` carries the profile's clock delay.

These primitives are what you use to hand-build a custom three-wire frame —
bit order, idle levels, setup/hold — when a protocol is not one the mode
already knows.

## No macros

3-Wire has **no macros, by design**.  `(0)` does not print a macro menu; the
mode's macro handler deliberately does nothing, so it reports an error:

```
3WIRE>(0)
ERROR: command has no effect here
Syntax error at char <n>
```

If you expected macros (as in 2-Wire's ISO 7816-3 ATR, or SPI's sniffer), use
one of these instead:

- build your sequence from the **user macros** `<n>` in the base layer (see
  [commands.md](../../../generic/commands.md)), or
- drive the bus from a **host script** over the binary `RAW1` path (below) or
  the terminal, or
- switch to the **SPI** mode if the target is a real SPI device — 3-Wire's
  primitive-only design is intentional.

## Scripting — the shared `RAW1` binary path

The 3-Wire primitives are also reachable without the terminal through the
bitbang binary layer's **`RAW1`** sub-protocol, which serves both 2-Wire and
3-Wire.  The shared framing and command groups are in
[the shared `RAW1` page](../binary.md).  Its command set drives CS explicitly
(`0x04` CS low / `0x05` CS high).

- The worked exchange on that page is written for 2-wire; a 3-wire host
  changes the configuration byte and drives CS explicitly.  The rest of the
  framing is identical.
- The 3-Wire-specific difference is one configuration bit: in the `0x8x`
  configuration command, bit 2 selects **3 wires** (on entry the handler starts
  in 2-wire).  For example `0x8C` = open drain + 3 wires + MSB-first, and
  `0x84` = normal output + 3 wires + MSB-first.
- Reads in 3-wire use the same "clock `0xFF`, capture MISO" path as `r`.
- This path is **not verified** on hardware.

A minimal 3-wire byte exchange.  The device bytes are illustrative; with open
drain a pull-up is still required, so the `0x51` / `0x4D` setup steps are
included:

```
host -> 0x00 x20     enter bitbang
host <- "BBIO1"
host -> 0x05         select raw wire
host <- "RAW1"
host -> 0x8C         configure: open drain, 3 wires, MSB-first
host <- 0x01
host -> 0x60         set speed = profile 0
host <- 0x01
host -> 0x51         v4: enable the 3.3V pull-up feed
host <- 0x01
host -> 0x4D         peripherals: supply + pull-ups on, CS high
host <- 0x01
host -> 0x04         CS low
host <- 0x01
host -> 0x10         bulk-write 1 byte (length selector 0)
host <- 0x01
host -> 0xA5         data byte
host <- (sampled byte)   the MISO byte read during the write
host -> 0x06         read one byte (clocks 0xFF, captures the next sample)
host <- (one data byte)
host -> 0x05         CS high
host <- 0x01
host -> 0x00         exit -> BBIO1
host <- "BBIO1"
```

Unlike the 2-wire path, the 3-wire bulk-byte command sends one `0x01` before
the data bytes, then returns one sampled MISO byte for each byte sent.  The
write is full duplex — the byte is clocked out on MOSI while MISO is
sampled — and the read command (`0x06`) returns the next value clocked in from
MISO.

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

## v3 / v4

The CS control, byte read/write, the read-back flag, the width rules and the
bitwise controls are the same in both builds.  Only the `v` label order differs
(see [wiring.md](wiring.md) §Pin table), and the `RAW1` pull-up feed is v4
only.  The v3 path is **not verified**.
