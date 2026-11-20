# DIO

**Status:** v3 and v4 documented; the v3 behaviour is not verified on
v3 hardware.

**DIO** is the firmware's direct I/O mode: the terminal sets each of the five
signal pins by hand, direction and level, together with the on-board supply
and the optional pull-up resistors.  There is no clock generator, framing or
protocol engine; the host decides the sequence and the pace.  Use it for
custom protocols, bit-level debugging, or any pin no bus mode exposes.

`DIO` is the mode name and prompt (`DIO>`); in prose this set calls the
function **direct I/O**.

This README is a map, not the manual; the details live in the files below.

| Task | Start here |
|---|---|
| wire and drive one pin | [wiring.md §The five signal pins](wiring.md#the-five-signal-pins), then [pin-control.md §Worked session](pin-control.md#worked-session) |
| use `AUX1` / `AUX2` | [aux-pin.md §Selecting the target](aux-pin.md#selecting-the-target) |
| read a pin back | [read-back.md](read-back.md) |
| park safely | [park-and-reset.md §What "parked" means](park-and-reset.md#what-parked-means) |
| recover from a symptom | [edge-cases.md §Symptom index](edge-cases.md#symptom-index) |

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | availability, mode number, both entry forms, no settings menu, what entry resets, `W` / `w` |
| [wiring.md](wiring.md) | the five signal pins, ground, target supply, VPU and pull-ups, output type, voltage limits, header order |
| [pin-control.md](pin-control.md) | numeric writes: direction and level bytes, the bit maps, value traps, a worked session |
| [aux-pin.md](aux-pin.md) | `a` / `A` / `@` on the selected AUX target; `c` / `C` (v4 `k` / `K`) selection; `AUX1` / `AUX2` |
| [read-back.md](read-back.md) | `r`, `v`, reading an external signal, `@`, and write-with-read |
| [park-and-reset.md](park-and-reset.md) | the safe-park contract, `w` versus `#`, mode changes, what survives |
| [macros.md](macros.md) | no mode macros; user macros `<n>` for pin sequences |
| [edge-cases.md](edge-cases.md) | concrete traps and failure modes, by topic |

## The five-minute version

- Enter with `m10` on v4 (`m9` on v3), or `m` then the number.  The prompt
  becomes `DIO>`; there is no settings menu and no output-type prompt.  On a
  custom build, run `i` and look for `DIO` in the bracketed mode list, or use
  the number the `m` menu prints; the numbers above are the default image.
- Wiring: `GND`, the signal pins you use, and at most one supply rail.  `W`
  turns on the on-board `+5.0V` and `+3.3V` rails; `w` turns them off and
  parks the five signal pins on v3 and all seven pins — the five plus
  `AUX1` / `AUX2` — on v4.  For a self-powered target, leave both rails
  unconnected.  **No pull-up is required.**
- A value is a write.  `0x00`–`0x7F` sets the five **directions** (1 = input,
  0 = output); `0x80`–`0xFF` sets the **levels** plus the supply and pull-up
  bits.  The reply is `WRITE: <value>`: the echoed value, not the pin state.
- `a` / `A` / `@` drive or read the selected AUX target; `c` / `C` (v4 `k` /
  `K`) select `AUX`, `CS`, `AUX1`, `AUX2`.  That is the only route to `AUX1`
  and `AUX2`.  Run `i` to see the selected target; `v` does not show it
  ([aux-pin.md](aux-pin.md#which-target-is-selected)).
- Read with `r` (five levels as one byte) or `v` (directions, levels and
  rails).  A write never adds a `READ:`; see [read-back.md](read-back.md).
- Before touching the wiring, release the pins and check `v`: after `#` every
  I/O pin must be `I`; after `w` the five signal pins (v3) or all seven pins
  (v4) are released.  The board rails must read `0.00V`.  Use `#` only when you
  also need the selector / bit-order / display reset.  `#` returns to `HiZ`;
  `w` stays in `DIO>`.

## Availability (summary)

| Item | v3 | v4 |
|---|---|---|
| build | included in the default image | included in the default image |
| build option | on by default; a build can leave the mode out | same |
| mode number (default image) | **9** | **10** |
| prompt | `DIO>` | `DIO>` |
| settings menu | none | none |
| pins a value can address | `AUX` (4), `MOSI` (3), `CLK` (2), `MISO` (1), `CS` (0) | same |
| `AUX1` / `AUX2` in a value | — | no; AUX-target path only |
| `a` / `A` / `@` targets | `AUX`, `CS` | `AUX`, `CS`, `AUX1`, `AUX2` |
| `e` (VPU source) | absent | refused |
| `w` park scope | five signal pins | five signal pins plus `AUX1` / `AUX2` |
| `#` | hardware reset | terminal reset to `HiZ` |

The v3 column is **not verified** on v3 hardware.

## Names used in this set

| Form | Meaning |
|---|---|
| `DIO` / `DIO>` | mode name / prompt; prose says direct I/O |
| `AUX` (`A0`) | auxiliary header pin; bit 4 of a value |
| `CS` | chip-select header pin; bit 0; also an AUX target |
| `MISO`, `CLK`, `MOSI` | signal pins; bits 1, 2, 3 |
| `AUX1` (`A1`), `AUX2` (`A2`) | v4 header pins; AUX-target path only |
| `Vpullup` (`Vpu`, `VPU`) | pull-up reference pin; the `v` column says `VPU` |

The `?` help prints the AUX assignment tokens: `A0` / `CS` / `A1` / `A2` on
v4, and `aux` / `CS` on v3.  The confirmation line and `v` use the pin names.
Both name the same pins; the shared
[generic/commands.md](../../generic/commands.md#aux-targets-and-pin-names)
owns the full mapping.  The `v` table shows pin states, not the selected AUX
target; run `i` to see the selection.

## Limits & out of scope

- No clock or protocol engine, no framing and no waveform timing; the host
  types the sequence, and the shared `&` / `%` delays are the only built-in
  waits between commands.
- No **mode** macros (`(n)`); user macros `<n>` still run
  ([macros.md](macros.md)).
- The base-layer ADC, frequency and PWM/servo commands (`d` / `D`, `f`,
  `g` / `S`) also run here; they use the shared ADC and AUX pins and are
  documented in their own sets.
- No binary sub-protocol.  A value uses the same pin layout as the binary
  bitbang layer, but the binary command codes do not apply here.
- Outputs are **push-pull only**; there is no output-type setting, and the
  on-board VPU source cannot be selected from direct I/O.  Feed `VPU`
  externally if the pull-ups are used.
- Only the **low byte** of a value reaches the pins; `;N` changes the display
  and bit-order window, not the pin count.
- The v4 `AUX1` / `AUX2` pins are not in the value, but `w` releases them
  along with the five signal pins (seven pins on v4; five on v3).  Use `#` or a
  mode change when you also need the selector reset.
- The I/O is 5 V class; the exact absolute maximum is not documented here,
  so treat it as unverified.
- Shared prompt grammar (`m`, `W` / `w`, `P` / `p`, `e`, `v`, `r`, `o`,
  `l` / `L`, `(n)`, `<n>`, `#`, `$`) lives in
  [docs/generic/](../../generic/README.md).

## Related docs

- [generic/](../../generic/README.md) — the shared prompt: mode menu, values,
  macros, reset and the `v` table.
- [capabilities/auxiliary/](../../capabilities/auxiliary/README.md) — the owned
  AUX pin, PWM, servo and frequency set.
- [capabilities/adc/](../../capabilities/adc/README.md) — the owned ADC probe
  and continuous-sampling set.
- [binary/](../binary/README.md) — the binary bitbang layer whose state-byte
  layout direct I/O shares.
- [SPEC.md](../../SPEC.md) — the function and capability index.
- [dev/building.md](../../dev/building.md) — the build option table.
