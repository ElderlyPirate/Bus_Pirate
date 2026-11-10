# UART — macros

Part of the UART documentation set (`docs/protocol/uart/`).  The mode macro
list, the live monitor `(2)`, and auto-baud detection `(4)`.  The two bridges
are covered in [bridge.md](bridge.md).

The split is deliberate, not duplication: **use the bridge `(1)`/`(3)` or the
raw monitor `(2)` to move bulk data faster than the `[`/`{` display.**

## The `(0)` list

`(0)` prints the macro menu.  The wording differs slightly between builds
(the v4 entry `(4)` adds a parenthetical hint):

| Macro | v4 text | v3 text |
|---|---|---|
| `0` | `Macro menu` | `Macro menu` |
| `1` | `Transparent bridge` | `Transparent bridge` |
| `2` | `Live monitor` | `Live monitor` |
| `3` | `Bridge with flow control` | `Bridge with flow control` |
| `4` | `Auto Baud Detection (Activity Needed)` | `Auto Baud Detection` |

Any other number prints `Unknown macro, try ? or (0) for help`.

## `(1)` and `(3)` — bridges

See [bridge.md](bridge.md).  Briefly: `(1)` is the transparent
USB↔serial bridge; `(3)` is the same with the CTS/RTS handshake lines
relayed on v3 and is identical to `(1)` on v4.

## `(2)` — live monitor

`(2)` shows a continuous, **unformatted** display of received bytes — the
raw byte stream with no `READ:` framing.  It is the high-throughput
alternative to the `[`/`{` display.

```
UART>(2)
Raw UART input
Any key to exit
```

- **Any host key exits** (unlike the bridge, which needs a button or a
  reset).
- It **receives only** — it does not transmit to the bus.
- It hands bytes straight to the host, so it survives high rates that would
  overflow the formatted display's buffer; use a terminal that can render
  raw bytes.

Like the bridges, the monitor prints bytes to the host directly.  On both
builds the receive path tolerates a stalled host better than the formatted
display because it does less per byte (see
[terminal-usage.md](terminal-usage.md) §Receive buffer).

## `(4)` — auto-baud detection

`(4)` measures the rate of incoming traffic and reports it:

```
UART>(4)
Waiting activity...
Estimated:  9615 bps
Calculated: 9600 bps
UART>
```

- It waits for activity on the **MISO input**, measures the bit period
  across several samples, and prints both the estimated rate and the
  nearest common rate.
- If it cannot get a reading above the measurement ceiling it prints
  `** Baud>16m: The BP cannot measure above 16000000, Done.`
- Pressing a key during the wait aborts with `** Early Exit!`.
- **It only reports; it does not change the configured speed.**  To have a
  detected rate applied automatically, use the v4 entry-time speed option
  `11` (`Auto-Baud Detection (Activity Required)`) — see
  [entry-and-setup.md](entry-and-setup.md).
- It samples the **hardware MISO pin** (the full-duplex RX input).  In
  half-duplex single-wire mode the receive path is remapped onto the shared
  MOSI line, so MISO no longer sees the traffic and `(4)` is only useful in
  full duplex — see [hduart.md](hduart.md).

### `(4)` on v3

The auto-baud macro itself is not build-specific:

- the **macro `(4)` is listed and handled on both builds** — the menu text
  and the measurement routine are present in v3 and v4;
- what is v4-only is the **settings field** behind the entry-time option
  (`11`) and the **bps custom-baud** form, not the macro.

`(4)` on v3 is **not verified**.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `(1)` transparent bridge | yes | yes |
| `(2)` live monitor | yes | yes |
| `(3)` flow-control bridge | yes (CTS/RTS wired) | yes (no flow control — same as `(1)`) |
| `(4)` auto-baud macro | listed + handled (build-only) | listed + handled |
| entry-time auto-baud | — | speed option `11` |