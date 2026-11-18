# UART — parity, framing and overrun

Part of the UART documentation set (`docs/protocol/uart/`).  How the receive
faults are detected and reported, how the buffer behaves, and how to recover.
The buffer is introduced in [terminal-usage.md](terminal-usage.md).

## Parity and framing flags

The receiver records a parity error and/or a framing error for each word.
When such a word is read, the firmware prints a flag:

| Flag | Meaning | Typical cause |
|---|---|---|
| `-p ` | parity error on the word | parity mode mismatched, or a corrupted bit |
| `-f ` | framing error on the word | baud-rate mismatch, wrong stop bits, a break, or a glitch |

Where the flag appears depends on how the word is read:

- with `r`, the flag comes **before** the value:
  `READ: -p 0x40`; with both flags `READ: -p -f 0x40`.
- in the live display (`[`/`{`), the flag comes **after** the value and is
  printed hard against it: `READ: 0x40-f`; with both flags
  `READ: 0x41-p -f`.  The flag strings end with a space, but there is no space
  between the value and the first flag.

Both flags can appear together.  A word is still delivered — the flags mark
it, they do not stop the mode.

**Recovery:** a framing error almost always means the two ends disagree on
speed (or frame format).  Check the selected speed and the data
bits/parity/stop settings against the sender.  A parity error with a correct
frame means the parity mode is mismatched, or one bit was corrupted.  One
stray `-f` at link start-up or reset is normal line noise.

## Overruns

The [receive ring](terminal-usage.md#receive-buffer-and-overruns) stores each
word with its ninth data bit and its parity/framing flags.  Two events
increment the same overrun counter:

- **Hardware FIFO overrun** — the FIFO was full before the interrupt could
  drain it.  The interrupt clears it and increments the counter.
- **Full software ring** — the ring cannot accept the word; the word is read
  and discarded, and the counter is incremented.

The counter counts **events** since the port was last enabled.  It is **not a
lost-character count**: one hardware-overrun event can correspond to more
than one missing character.  The counter resets whenever the port is enabled
— after `W`, after a binary speed/frame change, and when `(4)` re-runs the
setup.  **The count is internal: no command or status field prints it.**

Outside a bridge/monitor nothing is printed and no LED changes; no
`*Bytes dropped*` message is produced.  In the **terminal bridge `(1)`**, the
**live monitor `(2)`** and the **`ART1` bridge `0x0F`**, **any** counter change
turns the **MODE LED** off — the only visible signal.  The MODE LED is the
LED the firmware turns on when a non-HiZ mode is entered (not the USB LED).
No message is printed and the link does not stop.  The LED stays off until the
mode is re-entered.

On **v3** the terminal bridge also clears the terminal UART's own overrun
flag and turns the MODE LED off for it.  Separately, v3 prints
`WARNING: Possible buffer overflow` at setup when the target baud is above
the terminal link rate (and again after `(4)`); slow the target or raise the
terminal rate.

**Recovery / avoidance:**

- Use the **transparent bridge `(1)`** or the **live monitor `(2)`**.  They do
  less per word than the formatted display and check host readiness before
  dequeuing, but the 127-word ring is still finite; pacing is not optional.
- Reduce the host-side polling interval, or slow the sender.
- After an overrun, the link is still alive.  Read out what is queued and
  continue; nothing needs to be reset.

## Failed reads

`r` (or `r:N`) with nothing waiting prints:

```
UART>r
READ: FAILED, NO DATA0x00
```

This is not a bus error — the receive buffer was empty when asked.  The
no-data token has no trailing newline, and the prompt then prints the zero
the read returns; the result is the glued `0x00`.  With `o2` (DEC) the tail is
`0`; with `o3` (BIN) it is `0b00000000`; with `o4` (RAW) it is an invisible NUL
byte.  In 9-bit mode the tail follows the width, for example `0x0000;9`.

A counted read that runs out of words mixes values and tokens:

```
UART>r:4
READ: 0x41 FAILED, NO DATA0x00 FAILED, NO DATA0x00 FAILED, NO DATA0x00
```

Wait for the sender and read again, or open the live display so words are
shown as they arrive.

## Auto-baud messages

These appear while measuring a rate (entry option `11` on v4, or the `(4)`
macro), not as data errors:

| Message | Meaning |
|---|---|
| `Baud detection selected..` | option `11` recorded; the measurement runs at `W` |
| `Waiting activity...` | listening for edges on the active receive input (MISO in full duplex, MOSI in half duplex) |
| `Estimated:  <n> bps` | measured rate |
| `Calculated: <n> bps` | nearest common rate from the measurement |
| `** Baud>16m: The BP cannot measure above 16000000, Done.` | the measured rate exceeded the 150 kbaud check |
| `** Early Exit!` | a key was pressed, the five-second window expired, or the measurement timer was busy; aborted |

See [macros.md](macros.md) and [entry-and-setup.md](entry-and-setup.md).

## Flag cautions

- Clean traffic at a matched speed shows **no** flags.
- A rate mismatch between the two ends produces `-f` framing errors; this is
  not a firmware fault.  At 115200 a 16 MHz-derived peer divider truncates:
  it lands at about 125 kbaud against the preset's about 114.3 kbaud, an
  about 9 % mismatch — use a matched peer (see [hduart.md](hduart.md)
  §Speeds).
- The flag wording and the before/after position rule are per the firmware
  wording; the `-p` (parity) case is **not verified**.
- Flags are per word and survive the ring: the flag belongs to the word that
  was queued with it, not to the one read after it.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| `-f` / `-p` reporting | same | same |
| overrun message | none (silent outside a bridge) | none (silent outside a bridge) |
| receiving buffer | 127 usable words | 127 usable words |
| overrun LED (bridge/monitor/`ART1`) | any counter change | any counter change |
| rate above the terminal link | `WARNING: Possible buffer overflow` | — |
| terminal output buffer | 4096 bytes | 4096 bytes |

## Related docs

- [terminal-usage.md](terminal-usage.md) — the receive ring and examples.
- [bridge.md](bridge.md) — overruns during a bridge.
- [macros.md](macros.md) — the monitor and auto-baud.
- [edge-cases.md](edge-cases.md) — concrete failure modes.
