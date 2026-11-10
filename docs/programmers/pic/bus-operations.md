# PIC — bus operations

Part of the PIC documentation set (`docs/programmers/pic/`).  This page owns
the on-bus behaviour: the Command/Data state selection, writes, reads, and the
fixed line timing.  The shared prompt grammar — value entry, repetition `:N`,
width `;N`, the wrappers, `(n)` macros — is owned by the generic layer; only
the PIC effect is described here.

## Command / Data selection — `[` and `]`

The two states are toggled with the transaction wrappers.  This is the spine
of the mode:

| Input | State entered | Firmware output |
|---|---|---|
| `[` | **Command** — writes are opcodes | `CMD6` or `CMD4` |
| `{` | same as `[` | `CMD6` or `CMD4` |
| `]` | **Data** — writes are payloads | `DTA` |
| `}` | same as `]` | `DTA` |

`[` and `{` are the same call; `]` and `}` are the same call.  The banner
digit is the **active opcode width**: `CMD6` in 6b/14b, `CMD4` in 4b/16b.  The
banner names the state, so a wrong-state write is visible in the transcript.

A fresh boot starts the mode in **Data** state; use `[` to begin a command
sequence.  The setup and cleanup paths do not reset the state, so if you left
the mode while in Command state, re-entry is still Command — send `]` and look
for `DTA` before writing payloads (see [edge-cases.md](edge-cases.md)).

## Writing a value

Any value written — `0xNN`, `0b…`, a decimal, or `"text"` — is clocked out
on PGD (MOSI) on PGC (CLK) edges.  What is sent depends on the state:

| State | Sent | Width |
|---|---|---|
| Command | the value's low bits as the programming **opcode** | 6 bits (6b/14b) or 4 bits (4b/16b) |
| Data | the value as the programming **payload** | 14 bits (6b/14b) or 16 bits (4b/16b) |

- In **6b/14b** data mode each payload is framed with a leading and trailing
  zero clock, as the mode's 14-bit frame requires.
- In **4b/16b** data mode the payload is sent as a plain 16-bit run.
- **PIC clocks LSB-first.**  Leave the generic bit order at the default
  (`l`).  `L` is not inert: the dispatcher reverses the value before the mode
  sends it and reverses a read after the mode returns it; the current `;N`
  width is the reversal width.  With `L` selected, both the wire pattern and
  the displayed read change.

The generic layer echoes each write as `WRITE: <value>`:

```
PIC>[
CMD6
PIC>0x00
WRITE: 0x00
```

## Reading a value

`r` reads a payload back on PGD (MOSI).  It is only meaningful in **Data**
state:

| State | `r` behaviour |
|---|---|
| Data | always clocks in 14 bits (6b/14b) or 16 bits (4b/16b), right-aligned |
| Command | refused — the firmware prints `no read` and a zero result |

The read width is fixed by the mode; `;N` does **not** narrow what is clocked
in from the target.  It sets the generic display mask (and the `L` reversal
width).  The mode enters with an 8-bit display, so a bare `r` shows only the
low 8 bits and appends `;8`; use `r;14` (6b/14b) or `r;16` (4b/16b) to see
the whole value.  A wider write echo works the same way: `0x1234;14` or
`0x1234;16`.

```
PIC>]
DTA
PIC>r
READ: 0x0034;8
PIC>r;14
READ: 0x1234;14
PIC>
```

The replies are an example; each `r` performs a fresh read.

`r:N` repeats the read N times on one `READ:` header (generic grammar).  A
read needs the PGD line pulled up so it has a defined idle level.

## The programming delay

A **delay of 0–3 ms** is inserted after **each command write** (not after a
data write, and not after a read).  It is the settle time some PIC parts need
between an opcode and the next operation.

The delay is set at **entry only** — there is no runtime terminal command to
change it.  The interactive `Delay?` prompt sets it (default 1 ms); the
one-line form applies the delay it is given (see
[entry-and-setup.md](entry-and-setup.md)).  In the binary `PIC1` protocol the
delay is changed with the `0x04`–`0x07` commands (see [binary.md](binary.md)).

## Line timing

The mode has **no speed menu**.  Each clock phase carries a fixed short delay
(about 25 µs while writing, about 50 µs while reading); the resulting line
rate is on the order of a few kHz.  This is intentional — ICSP is a slow,
timing-tolerant bus.

## Worked session

A short session on a target that is already in programming mode (pull-ups on,
VPU fed).  The device replies are an example:

```
PIC>[
CMD6
PIC>0x00
WRITE: 0x00
PIC>]
DTA
PIC>(1)
DevID = 0x0012 Rev = 0x1F
Please exit PIC programming mode
PIC>
```

The `[` puts the mode in Command state so `0x00` is clocked out as an opcode;
`]` returns to Data state.  The `(1)` macro reads the device ID (see
[macros.md](macros.md)) and then asks you to leave the mode.  A full
programming run — entry sequence, erase, page writes, verify — is driven by
the host tool, not typed by hand ([picprog.md](picprog.md)).

## v3 / v4

The Command/Data selection, write framing, read width and the delay are the
same in both builds.  Only the mode number and the `v` pin-row order differ
([entry-and-setup.md](entry-and-setup.md), [wiring.md](wiring.md)).  The v3
path is **not verified**.
