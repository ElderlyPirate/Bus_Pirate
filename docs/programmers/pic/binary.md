# PIC — binary protocols

Part of the PIC documentation set (`docs/programmers/pic/`).  This page owns
the two scriptable PIC interfaces: the dedicated `PIC1` loop entered with
`0x07`, and the PIC command group inside raw wire (`0xAx`).  The shared
binary layer — entering `BBIO1`, framing, leaving — is in
[the binary layer](../../protocol/binary/README.md); the raw-wire framing is
owned by [the shared RAW1 page](../../protocol/wire/binary.md).

The examples in this page are protocol descriptions; v3 is build-only.

## `PIC1` — the dedicated loop (`0x07`)

### Entry and exit

1. Enter the binary bitbang layer and wait for `BBIO1`; the 20-NUL entry is
   in [entering and framing](../../protocol/binary/entering-and-framing.md).
2. Send `0x07`; the loop prints `PIC1`.
3. Send `0x00` to leave; the binary layer prints `BBIO1`.  From `BBIO1`,
   send `0x0F` to return to the terminal.

On entry the loop selects open-drain outputs, drives PGD (MOSI) and PGC (CLK)
low, and sets:

| Setting | Default |
|---|---|
| opcode width | 6-bit (6b/14b) |
| state | Data |
| post-command delay | 2 ms |

The binary entry reset switches the v4 **Vpullup source off**, and `PIC1` has
no command to select it.  Feed the VPU pin externally (or fit external
pull-ups) and use `0x11` if you want the board pull-ups in circuit.

### Command set

Each request is one byte.  The reply is `0x01` for success and `0x00` for
failure, unless the request returns data.  The high two bits select the
family:

| Request | Family | Action | Reply |
|---|---|---|---|
| `0x00` | control | leave to `BBIO1` | `BBIO1` |
| `0x01` | control | report the identifier | `PIC1`, then `0x01` |
| `0x02` | control | select 6-bit opcodes | `0x01` |
| `0x03` | control | select 4-bit opcodes | `0x01` |
| `0x04`–`0x07` | control | post-command delay 0–3 ms | `0x01` |
| `0x10`–`0x17` | config | PWM / VREG / pull-up switches | `0x01` |
| `0x18`–`0x1F` | config | AUX / MISO / CS switches | `0x01` |
| `0x40`–`0x7F` | write | clock out an opcode (low 6 bits) | `0x01` |
| `0x80`–`0xBF` | write+data | opcode, then two host bytes as a payload | `0x01` |
| `0xC0`–`0xFF` | write+read | opcode, then a payload read back | `0x01`, then two bytes |

Control and config details:

- `0x10`–`0x17`: bit 2 switches the PWM preset (100 Hz, 50 % when on),
  bit 1 the voltage regulator, bit 0 the pull-up resistors.  `0x10` turns all
  three off.  These switches always reply `0x01`.
- `0x18`–`0x1F`: bit 2 = AUX, bit 1 = MISO, bit 0 = CS; 1 = high (released
  in open drain).  The byte sets all three lines at once, so track the full
  state.  Always `0x01`.
- Any other byte in `0x00`–`0x3F` is a failure (`0x00`).

Write families:

- `0x40`–`0x7F` sends the low 6 bits as an opcode; in 4-bit mode only the low
  4 bits are clocked.  Every write command selects command state, sends the
  opcode, then returns to data state; the host never sends `[`/`]`.
- `0x80`–`0xBF` sends the opcode, reads two host bytes high then low, and
  clocks them out as the 14/16-bit payload.
- `0xC0`–`0xFF` sends the opcode, replies `0x01`, then clocks in a 14/16-bit
  payload and returns it high then low.  A 14-bit value is right-aligned:
  bits 15–14 of the high byte are zero.

### Worked exchange

```
host -> 0x00 x20        enter the binary layer
host <- "BBIO1"
host -> 0x07            enter PIC1
host <- "PIC1"
host -> 0x04            post-command delay 0 ms
host <- 0x01
host -> 0x11            pull-up resistors on (VPU fed externally)
host <- 0x01
host -> 0x80            opcode 0x00, then a 16-bit zero payload
host -> 0x00 0x00       payload high, low
host <- 0x01
host -> 0x82            opcode 0x02, then a 14-bit payload
host -> 0x12 0x34       payload high, low
host <- 0x01
host -> 0xC4            opcode 0x04, then read a payload
host <- 0x01
host <- 0x12 0x34       payload high, low
host -> 0x00            leave PIC1
host <- "BBIO1"
host -> 0x0F            back to the terminal
host <- 0x01
```

The opcode values are the six-bit example set (see
[config-words.md](config-words.md)); the exchange is an example.

With the pull-up fitted and no target, a `0xC0`-family read returns
`0x3F 0xFF` in 6-bit mode (14-bit all ones) or `0xFF 0xFF` in 4-bit mode.

## PIC in raw wire (`0x05` + `0xAx`)

Raw wire carries a separate script-driven PIC engine.  Enter raw wire with
`0x05` and wait for `RAW1`; the shared framing is in
[the shared RAW1 page](../../protocol/wire/binary.md).  The default engine is
**614**; `0xA0` selects another.

| Request | Engine | Action | Host sends | Device returns |
|---|---|---|---|---|
| `0xA0` | all | select engine | 1 byte: `1` = 416, `2` = 424, `3` = 614 | `0x01` / `0x00` |
| `0xA4` | 416 | write records | count, then 3 bytes per record | `0x01` / `0x00` |
| `0xA4` | 424 | write records | count, then 4 bytes per record | `0x01` / `0x00` |
| `0xA5` | 416 | write and read bits | count, then one command byte | one byte per count; **no result byte** |
| `0xA5` | 424 | write and read bits | count only | six bytes per count; **no result byte** |
| `0xA7` | all | write and read script | write count, read count, then the script | `0x01`, plus any read bytes |

Every count in that table is **one byte** (0–255 records).  The shared binary
page calls the `0xA4`/`0xA7` payloads "command words"; this page calls the
`0xA7` sequence a "script" and its entries "records".  They are the same
bytes.

Engine names follow the widths: **614** = 6-bit command / 14-bit data,
**416** = 4-bit / 16-bit, **424** = 4-bit / 24-bit.  Match the engine to the
part.  The numbers are crossed between interfaces:

| Family | Terminal answer | Header digit | `PIC1` command | Raw-wire `0xA0` |
|---|---|---|---|---|
| PIC10/12/16 (6b/14b) | `1` | `6` | `0x02` | `3` (614) |
| PIC18 (4b/16b) | `2` | `4` | `0x03` | `1` (416) |
| PIC24/dsPIC (4b/24b) | — | — | — | `2` (424) |

A user who selected terminal `1` for a PIC16 must send `0xA0 3`, not
`0xA0 1`, on the raw-wire route.

### `0xA7` record fields

The record layout is not just a byte count:

- **614 write** (4 bytes: marker, command, data low, data high): bit 7 of the
  command byte means **no payload**.  When it is set, only the six-bit command
  is clocked and the two payload bytes are ignored.
- **416 write** (4 bytes: marker, command, data low, data high): bits 7–6 of
  the command are a 0–3 ms delay that **replaces the fourth command bit**;
  the low four bits are the command.  The two payload bytes follow.
- **424 write** (5 bytes: marker, three payload bytes, NOP count): the NOP
  count is masked to the low four bits and that many NOPs are inserted after
  the SIX payload.
- **Reads:** 614/416 read records are marker + command (2 bytes); 424 is the
  marker alone (1 byte).  The device returns two bytes for a 614 or 424 read
  record and one byte for a 416 read record, after the script.

A minimal 614 write script is `0xA7 01 00 01 02 00 00`: one write record,
zero reads, marker `1`, command `0x02`, payload `0x0000`.

### Worked raw-wire exchange

```
host -> 0x00 x20        enter the binary layer
host <- "BBIO1"
host -> 0x05            enter raw wire
host <- "RAW1"
host -> 0xA0 0x03       select the 614 engine (PIC10/12/16)
host <- 0x01
host -> 0x4C            power + pull-ups on; AUX and CS low
host <- 0x01
host -> 0xA7 0x01 0x00 0x01 0x02 0x00 0x00
host <- 0x01            one write record: marker, command, payload
host -> 0x00            leave raw wire
host <- "BBIO1"
host -> 0x0F            return to the terminal
host <- 0x01
```

On v4, select the VPU source with the raw-wire `0x5x` command before `0x4C`
when the board is to feed the pull-ups; the shared RAW1 page owns those
operands.

- `0xA4` and `0xA5` are refused (`0x00`) in the 614 engine; select 416 or 424
  first.
- The 424 `0xA5` takes no command byte: it runs a fixed sequence and returns
  six bytes per count (three two-byte reads).
- The 424 write record's last byte is the number of NOPs inserted; the 424
  `0xA7` write record's fifth byte is the same count, masked to `0x0F`.
- `0xA7`'s script is a run of marker bytes: `1` = write record, `2` = read
  record.  Record sizes:

  | Engine | Write record | Read record |
  |---|---|---|
  | 416 | 4 bytes (marker + 3) | 2 bytes (marker + command) |
  | 614 | 4 bytes (marker + 3) | 2 bytes (marker + command) |
  | 424 | 5 bytes (marker + 4) | 1 byte (marker) |

- When the script has read records, the `0x01` success byte is sent before
  the read bytes; with write records only it is sent after the script.
- Any other byte in the `0xAx` group (`0xA1`–`0xA3`, `0xA6`, `0xA8`–`0xAF`)
  returns failure (`0x00`).

The raw-wire group is present even in a build without the PIC terminal mode;
it is not conditioned on the PIC support switch.  The `PIC1` entry (`0x07`)
is conditioned: without PIC support it returns the `BBIO1` identifier
without entering the loop.

## Limits

- Neither path has a part database or a complete programming algorithm.
  `PIC1` is a byte-wise bit engine; the raw-wire engines are fixed command
  interpreters.
- The `0xA4`/`0xA5`/`0xA7` script semantics are **unverified**.
- The raw-wire group's pull-up and VPU-source controls are the raw-wire
  group's own (`0x4x`/`0x5x`), not `PIC1`'s.

## v3 / v4

`PIC1` and the raw-wire group are present in both builds.
