# I2C — macros

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns the
mode macro list and the semantics of macros 1–4. The scanner's format and the
sniffer's output are described in [bus-operations.md](bus-operations.md) and
[sniffer.md](sniffer.md); the on-board chip's geometry and the Windows tool are
in [eeprom.md](eeprom.md). The `(n)` grammar itself is shared and is owned by
[docs/generic/macros.md](../../generic/macros.md).

## The `(0)` list

`(0)` prints the macro menu. On v4:

```
 0.Macro menu
 1.7bit address search
 2.I2C sniffer
 3.Connect to on-board EEPROM
 4.Enable Writing the on-board EEPROM
```

On a default v3 build the list stops after `2.I2C sniffer`.

| Macro | Firmware text | Availability |
|---|---|---|
| `(0)` | `Macro menu` | both |
| `(1)` | `7bit address search` | both |
| `(2)` | `I2C sniffer` | both |
| `(3)` | `Connect to on-board EEPROM` | v4 only |
| `(4)` | `Enable Writing the on-board EEPROM` | v4 only |
| any other | `Unknown macro, try ? or (0) for help` | both |

**Name mapping.** The menu calls macro 4 `Enable Writing the on-board EEPROM`;
its effect is to clear the on-board EEPROM's write protect, and the confirmation
line is `On-board EEPROM write protect disabled`. This set uses the menu text as
the macro's name and the effect wording when describing the state.

## Macro 1 — `7bit address search`

Scans every 8-bit address and lists the hits. The format, the short/no-pull-up
pre-check, the display-format rule and the read-address handling are in
[bus-operations.md](bus-operations.md#address-scanner--macro-1).

If a read ACK is pending when `(1)` is issued, the macro first sends NACK and a
stop so the scan starts on an idle bus.

## Macro 2 — `I2C sniffer`

Listens to the external SDA/SCL pins until a key is pressed. The output format,
the queue and the limits are in [sniffer.md](sniffer.md).

If a read ACK is pending when `(2)` is issued, the macro first sends NACK and a
stop, then releases the pins for sniffing. In the Hardware driver it re-runs the
hardware setup when the sniffer exits, so bus operations work again without a
mode change.

## Macro 3 — `Connect to on-board EEPROM`

Macro 3 switches start/stop/read/write and the address scanner to the internal
bus that carries the on-board EEPROM. It requires the mode to have been entered
with **2 Hardware**:

```
I2C>(3)
Now using on-board EEPROM I2C interface
I2C>
```

In the Software driver `(3)` is refused; no bus switch happens:

```
I2C>(3)
Onboard EEPROM requires hardware I2C
Syntax error at char 3
I2C>
```

The refusal also flags a command error, which is why the syntax-error line
follows. Leave the mode and re-enter with `m4 2 <speed>` to use the on-board
part.

Behaviour:

- If a transaction is active, the macro sends a stop first.
- It disables the external I2C module and sets up the internal one at the
  session's **hardware** rate. The software speed profile is irrelevant after
  the redirect.
- The redirect covers start/stop/read/write and the address scanner. The
  **sniffer is the exception**: it still samples the external SDA/SCL pins.
- The redirect lasts only the **current session**. Leaving and re-entering the
  mode, or resetting, returns to the external bus.
- The on-board chip, its 3.3 V supply and its pull-ups are on the board, so the
  redirect needs no external wiring, no Vpullup feed and no `W`. The chip's
  geometry and address sequence are in [eeprom.md](eeprom.md).

## Macro 4 — `Enable Writing the on-board EEPROM`

The on-board EEPROM is write-protected by default. Macro 4 clears that protect:

```
I2C>(4)
On-board EEPROM write protect disabled
I2C>
```

- Macro 4 does **nothing** — no message, no state change — unless macro 3
  has been run in this session.
- The write protect is separate from the bus redirect. `(4)` clears it for the
  session; a reset or leaving and re-entering a mode re-asserts it. Run `(3)`
  then `(4)` before any EEPROM write in a new session.
- Clearing the protect does not itself write anything; it only allows the next
  write to the internal chip.

## Unknown macro

Any value that is not in the list prints
`Unknown macro, try ? or (0) for help`. Unlike the macro 3 refusal, this does
not flag a command error, so no syntax-error line follows.

On a default v3 build `(3)` and `(4)` fall into this case.

## Edge cases

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `(3)` in the Software driver | `Onboard EEPROM requires hardware I2C` plus a syntax error | re-enter with `m4 2 <speed>` |
| `(4)` before `(3)` | nothing at all | run `(3)` first |
| `(3)` is run, then the mode is left and re-entered | the external bus again | the redirect is per session |
| `(3)`/`(4)` on a default v3 build | `Unknown macro, try ? or (0) for help` | v4 only |
| `(5)` or a larger number | `Unknown macro, try ? or (0) for help` | macros are 0–4 (0–2 on v3) |
| `(1)` or `(2)` with a read ACK pending | `NACK`, `I2C STOP BIT`, then the macro | the macro finishes the read first |
| `(3)` when already on the internal bus | a stop if a transaction is open, then the redirect message | re-running is safe |
| The scanner after `(3)` | it scans the internal bus | that is the redirect working |
| The sniffer after `(3)` | it shows external traffic, usually nothing | the sniffer does not follow the redirect |
| `(4)` is run, then the mode is re-entered | the chip is write-protected again | run `(3)` and `(4)` in the new session |

## v3 / v4

| Macro | v3 | v4 |
|---|---|---|
| `(0)`, `(1)`, `(2)` | yes | yes |
| `(3)`, `(4)` | no — unknown macro | yes |
| on-board EEPROM chip | not present | 24xx64 on the internal bus |

The v3 column is **not verified**.

## Related docs

- [bus-operations.md](bus-operations.md) — the scanner format and rules.
- [sniffer.md](sniffer.md) — the macro 2 output and limits.
- [eeprom.md](eeprom.md) — the on-board chip and the Windows tool.
- [binary.md](binary.md) — the binary `0x0F` sniffer and the command set.
