# I2C — macros

Part of the I2C documentation set (`docs/protocol/i2c/`).  This page owns the
mode macro list and the semantics of macros 1–4.  The scanner is described in
full in [bus-operations.md](bus-operations.md); the sniffer in
[sniffer.md](sniffer.md); the on-board chip geometry and the Windows tool in
[eeprom.md](eeprom.md).

## The `(0)` list

`(0)` prints the macro menu (v4 wording):

```
 0.Macro menu
 1.7bit address search
 2.I2C sniffer
 3.Connect to on-board EEPROM
 4.Enable Writing the on-board EEPROM
```

On a v3 build the menu lists only `0`–`2`.

| Macro | Description | Where |
|---|---|---|
| `(0)` | macro menu | both |
| `(1)` | 7-bit address search | both |
| `(2)` | I2C sniffer | both |
| `(3)` | connect to the on-board EEPROM | **v4 only** |
| `(4)` | clear the on-board EEPROM write protect | **v4 only** |
| other | `Unknown macro, try ? or (0) for help` | both |

## Macro 1 — 7-bit address search

See [bus-operations.md](bus-operations.md) §Address scanner.

## Macro 2 — I2C sniffer

See [sniffer.md](sniffer.md).

## Macro 3 — connect to the on-board EEPROM (v4)

Macro 3 switches start/stop/read/write and the address scanner to the internal
bus that carries the on-board EEPROM.  It requires the mode to have been
entered with **2 Hardware**:

```
I2C>(3)
Now using on-board EEPROM I2C interface
I2C>
```

In Software driver mode `(3)` is refused; no bus switch happens:

```
I2C>(3)
Onboard EEPROM requires hardware I2C
I2C>
```

Leave the mode and re-enter with `m4 2 <speed>` to use the on-board part.

The redirect lasts only the **current session**: leaving and re-entering the
mode, or resetting, returns to the external bus.  The sniffer is the exception
— it still samples the external SDA/SCL pins.  The write protect has its own
state (see macro 4).

On the internal bus, start/stop/read/write use the on-chip hardware engine at
hardware timing, not the software speed profile.

## Macro 4 — clear the on-board EEPROM write protect (v4)

The on-board EEPROM is write-protected by default.  Macro 4 clears that
protect:

```
I2C>(4)
On-board EEPROM write protect disabled
I2C>
```

- Macro 4 does **nothing** (no message, no state change) unless macro 3 was
  run first.
- Write protect is separate from the bus redirect: `(4)` clears it for the
  session; a reset or leaving and re-entering a mode re-asserts it.  Run `(3)`
  then `(4)` before any EEPROM write in a new session.

## Status

Macros 3 and 4 are **not verified**; they require a v4 with a populated
on-board EEPROM.  The chip is a 24xx64 — see [eeprom.md](eeprom.md).

## v3 / v4

| Macro | v3 | v4 |
|---|---|---|
| `(0)`, `(1)`, `(2)` | present | present |
| `(3)`, `(4)` | absent | present (not verified) |

Running `(3)` or `(4)` on a v3 build falls to the unknown-macro message.
The v3 column is **not verified**.
