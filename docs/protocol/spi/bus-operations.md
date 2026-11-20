# SPI — bus operations

Part of the SPI documentation set (`docs/protocol/spi/`). This page owns the
on-bus terminal commands: chip-select control, byte writes and reads, the
read-back form, and the byte-only transfer rule. Shared prompt grammar — value
entry, repetition `:N`, width `;N`, the wrappers, bit order, `(n)` macros — is
owned by [docs/generic/](../../generic/README.md); only the SPI effect is
described here.

## Chip select

| Input | Action | Reply with the default `/CS` |
|---|---|---|
| `[` | assert CS; clear the read-back flag | `/CS ENABLED` |
| `{` | assert CS; set the read-back flag | `/CS ENABLED` |
| `]` | release CS | `/CS DISABLED` |
| `}` | release CS (same as `]`) | `/CS DISABLED` |

The CS polarity is selected during setup ([entry-and-setup.md](entry-and-setup.md)).
With the **`/CS`** polarity (the default) the firmware prefixes the label with
`/`, so the line is `/CS ENABLED` / `/CS DISABLED`. With the **`CS`** polarity
there is no prefix: `CS ENABLED` / `CS DISABLED`.

`[` and `{` both assert CS; they differ only in the **read-back flag**:

- `[` clears it — a plain write sends its value and reports nothing back.
- `{` sets it — every subsequent write also clocks in a byte and reports it.

`]` and `}` are identical: both release CS. The flag set by `{` is **not**
cleared by `]` or `}`; it stays set until `[` clears it or the mode is
re-entered. `w`/`W` do not clear it. Use `[` for the next plain transaction.

In open-drain output, "release" means high impedance, not a driven high. With
active-high `CS` the asserted level is high, which in open drain also depends
on the pull-up. The electrical rules are in [wiring.md](wiring.md).

## Writing a byte

A number written in the mode is one SPI byte, clocked out on MOSI:

| Input | Meaning |
|---|---|
| `0xNN` | hex byte (`0x9F`, `0xA` — partial widths allowed) |
| `0b…` | binary byte (partial widths allowed) |
| `NNN` | decimal byte (no prefix) |

The reply echoes the value:

```
WRITE: 0x9F
```

When the read-back flag is set (after `{`), the write also reports the byte
clocked in from MISO during the same eight clocks:

```
WRITE: 0x06 READ: 0x00
```

The values are printed in the current display format (`o`), hex by default.
`:N` repeats the write; see the shared grammar.

## Reading a byte

`r` reads one byte. SPI is full duplex, so the read is a write of the dummy
byte `0xFF` while the returned byte is captured:

```
READ: 0xEF
```

`r:N` reads N bytes and prints them after one `READ: ` prefix. The shared `:N`
and `;N` grammar is in [docs/generic/](../../generic/values.md).

`r` does not require CS to be asserted; it clocks the dummy byte and captures
MISO with whatever CS state is current. Assert CS first if the target needs it.

If the SPI module cannot complete the transfer, the read returns `0xFF` — the
same value a real all-ones byte produces. There is no separate timeout error;
see [binary.md](binary.md) §Transfer faults.

## Bit and word transfers

SPI here is **byte-wide**. Every value write clocks exactly eight bits, and
every read returns eight bits. There is no bit-level or 16-bit transfer
command in the mode.

The shared bit-level wrappers have no SPI handler:

| Input | SPI behaviour |
|---|---|
| `/` `\` `^` | no clock control; generic no-effect error and a syntax error |
| `-` `_` `.` `!` | no data-pin control; generic no-effect error and a syntax error |

The commands still print their normal prefix (`CLOCK, 1`/`CLOCK, 0`,
`DATA OUTPUT, 1`/`DATA OUTPUT, 0`, `DATA STATE: `, `READ BIT: `,
`CLOCK TICKS: `) before the `ERROR: command has no effect here` line, so the
result can look mangled; the syntax-error line follows. Use byte writes and
reads instead.

`;N` (the shared width setting) changes the **displayed** width and the window
used by the bit-order setting; it does not make the transfer N bits wide. A
value above 8 bits is truncated to its low byte on the wire, even though the
printed value can show the wider number. Keep `;8` in SPI. `l`/`L` reverses
the bits within the selected width; with `;8` that is the bit order within the
byte.

There is no 16-bit read either: read two bytes and assemble the word in the
host.

## Worked session

A small transaction: bring the bus up with `W`, assert CS, send a command byte,
read a byte, release CS. This session uses the default `/CS` polarity and
normal output. The `0x9F` / `0xEF` values are example values; the byte read
from a real target is whatever the target returns.

```
SPI>W
POWER SUPPLIES ON
Clutch engaged!!!
SPI>[
/CS ENABLED
SPI>0x9F
WRITE: 0x9F
SPI>r
READ: 0xEF
SPI>]
/CS DISABLED
SPI>
```

The read-back form (`{`) reports the returned byte next to each write:

```
SPI>{
/CS ENABLED
SPI>0x06
WRITE: 0x06 READ: 0x00
SPI>0x00
WRITE: 0x00 READ: 0x00
SPI>}
/CS DISABLED
SPI>
```

The flag remains set after `}`; the next plain transaction starts with `[`.

## Edge cases

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A byte is written with no `[` or `{` | the clocks are sent with CS in its previous state | assert CS with `[` or `{` first |
| `CS` was selected instead of `/CS` | CS is asserted with the opposite level; the target ignores the transaction | re-enter SPI mode and pick the other CS option |
| `{` was used and a later write also printed `READ:` | every write reports the byte clocked in | send `[` to clear the flag, or re-enter the mode |
| `]` and `}` were expected to differ | both release CS and print the same line | they are identical; only `[`/`{` differ |
| A bit-level command was typed | the generic no-effect error and a syntax error, possibly after the normal prefix | use byte writes/reads |
| `;16` was set and a value written | the printed value is wide but only the low byte is clocked | set `;8` |
| A write value is above `0xFF` | the printed value can be wider, but only the low byte reaches the bus | send one byte per value |
| `r` returns `0xFF` | a real target byte can also be `0xFF` | there is no separate timeout error to tell them apart |
| MISO floats (no target, or target not selected) | `r` returns `0x00`, `0xFF` or noise | select and power the target, or provide the bus's normal pull-up |

## v3 / v4

CS control, byte read/write, the read-back flag and the byte-only rule are the
same in both builds. Only the sniffer's CS capture path differs (polled on v3,
CN on v4 — see [sniffer.md](sniffer.md)). The v3 path is **not verified**.

## Related docs

- [entry-and-setup.md](entry-and-setup.md) — CS polarity and output type.
- [wiring.md](wiring.md) — pull-ups and the released CS level.
- [macros.md](macros.md) — macros and the in-place clock tweaks.
- [binary.md](binary.md) — the `SPI1` byte commands and fault fill.
- [edge-cases.md](edge-cases.md) — transfer and chip-select failures.
