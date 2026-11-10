# UART — parity, framing and overrun

Part of the UART documentation set (`docs/uart/`).  How the three receive
faults are detected and reported, and how to recover.  Buffer behaviour is
introduced in [terminal-usage.md](terminal-usage.md).

## Parity and framing flags

The receiver records a parity error and/or a framing error for each byte.
When such a byte is read, the firmware prints a flag next to it:

| Flag | Meaning | Typical cause |
|---|---|---|
| `-p ` | parity error on the byte | parity setting mismatched, or a corrupted bit |
| `-f ` | framing error on the byte | baud-rate mismatch, wrong stop bits, a break, or a glitch |

Where the flag appears depends on how the byte is read:

- with `r`, the flag comes **before** the value:
  `READ: -p 0x40`
- in the live display (`[`/`{`), the flag comes **after** the value:
  `READ: 0x40 -f`

Both flags can appear together.  A byte is still delivered — the flags mark
it, they do not stop the mode.

**Recovery:** a framing error almost always means the two ends disagree on
speed (or frame format).  Check the selected speed and the data
bits/parity/stop settings against the sender.  A parity error with a
correct frame means the parity mode is mismatched (or one bit was
corrupted).  One stray `-f` at a link start-up or reset is normal line
noise.

## Overruns

The receiver path is a small hardware FIFO backed by a **128-byte software
buffer**.  When that buffer fills before the host reads it:

- the new byte is **dropped**;
- an internal drop counter is incremented;
- **no terminal message is printed.**

This is a change from older firmware: the `*Bytes dropped*` line and the
immediate overrun check no longer exist.  Overruns are cleared so the
port keeps running, but they are silent outside the bridge.

**Recovery / avoidance:**

- Use a **transparent bridge `(1)`** or the **raw live monitor `(2)`**, which
  stream bytes straight to the host and survive rates the formatted display
  cannot (see [macros.md](macros.md)).
- In the bridge, an overrun turns the **MODE LED off** as the only visible
  signal (see [bridge.md](bridge.md)).
- Reduce the host-side polling interval, or slow the sender.

## Failed reads

`r` (or `r:N`) with nothing waiting prints:

```
READ: FAILED, NO DATA
```

This is not a bus error — it just means the receive buffer was empty when
asked.  Wait for the sender to transmit and read again, or open the live
display so bytes are shown as they arrive.

## Auto-baud and measurement messages

These appear while measuring a rate (entry option `11` on v4, or the `(4)`
macro), not as data errors:

| Message | Meaning |
|---|---|
| `Waiting activity...` | listening for edges on the MISO receive input |
| `Estimated:  <n> bps` | measured rate |
| `Calculated: <n> bps` | nearest common rate from the measurement |
| `** Baud>16m: The BP cannot measure above 16000000, Done.` | the measured rate exceeded the ceiling |
| `** Early Exit!` | a key was pressed during the wait; aborted |

See [macros.md](macros.md).

## Flag cautions

- Clean traffic at a matched speed shows **no** flags.
- A rate mismatch between the two ends produces `-f` framing errors; this
  is not a firmware fault.  At 115200 a 16 MHz-derived peer divider
  truncates: it lands at ≈ 125 kbaud against the preset's ≈ 114.3 kbaud,
  an ≈ 9 % mismatch — use a matched peer (see [hduart.md](hduart.md)
  §Speeds).
- The flag wording and the before/after position rule are per the firmware
  wording; the `-p` (parity) case is **not verified**.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `-f` / `-p` reporting | same | same |
| overrun message | none (silent) | none (silent) |
| receiving buffer | 128-byte software buffer | 128-byte software buffer |

The v3 column is **not verified**.