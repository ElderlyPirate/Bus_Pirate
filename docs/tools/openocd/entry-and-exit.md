# Entry and exit — `HiZ>` to `OCD1` and back

Part of the OpenOCD adapter set.  This page covers how the firmware path is
reached, what state it starts in, and how it returns to the terminal.  The
commands are in [commands.md](commands.md); TAP mechanics are in
[protocol/jtag/tap.md](../../protocol/jtag/tap.md).

## Scope

- Entry from the terminal prompt through `BBIO1` into `OCD1`.
- The state at entry.
- The two exit steps and what each one changes.
- The no-prompt cases; the full recovery table is in
  [limits-and-errors.md](limits-and-errors.md#leaving-and-recovery).

## Entry

There is no JTAG menu entry.  The path is reached from the binary layer:

| Step | Host sends | Board replies |
|---|---|---|
| 1 | 20 × `0x00` (NUL) | `BBIO1` |
| 2 | `0x06` | `OCD1` |

- The 20 NUL bytes must be consecutive; any other byte resets the count.
- `BBIO1` is five bytes and `OCD1` is four, with no line ending.
- A terminal program that sends only printable text cannot enter.  Use
  OpenOCD or a small raw-serial script.
- OpenOCD performs both steps itself.  Do not pre-enter `BBIO1`.

Both steps run the same BBIO pin reset: the 20-NUL entry switches VREG and
the on-board pull-ups off before it prints `BBIO1`, and `0x06` repeats the
reset before it prints `OCD1`.  A board that a terminal left with the
supplies on loses them at binary entry.

The binary layer's own entry rules are in the binary set:
[entering and framing](../../protocol/binary/entering-and-framing.md).

### State at entry

| Item | State after `0x06` |
|---|---|
| TDI, TMS, TCK, SRST | inputs (high-Z); latches low |
| TDO | input |
| TRST | not changed by entry; the pin mode takes it over |
| TCK delay | `1` |
| link rate | unchanged; 115200 at boot |
| mode LED | on after binary entry; not changed by `0x06` |
| regulator (VREG) | **off; the binary entry and `0x06` both switch it off** |
| on-board pull-ups | **off; the binary entry and `0x06` both switch them off** |
| Vpullup source (v4) | off (binary entry) |
| TAP state known to the firmware | none |
| saved TAP position | none; the host tracks it |

The five BBIO pins are `AUX`, `CS`, `MISO`, `CLK` and `MOSI`; they become
inputs with their latches low.  TRST is not one of them and is not part of
the reset.  The pin mode is **Hi-Z** until the host sends `0x01`.  SRST is an
input after entry, so its feature byte changes only a latch until a pin mode
makes it an output.  TRST is the exception: it is the v4 `AUX1` or the v3
`PGD` pin, which the binary reset does not touch; a pin mode takes it over
and drives it low.

There is no settings menu and no read-back: pin mode, features, delay and
rate are per-session bytes, and the host must track what it sent.  The
defaults in the table apply at entry; §Re-entering lists what a new entry
resets.

**Entry switches the regulator and the on-board pull-ups off.**  A session
started from a board that a terminal left with the supplies on therefore
starts with them off.  OpenOCD writes both features at init, so a stock
session reaches the configured state: on only when `buspirate vreg 1` or
`buspirate pullup 1` asks for it.

### Re-entering

`0x06` inside `OCD1` only prints `OCD1` again.  It does not reset the pin
mode, the features, the delay or the rate.  A fresh entry from `BBIO1`
returns to Hi-Z and delay `1` and switches VREG and the on-board pull-ups
off; the link rate keeps whatever state the terminal or the previous session
left.

## Exit

The clean exit is two steps because `0x0F` is not an `OCD1` command:

| Step | Host sends | Board replies | Effect |
|---|---|---|---|
| 1 | `0x00` | `BBIO1` | leaves `OCD1`; the five BBIO pins (`AUX`, `CS`, `MISO`, `CLK`, `MOSI`) become inputs; TRST keeps its last level; VREG and the on-board pull-ups switch off |
| 2 | `0x0F` | `0x01` | leaves `BBIO1`; v4: terminal reset and `HiZ>`; v3: hardware reset |

`0x00` already removes the target supply and releases the on-board pull-ups.
`0x0F` is for the return to the terminal, not for the supply.

A bare `0x00` does not release TRST.  The OpenOCD driver sends
`0x01 0x00` (Hi-Z) before it, which makes TRST an input as well.

After step 2 the board is back at the terminal.  On v4 the terminal reset
repeats the cleanup — the supply and the pull-ups are already off — clears the
`AUX1` open-drain bit, detaches the AUX output source, restores the AUX
assignment to `A0` and the display format, and prints the version banner and
`HiZ>`.  On v3 the hardware reset
performs the same cleanup by restarting the firmware; the link can emit
stray bytes while it comes back, so discard until the banner.

### `0x0F` inside `OCD1`

`0x0F` is answered as an unrecognised command with a single `0x00`; the mode
does not change.  A host that sends it as the only exit byte stays in
`OCD1`, and every later request is interpreted in the wrong context.  Send
`0x00` first, then `0x0F` from `BBIO1`.

### What the target sees

`0x00` does not reset the target's TAP.  The target changes state only if it
sees TCK/TMS or TRST.  If the chain must be left in a known state, walk it
to Test-Logic-Reset (five TCK with TMS high) or pulse TRST before leaving.

The Hi-Z step on the way out writes all five JTAG output latches low before
the pins become inputs, so SRST and TRST see a very short low pulse; the
target's own pull-ups restore the released level unless it holds them.

## If the host stops

| Stopped | Board state | Recovery |
|---|---|---|
| between commands | `OCD1` is waiting for a command byte | send `0x00`, then `0x0F` |
| inside a `0x05` shift | the firmware is still reading TDI/TMS payload | press reset, or finish the payload, then `0x00` and `0x0F` |

Mid-shift, `0x00` and `0x0F` are consumed as payload, not as commands.
Pressing reset is the reliable action.  The full table is in
[limits-and-errors.md](limits-and-errors.md#leaving-and-recovery).

## Related

| Doc | Why |
|---|---|
| [commands.md](commands.md) | the command set and the entry state's defaults |
| [overview.md](overview.md) | what the path is and the XSVF comparison |
| [../../protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and reply shapes |
| [../../protocol/jtag/entry.md](../../protocol/jtag/entry.md) | the JTAG set's entry page |
| [limits-and-errors.md](limits-and-errors.md) | recovery and driver messages |
