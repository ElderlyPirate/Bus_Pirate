# UART — the transparent bridge

Part of the UART documentation set (`docs/protocol/uart/`).  The two bridge
macros, their entry/exit rules, and the host-side baud caveat.  Macros `(2)`
and `(4)` are in [macros.md](macros.md).

This page is separate from [macros.md](macros.md) on purpose: **use the
bridge `(1)`/`(3)` to move bulk data faster than the `[`/`{` display.**

## `(1)` — transparent bridge

`(1)` turns the Bus Pirate into a **serial↔USB bridge**: every byte on the
target UART is forwarded to the host, and every byte the host sends is
forwarded to the target, 1:1 and unformatted.

```
UART>(1)
UART bridge
Normal to exit
Are you sure?
```

- The line after `UART bridge` is `Normal to exit` on **v4** and
  `Reset to exit` on **v3**.
- The confirmation requires **`y` or `Y`** (it is case-insensitive).
  Any other key cancels and returns to the prompt.  The older note that a
  space continues is **wrong** for this firmware.
- Once inside, the Bus Pirate prints nothing of its own — it is a pure byte
  pipe.

### Exiting

| Build | How to exit |
|---|---|
| **v4** | press the **NORMAL button** (the on-board button) |
| **v3** | hardware reset only (no button escape) |

On exit a blank line is printed and the `UART>` prompt returns.

### Overruns during the bridge

If the target sends faster than the host drains, two things can happen.  A
**hardware FIFO overrun** is cleared automatically and turns the **MODE LED
off** — the only visible signal.  A full **128-byte software ring** also
drops bytes, but **silently**: no LED change, no message.  Neither stops the
link — see [errors.md](errors.md).

## `(3)` — bridge with flow control

`(3)` is the flow-control variant.  On **v3** it relays the two handshake
lines between the target and the host's serial chip:

| Bus Pirate pin | Signal | Direction |
|---|---|---|
| CS | CTS (target ready) | target → host |
| CLK | RTS (host ready) | host → target |

On **v4** those pins are not wired to the host serial path, so `(3)` sets
**no flow control** and behaves exactly like `(1)`.  Treat `(3)` as a v3
feature; on v4 use `(1)`.

## Host-side baud (v3 only)

The bridge's speed applies to the **target UART only**.  How the USB-side
link behaves depends on the build:

- **v3 — FTDI serial link.**  The link keeps its own rate — default
  **115200 bps** — unless you change it with the `b` menu.  If the host
  opens the virtual serial port at a different rate (say 9600) while
  expecting to talk to the bridge, the exchange is **garbled**, because the
  Bus Pirate still expects 115200 from the computer.  Adjust the
  computer-side rate first (`b`), then start the bridge at the target speed.
  See [commands.md](../../generic/commands.md) for the `b` menu.
- **v4 — USB CDC link.**  The rate is **host-controlled metadata**: the
  firmware does not reconfigure the link, and `b` prints
  `USB CDC baud rate is host-controlled.`  Opening the port at any rate does
  not garble the bridge — only the target UART speed matters.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `(1)` transparent bridge | yes | yes |
| `(3)` flow-control bridge | relays CTS/RTS on CS/CLK | no flow control (same as `(1)`) |
| entry confirmation | `y`/`Y` | `y`/`Y` |
| exit | hardware reset | NORMAL button |
| host-side link | FTDI; `b` sets the rate | CDC; host-controlled (`b` no-op) |
| overrun handling | FIFO overrun clears + MODE LED off; full ring silent | same |