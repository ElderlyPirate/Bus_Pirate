# Self-test — binary variants (`0x10` / `0x11`)

Part of the self-test documentation set (`docs/capabilities/selftest/`).  The
scriptable self-test carried by the binary bitbang layer, entered with `0x10`
(short) or `0x11` (full).  The interactive form is in
[terminal.md](terminal.md); the checks themselves are in [checks.md](checks.md);
the jumpers are in [wiring.md](wiring.md).

The binary layer's entry, framing and leave rules are owned by
[protocol/binary/](../../protocol/binary/README.md); this page covers only the
two self-test bytes.  The shared terminal command is in
[generic/commands.md](../../generic/commands.md).

## Names

This set calls the two variants **short** (`0x10`) and **full** (`0x11`).
Older material calls the full one the "long test".  The firmware prints no
variant name: both run with all text suppressed.

## Entry

1. Enter the bitbang layer from the terminal: 20 × `0x00` replies `BBIO1`.
   The entry sequence is in
   [generic/commands.md](../../generic/commands.md) and the
   [binary framing page][framing].
2. Send the command byte:

| Byte | Pass | Jumpers | Checks |
|---|---|---|---|
| `0x10` | short | none | every check except those marked `skipped` in the `0x10` column of [checks.md](checks.md) |
| `0x11` | full | the setup in [wiring.md](wiring.md) | every check |

The two bytes are self-test commands only in `BBIO1`.  In a sub-protocol the
`0x1x` group is bulk byte transfer, not the self-test.

The full pass does **not** prompt for the jumpers and does **not** wait for a
key.  Fit them before sending `0x11`; a missing jumper only shows up as a
larger count.  The short pass needs no jumpers.  Both passes need every target
disconnected and the board powered.

The binary bytes do not check the terminal mode.  They start from the state
`BBIO1` is in; the entry resets the pins but does not run the previous
terminal mode's cleanup.  Enter from HiZ for a clean baseline.

## The count

The firmware runs the pass with all text suppressed and replies with **one
raw byte**: the number of failed checks.  There is no leading success byte.

| Reply | Meaning |
|---|---|
| `0x00` | no check failed |
| `0x01`–`0x1B` | that many checks failed |
| `0x01` while the SMPS owns the ADC | refusal; no check ran |

The reply is a byte, not an ASCII digit.  It is the same count the terminal
run prints.  The maximum is the pass's check count: 27 on a v4 full pass, 22
short, 21 on a v3 full pass, 15 short.

A count of `0x01` is ambiguous on its own: one failed check and the SMPS
refusal look the same.  The refusal can only happen while the SMPS is
running; if you know its state, you know which byte you have.  To get
per-check text, leave the test with `0xFF`, leave the layer with `0x0F`
(which stops the SMPS), and run `~`.  The refusal is in
[checks.md](checks.md#when-the-run-does-not-start).

## The idle loop

After the count the test stays in an idle loop.  It has no timeout; the host
must leave it.

| Host sends | Reply | Meaning |
|---|---|---|
| any byte except `0xFF` | byte + error count, one byte, wrapping | the test is idle; the count is added to the byte |
| `0xFF` | `0x01` | leave the self-test |

The count is not re-sent as a fresh result.  Examples:

| Errors | Host sends | Reply |
|---|---|---|
| 0 | `0x41` | `0x41` |
| 2 | `0x41` | `0x43` |
| 2 | `0xFE` | `0x00` (wraps) |
| 2 | `0x00` | `0x02` (this looks like the count again) |
| any | `0xFF` | `0x01`, then back in `BBIO1` |

`0x0F` is **not** a leave byte here: it is echoed like any other data byte.
Only `0xFF` leaves the test.  After `0xFF` the firmware is back in `BBIO1`
without re-printing `BBIO1`, so the host must track the layer.  Send `0x0F`
to leave the bitbang layer; it replies `0x01`, then on v4 runs the terminal
reset (banner and `HiZ>`), on v3 a hardware reset.  The leave rules are in
the [binary framing page][framing].

## LED indication

- No errors: the MODE LED **blinks** while the test waits.
- One or more errors: the MODE LED is **on solid**.

The LED starts off when the pass is clean.  A host that sends `0xFF`
immediately after reading the count can leave before the first blink.  After
`0xFF` the LED is no longer updated; it is only a status signal while the
self-test is idle.

The manual LED prompt of the terminal run does not exist here: the binary
pass has no key prompts.

## Worked exchanges

Short pass, clean.  The entry is twenty NUL bytes.

```
host -> 0x00 x20         enter the bitbang layer
host <- "BBIO1"
host -> 0x10             short self-test
host <- 0x00             count 0: no check failed
host -> 0x41             any data byte while the test is idle
host <- 0x41             0x41 + 0 errors, echoed back
host -> 0xFF             leave the self-test
host <- 0x01             leave acknowledgement
```

The host is now in `BBIO1`.  A minimal host can skip the `0x41` line and send
`0xFF` straight after the count.  `0x0F` then leaves the layer.

Full pass with two failures (example):

```
host -> 0x00 x20         enter the bitbang layer
host <- "BBIO1"
host -> 0x11             full self-test; jumpers already fitted
host <- 0x02             count 2: two checks failed
host -> 0xFF             leave the self-test
host <- 0x01             leave acknowledgement
```

SMPS-owned ADC refusal (example, SMPS-enabled build):

```
host -> 0x00 x20         enter the bitbang layer
host <- "BBIO1"
host -> 0x05             raw wire
host <- "RAW1"
host -> 0xF2 0x00        start the SMPS; lowest reachable request 0x200
host <- 0x01             accepted
host -> 0x00             back to BBIO1; the SMPS keeps running
host <- "BBIO1"
host -> 0x10             short self-test
host <- 0x01             refusal count; no check ran
host -> 0xFF             leave the test
host <- 0x01             leave acknowledgement
host -> 0x0F             leave to the terminal
host <- 0x01             acknowledged
```

The v4 banner and `HiZ>` follow the last `0x01`; that terminal reset stops
the SMPS, so a later `~` runs normally.  The same count is returned by
`0x11`.  The refusal itself is a binary-layer condition: the terminal `~` at
`HiZ>` normally cannot see it, because reaching `HiZ>` already reset the
board.  The full sequence and the terminal recovery are in
[checks.md](checks.md#when-the-run-does-not-start).  This needs an
SMPS-enabled build; the default image cannot reproduce it.  See
[capabilities/smps/binary.md](../smps/binary.md) for the SMPS commands.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `0x10` / `0x11` | present, not verified on v3 | present |
| count and idle echo | same | same |
| jumpers for `0x11` | `Vpu`→`+5V`, `ADC`→`+3.3V` | `ADC`→`+3.3V` |
| leave `0x0F` | hardware reset | terminal reset, banner and `HiZ>` |
| SMPS refusal | — | optional build only |
| checks, full / short | 21 / 15 | 27 / 22 |

The v3 column is **not verified on v3**.

## Limits

- No per-check text.  The reply reports only the count, not which checks
  failed; use the terminal `~` run for that.
- `0x11` needs the setup jumpers.  Without them the extra checks fail and the
  count is larger; `0x10` is the jumper-free pass, but it still needs the
  target disconnected.
- The idle loop has no timeout.  A host that stops after reading the count
  leaves the board in the self-test, not in `BBIO1`.
- `0x10` / `0x11` are `BBIO1` commands only.  Do not send them inside a
  sub-protocol.
- The binary pass cannot select individual checks or skip the EEPROM block.
- After the count (and after `0xFF`) the header pins are left in HiZ, but the
  host is still in `BBIO1`.

## Related docs

- [terminal.md](terminal.md) — the interactive `~` run.
- [checks.md](checks.md) — the check reference and the SMPS refusal.
- [wiring.md](wiring.md) — the jumpers and the post-run state.
- [edge-cases.md](edge-cases.md) — failure and recovery cases.
- [protocol/binary/commands.md](../../protocol/binary/commands.md) — the
  `BBIO1` command map.
- [binary framing page][framing] — entry, `0x0F` and the v3 reset.

[framing]: ../../protocol/binary/entering-and-framing.md
