# PC-AT keyboard (KEYB) — scancodes

Part of the KEYB documentation set (`docs/protocol/keyboard/`).  This page is a
lookup aid for the bytes the mode prints.  The mode itself does not decode or
translate them; the read commands are in [usage.md](usage.md).

## Scope

How to read a make/break sequence, the standard PC-AT Set 2 roster and code
index, the Set 1 difference, and the shared prompt settings that change the
printed form.  It does not teach the keyboard protocol
([protocol.md](protocol.md)) or the commands ([usage.md](usage.md)).

## Make and break

A keyboard sends a **make** code when a key goes down and a **break** code
when it goes up.  A PC-AT or PS/2 keyboard normally uses **Set 2**:

| Event | Bytes |
|---|---|
| press `A` | `1C` |
| release `A` | `F0 1C` |
| press `B` | `32` |
| release `B` | `F0 32` |
| press Up arrow | `E0 75` |
| release Up arrow | `E0 F0 75` |

In Set 2, a break is `F0` followed by the same make code.  An **extended** key
starts with `E0`; its break is `E0 F0` followed by the code after the `E0`.
Some keys are longer sequences (`Pause`, `Print Screen`).

Set 1, the older XT set, uses different make codes and marks a release by
setting bit 7: press `A` is `1E`, release `A` is `9E`.  The mode prints
whatever the keyboard sends; it does not select or convert a set.

## Set 2 roster

These are the standard Set 2 **make** codes.  For a break, add `F0` after any
`E0` prefix and before the base code.

### Code index

The key-name tables below answer "what does `A` send?".  This index answers
"what is `5A`?" for a stream.  Codes are in numeric order; a prefix is part of
the code.

| Code | Key | Code | Key |
|---|---|---|---|
| 01 | `F9` | 03 | `F5` |
| 04 | `F3` | 05 | `F1` |
| 06 | `F2` | 07 | `F12` |
| 09 | `F10` | 0A | `F8` |
| 0B | `F6` | 0C | `F4` |
| 0D | `Tab` | 0E | `` ` `` |
| 11 | `Left Alt` | 12 | `Left Shift` |
| 14 | `Left Ctrl` | 15 | `Q` |
| 16 | `1` | 1A | `Z` |
| 1B | `S` | 1C | `A` |
| 1D | `W` | 1E | `2` |
| 21 | `C` | 22 | `X` |
| 23 | `D` | 24 | `E` |
| 25 | `4` | 26 | `3` |
| 29 | `Space` | 2A | `V` |
| 2B | `F` | 2C | `T` |
| 2D | `R` | 2E | `5` |
| 31 | `N` | 32 | `B` |
| 33 | `H` | 34 | `G` |
| 35 | `Y` | 36 | `6` |
| 3A | `M` | 3B | `J` |
| 3C | `U` | 3D | `7` |
| 3E | `8` | 41 | `,` |
| 42 | `K` | 43 | `I` |
| 44 | `O` | 45 | `0` |
| 46 | `9` | 49 | `.` |
| 4A | `/` | 4B | `L` |
| 4C | `;` | 4D | `P` |
| 4E | `-` | 52 | `'` |
| 54 | `[` | 55 | `=` |
| 58 | `Caps Lock` | 59 | `Right Shift` |
| 5A | `Enter` | 5B | `]` |
| 5D | `\` | 66 | `Backspace` |
| 69 | `keypad 1` | 6B | `keypad 4` |
| 6C | `keypad 7` | 70 | `keypad 0` |
| 71 | `keypad .` | 72 | `keypad 2` |
| 73 | `keypad 5` | 74 | `keypad 6` |
| 75 | `keypad 8` | 76 | `Esc` |
| 77 | `Num Lock` | 78 | `F11` |
| 79 | `keypad +` | 7A | `keypad 3` |
| 7B | `keypad -` | 7C | `keypad *` |
| 7D | `keypad 9` | 7E | `Scroll Lock` |
| 83 | `F7` | | |

Prefixed codes, also in numeric order:

| Code | Key | Code | Key |
|---|---|---|---|
| E0 11 | `Right Alt` | E0 14 | `Right Ctrl` |
| E0 1F | `Left GUI` | E0 27 | `Right GUI` |
| E0 2F | `Menu` | E0 4A | `keypad /` |
| E0 5A | `keypad Enter` | E0 69 | `End` |
| E0 6B | `Left` | E0 6C | `Home` |
| E0 70 | `Insert` | E0 71 | `Delete` |
| E0 72 | `Down` | E0 74 | `Right` |
| E0 75 | `Up` | E0 7A | `Page Down` |
| E0 7D | `Page Up` | | |

Longer sequences:

| Code | Key |
|---|---|
| E0 12 E0 7C | `Print Screen` |
| E1 14 77 E1 F0 14 F0 77 | `Pause` |

### Letters

| Key | Make | Key | Make | Key | Make | Key | Make |
|---|---|---|---|---|---|---|---|
| `A` | 1C | `B` | 32 | `C` | 21 | `D` | 23 |
| `E` | 24 | `F` | 2B | `G` | 34 | `H` | 33 |
| `I` | 43 | `J` | 3B | `K` | 42 | `L` | 4B |
| `M` | 3A | `N` | 31 | `O` | 44 | `P` | 4D |
| `Q` | 15 | `R` | 2D | `S` | 1B | `T` | 2C |
| `U` | 3C | `V` | 2A | `W` | 1D | `X` | 22 |
| `Y` | 35 | `Z` | 1A | | | | |

### Number row and punctuation

| Key | Make | Key | Make | Key | Make | Key | Make |
|---|---|---|---|---|---|---|---|
| `1` | 16 | `2` | 1E | `3` | 26 | `4` | 25 |
| `5` | 2E | `6` | 36 | `7` | 3D | `8` | 3E |
| `9` | 46 | `0` | 45 | `-` | 4E | `=` | 55 |
| `[` | 54 | `]` | 5B | `\` | 5D | `;` | 4C |
| `'` | 52 | `,` | 41 | `.` | 49 | `/` | 4A |
| `` ` `` | 0E | | | | | | |

### Control and modifier keys

| Key | Make | Notes |
|---|---|---|
| `Esc` | 76 | |
| `Backspace` | 66 | |
| `Tab` | 0D | |
| `Enter` | 5A | main Enter |
| `Space` | 29 | |
| `Caps Lock` | 58 | |
| `Left Shift` | 12 | |
| `Right Shift` | 59 | |
| `Left Ctrl` | 14 | |
| `Left Alt` | 11 | |
| `Right Ctrl` | E0 14 | extended |
| `Right Alt` | E0 11 | extended |
| `Menu` | E0 2F | extended |
| `Left GUI` | E0 1F | Windows / Command key |
| `Right GUI` | E0 27 | Windows / Command key |

### Function keys

| Key | Make | Key | Make | Key | Make | Key | Make |
|---|---|---|---|---|---|---|---|
| `F1` | 05 | `F2` | 06 | `F3` | 04 | `F4` | 0C |
| `F5` | 03 | `F6` | 0B | `F7` | 83 | `F8` | 0A |
| `F9` | 01 | `F10` | 09 | `F11` | 78 | `F12` | 07 |

### Navigation keys

| Key | Make | Key | Make |
|---|---|---|---|
| `Up` | E0 75 | `Down` | E0 72 |
| `Left` | E0 6B | `Right` | E0 74 |
| `Home` | E0 6C | `End` | E0 69 |
| `Page Up` | E0 7D | `Page Down` | E0 7A |
| `Insert` | E0 70 | `Delete` | E0 71 |

### Locks and special keys

| Key | Make | Notes |
|---|---|---|
| `Num Lock` | 77 | |
| `Scroll Lock` | 7E | |
| `Print Screen` | E0 12 E0 7C | one key event, four bytes |
| `Pause` | E1 14 77 E1 F0 14 F0 77 | one key event, eight bytes |

`Print Screen`'s break is `E0 F0 7C E0 F0 12`.  `Pause` has no break code.

### Numeric keypad

| Key | Make | Key | Make |
|---|---|---|---|
| `keypad 0` | 70 | `keypad 1` | 69 |
| `keypad 2` | 72 | `keypad 3` | 7A |
| `keypad 4` | 6B | `keypad 5` | 73 |
| `keypad 6` | 74 | `keypad 7` | 6C |
| `keypad 8` | 75 | `keypad 9` | 7D |
| `keypad .` | 71 | `keypad Enter` | E0 5A |
| `keypad +` | 79 | `keypad -` | 7B |
| `keypad *` | 7C | `keypad /` | E0 4A |

With Num Lock on, the keypad digits send the codes above.  With Num Lock off,
keypad 1–4 and 6–9 send the navigation codes above; keypad 5 has no navigation
assignment, and its Num Lock-off behavior is keyboard-dependent.

## Reading a byte sequence

| Bytes | Meaning |
|---|---|
| `1C` | `A` pressed |
| `1C 1C 1C …` | `A` held; typematic repeats (the keyboard repeats the make code) |
| `F0 1C` | `A` released |
| `12 1C F0 1C 12` | Left Shift held, `A` pressed and released, Shift released |
| `E0 75` | Up arrow pressed |
| `E0 F0 75` | Up arrow released |
| `E1 14 77 E1 F0 14 F0 77` | `Pause` |
| `AA` | power-on self-test passed (BAT OK); not a key |
| `FC`, `FD` | power-on self-test failed (BAT error); not a key |
| `FA` | a command-acknowledge byte (`0xFA`); not a key |

`AA`, `FC`, `FD` and `FA` are keyboard responses, not scancodes
([usage.md §First bytes after power-up](usage.md#first-bytes-after-power-up)).

Modifier state is **not** tracked by the mode.  A byte sequence shows the make
and break bytes in order; the host must remember which modifiers are down.  The
same is true for Caps Lock, Num Lock and Scroll Lock: the lock state lives in
the keyboard, not in the Bus Pirate.

## What changes the printed value

The keyboard frame is always one 8-bit byte with LSB-first data.  The shared
prompt settings can change what the mode prints, and one of them changes what
is sent.  `l`/`L` is a value-level reversal, not a wire-order change: `l` (the
default) leaves the value alone; `L` reverses the 8-bit value before a write
and after a read.  The wire remains LSB-first
([protocol.md §Two bit orders](protocol.md#two-bit-orders)).

| Setting | Effect on KEYB |
|---|---|
| `o` (display format) | HEX (default), DEC, BIN or RAW; RAW sends the byte itself |
| `;N` (width) | masks the printed value and appends `;N`; the frame stays 8 bits |
| `l` / `L` (bit order) | `l` (default) leaves values alone; `L` reverses reads and writes |

Leave the width at 8 and the bit order at `l` for host commands: with `L`
set, a write reply shows the **typed** value, not the reversed byte that went
on the wire.  `w` and a mode change restore 8 bits; `l` restores the default
bit order.

## Caveats

- The roster is the standard PC-AT Set 2 table.  A specific keyboard, or a
  USB keyboard behind a PS/2 adapter, may use a different set or remap keys;
  the mode prints what arrives.
- The `0xF0` host command can select Set 1, 2 or 3 on keyboards that support
  it.  After a set change the bytes above no longer apply.
- There is no character map, no key-name map and no layout handling: `1C` is
  the physical `A` key on a US layout, but the mode only sees the code.

## Related docs

- [usage.md](usage.md) — the monitor, reads and host commands.
- [protocol.md](protocol.md) — the frame and parity.
- [generic/values.md](../../generic/values.md) — `o`, `;N` and `l`/`L`.
