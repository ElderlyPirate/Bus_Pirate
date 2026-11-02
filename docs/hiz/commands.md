# Commands in HiZ — what works, what is refused

Part of the HiZ documentation set (`docs/hiz/`). The full command reference
and the shared CLI grammar are owned by
[`docs/generic/commands.md`](../generic/commands.md); this page states only how
those commands behave **in HiZ** — which ones act, which are refused, and
which are HiZ-only.

## Commands that work in HiZ

These are handled by the base layer and are not restricted to a bus mode. They
behave as described in `docs/generic/` unless noted.

| Input | Action in HiZ | Notes |
|---|---|---|
| `i` | version + chip info | no status block in HiZ; the status block is printed only outside HiZ |
| `?` | help screen | same 23-line help as everywhere |
| `m` / `m<N>` | mode menu / direct change | `m1` re-enters HiZ |
| `h` | command history | — |
| `b` / `b<N>` | terminal baud rate | — |
| `o` / `o<N>` | display format | HEX / DEC / BIN / RAW |
| `v` | pin states + rail voltages | the HiZ table is in [overview.md](overview.md) |
| `t` / `T` | elapsed timer read / reset | µs |
| `f` | frequency counter on the AUX pin | usable in HiZ because no PWM can be running here |
| `a` / `A` / `@` | drive the AUX pin low / high / read it | acts on the selected AUX target |
| `c` / `C` (`k`/`K` v4) | AUX assignment | target mapping in [overview.md](overview.md) |
| `l` / `L` | bit order MSB-first / LSB-first | — |
| `d` / `D` | ADC probe (one-shot / continuous) | `d:N` takes N readings |
| `&` / `%` | busy delay N µs / N ms | — |
| `=` / `\|` | convert / bit-reverse and convert a value | — |
| `s` | enter the BASIC interpreter | `HiZ(BASIC)>` |
| `<0>` / `<n>` / `<n=body>` | user macros list / run / assign | v4 5 slots, v3 3 |
| `#` | reset | see [overview.md](overview.md) |
| `$` | jump to bootloader | confirmation prompt first |

The AUX commands (`a`, `A`, `@`, `c`, `C`, `k`, `K`) are the one place HiZ is
not inert: `A`/`a` actively drive the selected AUX target pin. Nothing else in
HiZ drives a pin.

## Commands refused in HiZ

These are refused with a single line, `Command not used in this mode`, and no
action is taken. They are the elements that need an active bus:

| Input | Meaning | In HiZ |
|---|---|---|
| `W` / `w` | power supplies on / off | `Command not used in this mode` |
| `P` / `p` | pull-up resistors on / off | `Command not used in this mode` |
| `g` / `S` | PWM / servo generator (AUX pin) | `Command not used in this mode` |
| `O` | 1-Wire standard/overdrive toggle | `Command not used in this mode` |
| `e` (v4) | VPU pull-up source menu | `Command not used in this mode` **and** a syntax error |

Notes:

- `W`, `w`, `P`, `p`, `g`, `S` and `O` print the line and return; only `e`
  also flags an error, so `e` is followed by `Syntax error at char 1`.
- The `e` command does not exist on v3 (no such command at all).
- `g`/`S` are base-layer commands available in **every mode except HiZ**. That
  is the exact rule: PWM/servo is the one capability the idle mode withholds.
  `f` (frequency) is not restricted — it works in HiZ too.
- To use any of these, switch to the mode that owns the bus with `m`/`m<N>`,
  start the supplies with `W`, and then issue them.

## HiZ-only command

| Input | Rule |
|---|---|
| `~` | runs the self-test **in HiZ only**.  In any other mode it prints `Self-test in HiZ mode only` and does nothing. |

`~` drives the Bus Pirate's own pins and supply, so disconnect any target
first. The terminal `~` runs the full test and first prompts for the jumper.
The checks, the jumper prompts and the output are owned by
[`docs/self-test/`](../self-test/README.md); the entry is shown in
[usage.md](usage.md).

## Commands with no effect in HiZ

The bus operations have no handler in HiZ, so they report
`ERROR: command has no effect here`:

| Input | Meaning | In HiZ |
|---|---|---|
| `[` `{` `]` `}` | transaction wrappers | `ERROR: command has no effect here` |
| `r` | read | same (see the `rx`/`rd`/`rb`/`rw` note below) |
| `/` `\` `^` | clock high / low / tick | same |
| `-` `_` `.` | data high / low / read | same |
| `!` | read bit(s) | same |
| `"abc"`, `123`, `0x55` | send a value | same |
| `(n)` | run a mode macro | same; HiZ has no mode macros |

The exact error text, the follow-on syntax error for line-level items, and the
`rx`/`rd`/`rb`/`rw` quirk (the read letter is not consumed) are documented in
[`docs/generic/edge-cases.md`](../generic/edge-cases.md) and
[`docs/generic/commands.md`](../generic/commands.md). A worked HiZ line is in
[usage.md](usage.md).

## Related docs

- [README.md](README.md) — the set map.
- [overview.md](overview.md) — pin-state model and entering/leaving.
- [usage.md](usage.md) — worked sessions.
- [edge-cases.md](edge-cases.md) — failure modes.
- [../generic/commands.md](../generic/commands.md) — the full command
  reference.
- [../SPEC.md](../SPEC.md) — the capability index (`v3 | v4`).