# PIC — binary protocols

Part of the PIC documentation set (`docs/pic/`).  This page documents the two
scriptable PIC interfaces: the dedicated binary sub-protocol entered with
`0x07`, which prints `PIC1`, and the PIC command group carried inside the
raw-wire binary mode.  The general binary layer (entering BBIO, framing,
returning to the terminal) is documented in `docs/generic/` (see
[commands.md](../../generic/commands.md) §Leaving the CLI) and `docs/binary/` —
see [entering-and-framing.md](../../protocol/binary/entering-and-framing.md),
[commands.md](../../protocol/binary/commands.md) and
[sub-protocols.md](../../protocol/binary/sub-protocols.md).  The raw-wire framing itself is
owned by [../2wire/binary.md](../../protocol/wire/2wire/binary.md).

Both paths are written from the firmware's command definitions and are
**unverified**.

## `PIC1` — the dedicated binary mode (`0x07`)

### Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x07` (enter PIC).  The mode prints **`PIC1`** and starts its own
   command loop.
3. Send `0x00` to leave PIC; the `BBIO1` identifier is printed again.  From
   BBIO, `0x0F` returns to the interactive terminal.

On entry the mode sets open-drain (HiZ) output, drives PGD (MOSI) and PGC
(CLK) low, and selects these defaults:

| Setting | Default |
|---|---|
| opcode width | 6-bit (6b/14b) |
| operating state | Data |
| post-command delay | 2 ms |

### Command set

The reply byte is `0x01` for success and `0x00` for failure, unless the
command itself returns data.  The **high two bits** select the command
family:

| Request | Family | Action | Reply |
|---|---|---|---|
| `0x00` | control | exit to BBIO | `BBIO1` |
| `0x01` | control | report the identifier | `PIC1` |
| `0x02` | control | select 6-bit opcodes | `0x01` |
| `0x03` | control | select 4-bit opcodes | `0x01` |
| `0x04`–`0x07` | control | set the post-command delay to `0`–`3` ms | `0x01` |
| `0x10`–`0x17` | config | PWM / VREG / pull-up switches | `0x01` / `0x00` |
| `0x18`–`0x1F` | config | AUX / MISO / CS switches | `0x01` / `0x00` |
| `0x40`–`0x7F` | write | clock out an opcode (low 6 bits) | `0x01` |
| `0x80`–`0xBF` | write+data | opcode, then two host bytes as a payload | `0x01` |
| `0xC0`–`0xFF` | write+read | opcode, then two payload bytes read back | `0x01`, then 2 data bytes |

Control and config families:

- `0x10`–`0x17` (bit 3 clear) switch board elements: bit 2 = PWM (a fixed
  preset), bit 1 = voltage regulator, bit 0 = pull-up resistors.
- `0x18`–`0x1F` (bit 3 set) drive the auxiliary pins: bit 2 = AUX, bit 1 =
  MISO, bit 0 = CS (set = high).
- Any other byte in the `0x00`–`0x3F` range is a failure (`0x00`).

Write families:

- `0x40`–`0x7F` sends the low 6 bits as a command opcode (in 4-bit mode only
  the low 4 bits are clocked).
- `0x80`–`0xBF` sends the opcode, then reads **two big-endian host bytes**
  (high, then low) and clocks them out as the 14/16-bit payload.
- `0xC0`–`0xFF` sends the opcode, acknowledges, then clocks in a 14/16-bit
  payload and returns it as **two big-endian bytes** (high, then low).

### Worked exchange

```
host -> 0x00 x20        enter bitbang
host <- "BBIO1"
host -> 0x07            enter PIC
host <- "PIC1"
host -> 0x02            select 6-bit opcodes
host <- 0x01
host -> 0x04            post-command delay = 0 ms
host <- 0x01
host -> 0x1C            AUX/MISO/CS: AUX high (bit 2 set)
host <- 0x01
host -> 0x40            clock out opcode 0x00
host <- 0x01
host -> 0x80            clock out opcode 0x00, then a 16-bit payload
host -> 0x12 0x34       the payload (high, low)
host <- 0x01
host -> 0xC0            clock out an opcode, then read a payload back
host <- 0x01
host <- 0x00 0x12       the device payload (high, low)
host -> 0x00            leave PIC
host <- "BBIO1"
```

## PIC in raw-wire mode (`0x05` + PIC command bytes)

The raw-wire binary mode also carries a PIC command group (`0xAx`).  This is
a separate PIC24/"SIX" ICSP engine: it takes whole command scripts and
executes them, rather than the byte-per-opcode model of `PIC1`.  It shares
the `RAW1` framing; enter raw wire with `0x05` (prints `RAW1`) and send one of
these command bytes:

| Request | Action | Payload / reply |
|---|---|---|
| `0xA0` | select the PIC engine | 1 host byte: `1` = 416, `2` = 424, `3` = 614; reply `0x01` / `0x00` |
| `0xA4` | write | 1 count byte, then `count`×3 bytes (416) or `count`×4 bytes (424); reply `0x01` |
| `0xA5` | write-and-read bits | 1 count byte, then (416) one command byte; replies with the read bytes |
| `0xA7` | write-and-read commands | a write count, a read count, then a script buffer; reply `0x01` plus any read bytes |

- `0xA4` and `0xA5` are refused (`0x00`) in the `614` engine.
- `0xA7`'s script is a run of marker bytes: `1` = a write record, `2` = a
  read record; the record length depends on the selected engine.
- Any other byte in the `0xAx` group (for example `0xA1`–`0xA3`, `0xA6`, or
  `0xA8`–`0xAF`) is undefined and returns failure (`0x00`).

The exact script grammar is part of the raw-wire engine and is **unverified**;
use [../2wire/binary.md](../../protocol/wire/2wire/binary.md) for the shared `RAW1` framing.

## Limits

- The `PIC1` protocol is a thin bit-shifter: it has no PIC algorithm, no part
  database, and no VPP control beyond the AUX/MISO/CS lines.
- `0xA4`/`0xA5`/`0xA7` in raw-wire mode are **unverified**, and the `PIC1`
  protocol is likewise documented from the firmware only.
- The raw-wire PIC group is always present; it is not conditioned on the PIC
  mode being built.

## v3 / v4

`PIC1` (`0x07`) and the raw-wire PIC group are present in both builds.  Neither
build has been verified on hardware for these paths here.