# UART — the transparent bridge

Part of the UART documentation set (`docs/protocol/uart/`).  The two bridge
macros, their entry and exit rules, the overrun signal, and the host-side
baud caveat.  Macros `(2)` and `(4)` are in [macros.md](macros.md).

## `(1)` — transparent bridge

`(1)` turns the Bus Pirate into a **serial↔USB bridge**: every received word's
low eight data bits are forwarded to the host, and every byte the host sends
is forwarded to the target, 1:1 and unformatted.  The ninth data bit and the
parity/framing flags are not encoded into the stream.

```
UART>(1)
UART bridge
Normal to exit
Are you sure? y
```

- The line after `UART bridge` is `Normal to exit` on **v4** and
  `Reset to exit` on **v3**.
- The confirmation requires **`y` or `Y`** (case-insensitive).  Any other
  byte cancels and returns to the prompt — including a space.
- After the `y` the bridge prints nothing of its own; it is a pure byte pipe.

### Exiting

| Build | How to exit |
|---|---|
| **v4** | press the **NORMAL button** (the on-board button) |
| **v3** | hardware reset only (no button escape) |

On **v4** the exit prints a blank line and the `UART>` prompt returns.  The
UART port stays configured, so a word received after the exit stays in the
ring for the next `r` or display.  On **v3** the reset reboots the board to
`HiZ>`: the UART settings and any queued words are lost.

### Overruns during the bridge

If the target sends faster than the host drains, the **127-usable-word** ring
fills and words are dropped.  The overrun counter increments for both a
hardware FIFO overrun and a full ring, and in this bridge **any** counter
change turns the **MODE LED off** — the only visible signal (on v3 a
terminal-UART overrun also turns it off).  No message is printed and the link
does not stop — see [errors.md](errors.md).  The LED stays off until the mode
is re-entered.

On v4 the forward path checks host readiness before removing a word from the
ring, so a busy host does not trap the loop in a blocking CDC write.  That is
a **liveness aid, not flow control**: a stalled host can still fill and
overrun the finite ring.  On v3 the equivalent check is the terminal
transmitter's buffer-full flag.  Pace the traffic.

## `(3)` — bridge with flow control

`(3)` is the flow-control variant.  On **v3** it relays the two handshake
lines between the target and the host's serial chip; the firmware names them
`CTS` and `RTS`:

| Bus Pirate pin | Firmware name | Direction |
|---|---|---|
| CS | CTS | target side, passed to the host serial chip |
| CLK | RTS | host side, mirrored out to the target |

This relays the **host serial handshake**, not flow control on the target
UART.  On **v4** those pins are not wired to the host serial path, so `(3)`
sets **no flow control** and behaves exactly like `(1)`.  Treat `(3)` as a
v3 feature; on v4 use `(1)`.

## Host-side baud

The bridge's speed applies to the **target UART only**.  How the USB-side
link behaves depends on the build:

- **v3 — FTDI serial link.**  The FTDI chip and the Bus Pirate are joined by
  a real UART.  The Bus Pirate's side runs at its **terminal** rate —
  **115200 bps** by default — and the host must open the virtual serial port
  at the same rate; a mismatch makes the exchange **garbled**.  `b` changes
  the **Bus Pirate** side, not the computer side: after the rate selection it
  prints `Adjust your terminal` and `Space to continue`, switches to the new
  rate and waits about five seconds for a space.  Set the terminal to the new
  rate and press Space, then start the bridge at the target speed.  See
  [navigation.md](../../generic/navigation.md#terminal-baud--b) for the `b`
  menu.
- **v4 — USB CDC link.**  The rate is **host-controlled metadata**: the
  firmware does not reconfigure the link, and `b` prints
  `USB CDC baud rate is host-controlled.`  Opening the port at any rate does
  not garble the bridge — only the target UART speed matters.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| `(1)` transparent bridge | yes | yes |
| `(3)` flow-control bridge | relays CTS/RTS on CS/CLK | no flow control (same as `(1)`) |
| entry confirmation | `y`/`Y` | `y`/`Y` |
| exit | hardware reset | NORMAL button |
| host-side link | FTDI; `b` sets the Bus Pirate rate | CDC; host-controlled (`b` is a no-op) |
| overrun handling | any counted overrun turns the MODE LED off | same |
| forwarded data | low 8 data bits | low 8 data bits |

## Related docs

- [macros.md](macros.md) — the monitor `(2)` and auto-baud `(4)`.
- [terminal-usage.md](terminal-usage.md) — the formatted display.
- [errors.md](errors.md) — overrun counting and the LED.
- [wiring.md](wiring.md) — the v3 flow-control pins.
