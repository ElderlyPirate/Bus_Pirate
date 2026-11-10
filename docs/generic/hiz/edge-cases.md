# Edge cases & failure modes — HiZ

Part of the HiZ documentation set (`docs/hiz/`). Each entry is concrete:
trigger → observable symptom → behaviour → recovery. Shared edge cases
(buffer overflow, `;` width limits, command-history limits) are in
`docs/generic/edge-cases.md`.

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `W`, `w`, `P`, `p`, `g`, `S` or `O` in HiZ | `Command not used in this mode` | refused, no action; switch to the owning mode and start supplies with `W` |
| `e2` / `e3` in HiZ (v4) | `Command not used in this mode` then `Syntax error at char 1` | refused; `e` is only accepted outside HiZ, on an open-drain bus, with no voltage already on VPU |
| `e` on v3 | `Syntax error at char 1` | there is no `e` command on v3 |
| `~` in any non-HiZ mode | `Self-test in HiZ mode only` | refused; return to HiZ (`m1` or `#`) and run `~` again |
| `~` with a target still attached | the self-test drives the Bus Pirate's own pins/supply | disconnect the target first — see `docs/self-test/` |
| `~` full run without the `ADC`→`+3.3V` jumper | `ADC` check prints `FAIL` | place the jumper; the remaining checks still run |
| bus operation in HiZ (`r`, `[`, `]`, `^`, `!`, a bare value) | `ERROR: command has no effect here` | nothing is sent; use a bus mode for bus traffic |
| `(n)` mode macro in HiZ | `ERROR: command has no effect here` plus a syntax error | HiZ has no mode macros; use a mode that has them |
| `a` / `A` in HiZ | `AUX LOW` / `AUX HIGH`, and the selected AUX pin is now driven | expected: `a`/`A` are not restricted to a bus mode. `@` returns the pin to input |
| `v` read-back shows `H`/`L` on idle pins | floating pin states | expected in HiZ with nothing connected; a state is not a driven level |
| a tool sends a reset byte after a mode change | it waits for `HiZ>` | v4 simulated reset returns HiZ + banner; v3 reboots |

## Mode-required commands are refused in HiZ

**Trigger:** you type `W`, `w`, `P`, `p`, `g`, `S` or `O` while the prompt is
`HiZ>`.

**Symptom:** `Command not used in this mode`; nothing changes.

**Behaviour:** the supplies, pull-ups, PWM/servo and the 1-Wire toggle all need
an active bus, and the base layer refuses them in HiZ. The `e` command is the
one exception in wording: it prints the same line **and** raises an error, so
it is followed by `Syntax error at char 1`.

**Recovery:** switch to the mode that owns the bus with `m`/`m<N>`, run `W` to
engage the bus, then issue the command. The availability rules are in
[commands.md](commands.md).

## The self-test needs HiZ

**Trigger:** `~` while the prompt is anything but `HiZ>`.

**Symptom:** `Self-test in HiZ mode only`.

**Behaviour:** the test runs only in HiZ; in another mode the command is
refused and no check runs.

**Recovery:** return to HiZ with `m1` (or `#`), then run `~`. The test itself is
documented in `docs/self-test/`.

## `~` drives the board — disconnect the target

**Trigger:** running `~` with a target still wired to the header.

**Symptom:** the self-test drives the Bus Pirate's own pins and supply; a
connected target sees those levels.

**Behaviour:** `~` is a board bring-up test, not a target test.

**Recovery:** disconnect the target before `~`. The header wiring is not a
guarantee against a target being driven.

## Floating pin states are not levels

**Trigger:** reading `v` in HiZ with nothing connected to the header.

**Symptom:** the direction row shows `I` for every I/O pin and the state row
shows some mixture of `H` and `L`.

**Behaviour:** high-impedance pins float, so the read-back state depends on
stray charge and any connected circuit; it is not a level the Bus Pirate is
driving.

**Recovery:** none needed — this is the idle state. To drive a line, use a bus
mode; to read a real level, connect a source or use the pull-ups in a mode
(HiZ itself refuses `P`).

## AUX can still be driven in HiZ

**Trigger:** `A` (or `a`) at the `HiZ>` prompt.

**Symptom:** `AUX HIGH` (or `AUX LOW`), and the selected AUX target pin becomes
an output at that level.

**Behaviour:** `a`/`A`/`@` and `c`/`C`/`k`/`K` are general commands, not
restricted to a bus mode, so HiZ is not fully inert on the AUX pin. The target
mapping is in [overview.md](overview.md).

**Recovery:** `@` reads the pin and returns it to input; re-entering HiZ with
`m1` re-impedes the pins and releases the pull-ups and supplies.

## A reset must land on `HiZ>`

**Trigger:** a host tool resets the Bus Pirate (reset byte, or waiting for the
prompt after a run).

**Symptom / expectation:** the tool waits for the `HiZ>` prompt.

**Behaviour:** on v4 a simulated reset (`#`, the binary reset byte `0x0F`, or
leaving SUMP) returns the terminal to HiZ and prints the banner; on v3 `#`
performs a real reboot. This is the documented reset contract.

**Recovery:** if the prompt is not `HiZ>`, issue `#` from the terminal to
return to HiZ.

## Related docs

- [README.md](README.md) — the set map.
- [overview.md](overview.md) — pin-state model and entering/leaving.
- [commands.md](commands.md) — what works and what is refused.
- [usage.md](usage.md) — worked sessions.
- [../generic/edge-cases.md](../edge-cases.md) — shared failure modes.
- [../self-test/README.md](../../capabilities/selftest/README.md) — the `~` self-test.