# PIC — bus operations

Part of the PIC documentation set (`docs/programmers/pic/`).  This page owns
the Command/Data states, the write and read framing, the display width and
the fixed timing.  The shared prompt grammar is in
[the generic commands page](../../generic/commands.md); the value model
(`;N`, `l`/`L`, repeat) is in [the generic values page](../../generic/values.md).

## Command and data state

The mode has two states.  The transaction wrappers select them:

| Input | State | Printed |
|---|---|---|
| `[` or `{` | **Command** — a value is an opcode | `CMD6` or `CMD4` |
| `]` or `}` | **Data** — a value is a payload | `DTA` |

`{` and `}` are aliases of `[` and `]` in this mode; PIC has no read-back
form.  The banner digit is the active opcode width: `CMD6` in 6b/14b,
`CMD4` in 4b/16b.

Entry and cleanup reset the state to **Data**.  Within a session it persists
until you send `[` or `]`; the banner is the reliable indicator.

## Writing a value

A value written in the current state is clocked out on PGD (MOSI) with PGC
(CLK) as the clock:

| State | Sent | Width |
|---|---|---|
| Command | the value's low bits as the programming **opcode** | 6 bits (6b/14b) or 4 bits (4b/16b) |
| Data | the value as the programming **payload** | 14 bits (6b/14b) or 16 bits (4b/16b) |

- In **6b/14b** data state each payload is framed with a leading and a
  trailing zero clock.
- In **4b/16b** data state the payload is a plain 16-bit run.
- The mode shifts **LSB-first** on the wire.  The generic bit-order setting
  is a separate layer: the default `l` (MSB-first, no reversal) leaves the
  value unchanged, so the mode's native LSB-first order goes out.  `L`
  reverses the value within the display width before the mode sees it, so the
  wire order becomes MSB-first.  Leave the generic setting at `l` (no
  reversal) unless the host sequence expects the reversal; the label
  describes the generic transformation, not the wire order in this mode.
  The full model is in [generic values](../../generic/values.md).
- The echo is `WRITE: <value>`.  In command state the echo is byte-wide but
  only the low 6 (or 4) bits reach the wire; keep opcodes in range.  In data
  state a 6b/14b write echoes with `;14`.

The mode has no address counter, no auto-increment and no transaction memory.
The part's address and increment opcodes are ordinary command writes; the
host sequences them.

```
PIC>[
CMD6
PIC>0x02
WRITE: 0x02
PIC>]
DTA
PIC>0x1234
WRITE: 0x1234;14
```

## Reading a value

`r` clocks a payload in on PGD (MOSI).  It is meaningful only in **Data**
state:

| State | `r` behaviour |
|---|---|
| Data | clocks in 14 bits (6b/14b) or 16 bits (4b/16b), right-aligned |
| Command | refuses: prints `no read`, returns 0 |

The transfer width is fixed by the state and the setup width.  `;N` does not
narrow or widen what is clocked in; it changes only the displayed value (and
the `L` reversal width).  A state change reasserts the mode's own width: a
`;N` set before `[` or `]` is replaced by 8 bits in Command state or 14/16
bits in Data state.  A bare `r` in data state already shows the full value:

```
PIC>]
DTA
PIC>r
READ: 0x1234;14
PIC>r;8
READ: 0x34
PIC>r;16
READ: 0x1234
```

The first read is 6b/14b; the `;14` is the display width marker.  In 4b/16b
the display is 16 bits by default and prints no `;16` suffix.  The values
are examples; each `r` performs a fresh read.

The first sampled bit is the least-significant bit of the returned value,
matching the mode's LSB-first shifter.  `r:N` repeats the read N times under
one `READ:` header.

A read in Command state prints the refusal line and a zero result:

```
PIC>[
CMD6
PIC>r
READ: no read
0x00
PIC>
```

## The command delay

A delay of **0–3 ms** is inserted after **each command write** — not after a
data write and not after a read.  It is the settle time some parts need
between an opcode and the next operation.

The delay is set at entry only; there is no terminal command to change it.
The interactive `Delay?` prompt sets it (default 1 ms); the one-line form
applies the value it is given.  In `PIC1` the delay is set with `0x04`–`0x07`
(see [binary.md](binary.md)).

The delay is **not** the part's programming time.  Erase and program cycles
take milliseconds to tens of milliseconds; the mode does not wait for them.
Pause for the part's specified time, or let the host tool do it.

## Line timing

There is no speed menu.  The mode uses a fixed 100 µs pin delay: about 100 µs
per bit (roughly 10 kHz).  A 14-bit payload is 16 clocks including the
leading and trailing framing zeros, about 1.6 ms, plus the command delay.
ICSP is a slow, timing-tolerant interface; the fixed rate is intentional.

## Terminal command reference

| Input | Action | Notes |
|---|---|---|
| `[` / `{` | command state | prints `CMD6` / `CMD4` |
| `]` / `}` | data state | prints `DTA` |
| `<value>` | clock out the value | `WRITE: …`; width follows the state |
| `r` | clock in a payload | data state only; `READ: …` |
| `r:N` | repeat the read N times | one `READ:` header |
| `<value>:N` | repeat the write N times | generic repeat |
| `(0)` | list macros | `(1) get devID` |
| `(1)` | device-ID sequence | 6b/14b only; see [macros.md](macros.md) |
| `v` | pin state row | PGC / PGD columns |
| `W` / `w` | target rails on / off | generic |
| `e` (v4) | VPU source | generic |
| `P` / `p` | pull-ups on / off | generic |
| `m` / `m1` | change mode / back to HiZ | leaves the mode |

**Not in this mode:** erase, program, verify, configuration-word, bulk or
speed commands.  Those are host-side sequences built from the opcode and
payload writes; see [config-words.md](config-words.md) and
[sessions.md](sessions.md).

## v3 / v4

The state selection, write framing, read width, display and delay are the
same in both builds.  Only the mode number and the `v` row order differ.
