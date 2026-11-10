# SPI — bus operations

Part of the SPI documentation set (`docs/spi/`). This page owns the on-bus
commands: chip-select control, byte read/write, and the read-back form. Shared
prompt grammar — value entry, repetition `:N`, width `;N`, the wrappers,
`(n)` macros — is owned by `docs/generic/`; only the SPI effect is described
here.

## Chip select

| Input | Action | Firmware output (default `/CS`) |
|---|---|---|
| `[` | assert CS; clears the read-back flag | `/CS ENABLED` |
| `{` | assert CS; sets the read-back flag | `/CS ENABLED` |
| `]` | release CS | `/CS DISABLED` |
| `}` | release CS (same effect as `]`) | `/CS DISABLED` |

The CS polarity is chosen during setup:
[entry-and-setup.md](entry-and-setup.md). With the **`/CS`** polarity (the
default) the firmware prefixes the label with `/`, so the printed line is
`/CS ENABLED` / `/CS DISABLED`. With the **`CS`** polarity there is no prefix:
`CS ENABLED` / `CS DISABLED`.

`[` and `{` both assert CS; they differ only in the **read-back flag**:

- `[` clears it — a plain write sends its value and reports nothing back.
- `{` sets it — every subsequent write also clocks in a byte and reports it
  (see [Writing a byte](#writing-a-byte)).

`]` and `}` are identical: both release CS. The flag set by `{` is **not**
cleared by `]` or `}`; it stays set until `[` clears it or the mode is
re-entered. Use `[` for the next plain transaction.

## Writing a byte

Any number written in the mode is an SPI byte, clocked out on MOSI:

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
clocked in from MISO on the same line:

```
WRITE: 0x06 READ: 0x00
```

The values are printed in the current display format (`o`), hex by default.

## Reading a byte

`r` reads one byte. Since SPI is full duplex, the read is a write of the
dummy byte `0xff` while the returned byte is captured:

```
READ: 0xEF
```

`r:N` reads N bytes. The shared `:N` and `;N` grammar is in
`docs/generic/` (see [commands.md](../../generic/commands.md)).

## Worked session

A format illustration of a small transaction — assert CS, send a command byte,
read a byte, release CS. The `0x9F` / `0xEF` values are example values; on real
hardware the read byte is whatever the target returns. This session uses the
default `/CS` polarity and normal output:

```
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

A binary-mode exchange of the same kind of transaction is in
[binary.md](binary.md).

## v3 / v4

The CS control, byte read/write and the read-back flag behave the same in both
builds; only the sniffer's CS capture path differs (see
[sniffer.md](sniffer.md)). The v3 path is **not verified**.