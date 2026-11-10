# Self-test — binary variants

Part of the self-test documentation set (`docs/capabilities/selftest/`).  The
scriptable self-test carried by the binary bitbang layer: enter it with
`0x10` (short) or `0x11` (full, jumpers required).  The interactive form is
in [terminal.md](terminal.md); the checks themselves are in
[checks.md](checks.md).

The general binary layer — entering BBIO, framing, returning to the
terminal — is owned by `docs/generic/` (see
[commands.md](../../generic/commands.md) §Leaving the CLI — binary / SUMP
entry) and `docs/protocol/binary/` — see
[entering-and-framing.md](../../protocol/binary/entering-and-framing.md),
[commands.md](../../protocol/binary/commands.md) and
[sub-protocols.md](../../protocol/binary/sub-protocols.md).

## Naming

This set calls the two variants *short* and *full*; older material calls the
full one the "long test".

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20× `0x00`; it
   prints `BBIO1`.
2. Send the command byte:

| Command | Variant | What it covers |
|---|---|---|
| `0x10` | short self-test | the full pass minus the checks marked `skipped` in [checks.md](checks.md) |
| `0x11` | full self-test | every check; fit the jumpers shown in [terminal.md](terminal.md) §Setup first |

3. The firmware runs the test with **all text suppressed** and replies with
   **one byte: the number of errors** (`0x00` = no errors).
4. While the test is idle it reads input: any byte other than `0xFF` is echoed
   back as `byte + errors` (one byte, wrapping); `0xFF` leaves the test,
   replying `0x01`.
5. After leaving the self-test the firmware is back in the bitbang layer.
   Send `0x0F` to return to the terminal.

The short pass needs no jumpers; the full test uses the same setup as the
interactive run (see [terminal.md](terminal.md) §Setup).  The `0x10` column in
[checks.md](checks.md) defines what the short pass skips.

## LED indication

- No errors → the MODE LED **blinks** while the test waits.
- One or more errors → the MODE LED is **on solid**.

The LED starts out off when the pass found no errors, so a host that sends
the leave byte (`0xFF`) immediately after the count can return before the
first blink.

The error count is the number of failed checks — the same count the
interactive run prints.

## Worked exchange

Short test, everything in order:

```
host -> 0x00 x20        enter bitbang
host <- "BBIO1"
host -> 0x10            short self-test
host <- 0x00            error count: 0 = no errors
host -> 0x41            any byte while the test is idle
host <- 0x41            0x41 + 0 errors, echoed back
host -> 0xFF            leave the self-test
host <- 0x01            confirmation
host -> 0x0F            return to the terminal
host <- 0x01            confirmation
host <- "HiZ>"          terminal prompt
```

## v3 / v4

Both builds expose both bytes.  The v3 column is **unverified**.

## Limits

- The reply is a single error count; the binary layer prints no per-check
  lines, so a failure is located from the interactive run.
- The MODE LED is the only status signal during the test — it is a light, not
  a text result.
- The firmware does not re-send `BBIO1` when the test exits; the host must
  track the current layer itself.

## Related docs

- [checks.md](checks.md) — the full check reference.
- [terminal.md](terminal.md) — the interactive run.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
- [../protocol/binary/README.md](../../protocol/binary/README.md),
  [../protocol/binary/commands.md](../../protocol/binary/commands.md) — the
  binary layer.
