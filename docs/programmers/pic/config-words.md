# PIC — configuration and fuse words

Part of the PIC documentation set (`docs/programmers/pic/`).  This page owns
the configuration-word question: what the mode does, what the host must do,
and the lock-out risk.  The mode itself has no configuration command.

## What a configuration word is

PIC datasheets call these **configuration words** (or configuration bits).
"Fuse" is the AVR term; it is sometimes used loosely for the PIC bits, but
the value is a PIC configuration word.  The bits select the oscillator, the
watchdog, code protection, the reset-pin function, low-voltage programming
and other part options.

The Bus Pirate never decodes them.  The mode sends a configuration word as a
14-bit (6b/14b) or 16-bit (4b/16b) payload, just like a code word.

## The mode's role

There is no `config`, `fuse`, `erase` or `verify` command in PIC mode.  The
host:

1. enters programming mode (VPP for HVPP, MCLR at VDD for LVPP),
2. selects configuration space if the family has a separate one,
3. loads the word, begins the write and waits the part's programming time,
4. optionally reads it back,
5. leaves programming mode.

The exact opcodes, addresses and timing are in the part's programming
specification.  The table below is the six-bit map used by the common PIC16F
midrange parts; other six-bit families can differ.  Do not treat it as a
universal map.

| Step | Six-bit opcode | Payload |
|---|---|---|
| Load Configuration (sets the PC to `0x2000`, the first User ID word) | `0x00` | 14-bit zero |
| Load Data for Program Memory | `0x02` | 14-bit word |
| Load Data for Data Memory | `0x03` | 8-bit byte, zero-padded to 14 |
| Read Data from Program Memory | `0x04` | — (14-bit read) |
| Read Data from Data Memory | `0x05` | — (14-bit read; byte in low 8 bits) |
| Increment Address | `0x06` | — |
| Begin Erase/Programming Cycle | `0x08` | — (some parts: Begin Programming Only) |
| Bulk Erase Program Memory | `0x09` | — |
| Bulk Erase Data Memory | `0x0B` | — |

Some families add an externally timed end-programming command; the part's
programming specification says whether one is needed.  The PIC16F628A family
does not require one.  The mode does not insert the programming time; that is
the host's pause.

## A worked write — one configuration word

For the PIC16F627A/628A/648A class the configuration word is at **`0x2007`**.
The `0x00` load-configuration opcode starts at `0x2000` (the first User ID
word), so step the address forward before loading the word:

1. Send `0x00` and a dummy 14-bit payload.  The PC is now `0x2000`.
2. Send `0x06` seven times.  The PC is now `0x2007`.
3. Send `0x02` and the 14-bit configuration word.
4. Send `0x08` to begin the programming cycle; pause for the part's
   programming time.
5. Send `0x04` and read with `r` to check the word.

To read without writing, do steps 1–2, then `0x04` and `r`.  Keep the value
before changing anything.

The exact opcodes, addresses and timing are part-specific.  Some parts call
`0x08` Begin Programming Only and need a bulk erase first; others call it
Begin Erase/Programming.  A part can need a different entry, write opcode,
row-latch sequence, or address step.  Confirm it against its programming
specification.

## 4b/16b (PIC18) and 24-bit parts

On 4b/16b the same idea uses 16-bit payloads and the PIC18 table-write
sequence; the opcodes differ from the 6-bit table.  The mode's job is still
only to clock them.  PIC24/dsPIC parts use the raw-wire 24-bit engine; see
[binary.md](binary.md).

In `PIC1`, the same operation is a `0x40` opcode write, a `0x80` opcode plus
payload, or a `0xC0` opcode plus read, with the opcode in the low six bits
(for example `0x46` sends increment address `0x06`).  See
[binary.md](binary.md).

## The lock-out risk

A wrong configuration word can disable ICSP itself: code protection,
MCLR-as-I/O, the watchdog, an oscillator setting that stops the part, or
low-voltage programming disabled.  Some of these leave no ICSP recovery: the
part needs a high-voltage programmer, an erased part, or is locked.

- Do not write configuration words by hand unless you have the value and the
  part's specification.
- Read the word first and keep a copy.
- Use [`picprog`](picprog.md)'s part entry; it owns the masks and the safe values.
- A read-protected part may return a plausible word that is not the real
  configuration; the mode cannot detect protection.

The exact lock-out behaviour and recovery are part-specific and
**unverified** here.

## v3 / v4

The terminal sequence and the binary paths are the same in both builds.  The
host-side opcodes are not firmware behaviour.
