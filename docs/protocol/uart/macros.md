# UART — macros

Part of the UART documentation set (`docs/protocol/uart/`).  The mode macro
list, the live monitor `(2)`, and auto-baud detection `(4)`.  The two bridges
are covered in [bridge.md](bridge.md).

## The `(0)` list

`(0)` prints the macro menu.  The wording differs between builds:

| Macro | v4 text | v3 text |
|---|---|---|
| `0` | `Macro menu` | `Macro menu` |
| `1` | `Transparent bridge` | `Transparent bridge` |
| `2` | `Live monitor` | `Live monitor` |
| `3` | `Bridge with flow control` | `Bridge with flow control` |
| `4` | `Auto Baud Detection (Activity Needed)` | `Auto Baud Detection` |

```
UART>(0)
 0.Macro menu
 1.Transparent bridge
 2.Live monitor
 3.Bridge with flow control
 4.Auto Baud Detection (Activity Needed)
UART>
```

Any other number prints `Unknown macro, try ? or (0) for help`.

## `(1)` and `(3)` — bridges

See [bridge.md](bridge.md).  `(1)` is the transparent USB↔serial bridge;
`(3)` is the same with the host serial handshake lines relayed on v3, and is
identical to `(1)` on v4.

## `(2)` — live monitor

`(2)` shows a continuous, **unformatted** display of received bytes — the raw
byte stream with no `READ:` framing.  It is the high-throughput alternative
to the `[`/`{` display.

```
UART>(2)
Raw UART input
Any key to exit
```

- **Any host key exits.**  Unlike the bridge, it does not need a button or a
  reset.  On v4 the NORMAL button also exits; on v3 the host key is the only
  exit.
- The exit key is consumed by the monitor and is not echoed.
- It **receives only** — it does not transmit to the bus.
- It forwards only the **low eight data bits**; the ninth data bit and the
  error flags are not encoded into the stream.
- On v4 it checks host readiness before removing a word from the ring, so a
  busy host does not stall the loop; on v3 the equivalent check is the
  terminal transmitter's buffer-full flag.  That is a **liveness aid, not
  flow control**: the 127-word ring is finite and can still overrun.  Pace
  the traffic.
- It does less per byte than the formatted display and is the safer choice
  for a fast stream; use a terminal that can render raw bytes.

The prompt returns after the exit.  The monitor does not print a stop
message of its own.

## `(4)` — auto-baud detection

`(4)` measures the rate of incoming traffic and reports it:

```
UART>(4)
Waiting activity...

Estimated:  9615 bps
Calculated: 9600 bps
UART>
```

- It waits for activity on the **active receive input** — MISO in full
  duplex, the shared MOSI line in half duplex — measures the bit period
  across several samples, and prints both the estimated rate and the nearest
  common rate.
- **It only reports; it does not change the configured speed.**  To have a
  detected rate applied automatically, use the v4 entry-time speed option
  `11` (`Auto-Baud Detection (Activity Required)`) — see
  [entry-and-setup.md](entry-and-setup.md).
- `Calculated:` is picked from a wider table than the menu and can report
  rates the menu does not offer: 600, 14400, 28800, 56000, 128000, and
  31250 bps.
- A key press during the wait aborts with `** Early Exit!`; so does a
  five-second window with no usable activity, or a measurement timer that is
  already in use.
- Above about 150000 bps neither line is printed; the firmware prints
  `** Baud>150k: The BP cannot measure above 150000, Done.`
- It follows the **active receive path**, so it also works in half-duplex
  single-wire mode — have the peer transmit during the wait.  See
  [hduart.md](hduart.md).
- If the port was already engaged, `(4)` re-runs the mode setup at the
  current speed afterwards.  That clears any words already queued in the
  receive ring.

### `(4)` on v3

The auto-baud macro itself is not build-specific:

- the **macro `(4)` is listed and handled on both builds** — the menu text
  and the measurement routine are present in v3 and v4;
- what is v4-only is the **entry-time option `11`** and the **bps custom-baud
  form**, not the macro.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| `(1)` transparent bridge | yes | yes |
| `(2)` live monitor | yes | yes |
| `(3)` flow-control bridge | yes (CTS/RTS on CS/CLK) | yes (no flow control — same as `(1)`) |
| `(4)` auto-baud macro | listed + handled | listed + handled |
| entry-time auto-baud | — | speed option `11` |
| `(4)` menu text | `Auto Baud Detection` | `Auto Baud Detection (Activity Needed)` |
| monitor exit | host key | host key or NORMAL button |

## Related docs

- [bridge.md](bridge.md) — `(1)` and `(3)`.
- [entry-and-setup.md](entry-and-setup.md) — the entry-time auto-baud.
- [hduart.md](hduart.md) — auto-baud on the shared wire.
- [terminal-usage.md](terminal-usage.md) — the formatted display.
