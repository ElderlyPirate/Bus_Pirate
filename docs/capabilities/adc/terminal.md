# ADC probe — terminal

Part of the ADC capability set (`docs/capabilities/adc/`).  This page covers the
interactive front ends: `d` (one reading or a repeat), `D` (continuous
voltmeter), their output shape, and the display-format setting.  The scale is
in [probe.md](probe.md); the binary bytes are in [binary.md](binary.md).

## Scope

Covered: `d`, `d:N`, `D`, the output text and units, the stop behaviour, and
the `o` interaction.  The shared prompt grammar (`:N`, `o`, mode entry) is
owned by [docs/generic/](../../generic/README.md) and is not repeated here.

## Where it works

`d` and `D` are base-layer commands: they work at the `HiZ>` prompt and in
every bus mode.  They do not drive, park or read any digital line, so a
reading does not disturb a bus in progress.  No supply (`W`) and no pull-up
(`P`) is needed; the probe input and its reference are always available at the
terminal.

## Commands

| Input | Action | Output |
|---|---|---|
| `d` | one conversion on the probe channel | `VOLTAGE PROBE: <d.dd>V (raw <n>)` |
| `d:N` | N conversions, N = 1..32767 | the header once, then N readings |
| `D` | continuous conversions until a key arrives | `VOLTMETER MODE`, `Any key to exit`, a live value |

`d:N` uses the shared repeat grammar: the count follows the command with a
colon, exactly as in `r:10` or `@:4`.

## One-shot output — `d`

```
HiZ>d
VOLTAGE PROBE: 3.29V (raw 512)
HiZ>
```

- `VOLTAGE PROBE: ` is the fixed header, printed without a newline.
- The voltage is always plain decimal volts with two decimals, then `V`.
- The raw count follows in parentheses in decimal, `0`..`1023`.
- Each reading ends with a newline.

A repeat takes the readings one after another and prints the header only once:

```
HiZ>d:3
VOLTAGE PROBE: 3.29V (raw 512)
3.29V (raw 512)
3.29V (raw 512)
HiZ>
```

The second and later lines start with the voltage; the header is not repeated.
This is the shape to expect from any `d:N`.

`d` takes no display letter.  The `r` command accepts `rx`/`rd`/`rb`/`rw`,
but `d` has only the optional `:N`; a letter after `d` is parsed as the next
command.

## Continuous output — `D`

`D` prints two header lines and then a live value:

```
HiZ>D
VOLTMETER MODE
Any key to exit
VOLTAGE PROBE: 3.29V
HiZ>
```

The transcript omits the key press that ends the display.

What the terminal does not show:

- The first value on the line is a `0.00V` placeholder printed before the
  sampling loop.  If a byte is already waiting (type-ahead, or a line
  terminator left by a terminal that sends both CR and LF), the loop exits at
  once and the `0.00V` is the placeholder, not a reading.
- Every new sample is preceded by five backspace characters, so the value is
  rewritten in place.  A raw capture shows those backspaces (often as `^H` or
  `\b`) between values; a terminal applies them and shows one changing number.
- Only the voltage is shown.  Unlike `d`, continuous mode does not print the
  raw count.

The displayed value is the most recent sample; earlier samples are gone.

## Stopping `D`

Any byte from the host stops the loop.  The loop reads one byte, discards it,
prints a newline and returns to the prompt:

- The stopping key is consumed: it does not become a command.
- Only one byte is consumed.  If several keys were queued, the remaining ones
  run as commands after the prompt returns.
- There is no timeout and no sample count: `D` runs until a key arrives.

Typing a key that is also a command (for example `d`) stops the display and
does not start a new reading; the consumed key is the stop.

## Display format — `o`

The `o` setting changes how bus values are printed, not how the probe is
printed.  Under `o1` HEX, `o2` DEC, `o3` BIN and `o4` RAW the voltage and the
raw count are always decimal.

## When continuous sampling helps

`D` (and binary `0x15`) is for watching a value change, not for recording it:

- turning a potentiometer and watching the wiper voltage move;
- checking whether a rail sags when a load switches on;
- probing a slow sensor output while adjusting it;
- finding the threshold at which a circuit changes state.

It is the wrong tool for capturing a transient (no trigger, no history, no
timestamps), for anything faster than the serial link, and for a measurement
you need to keep.  Use `d` or one-shot `0x14` for a single value, and a
dedicated instrument for timing.

## Reading the value

- The display truncates to 0.01 V, and the fixed-point scale sits a shade
  below 6.6 / 1024, so exact step values can print one step low: 1.650 V prints
  as `1.64V` and 3.300 V as `3.29V`.  The full explanation is in
  [probe.md §Why 3.3 V can print 3.29 V](probe.md#why-33-v-can-print-329-v).
- The top of the scale is `6.59V` (raw 1023); an over-range input is
  indistinguishable from 6.6 V.
- An unconnected input has no defined source: it usually sits near 0 V through
  the divider's lower leg, but leakage or a nearby signal can move it.  Never
  interpret a reading without a connected source.  See
  [edge-cases.md](edge-cases.md).
- The raw count is the finer figure: one count is 6.45 mV.

## Related

- [probe.md](probe.md) — the scale, resolution and accuracy.
- [binary.md](binary.md) — the same readings over the binary layer.
- [wiring.md](wiring.md) — the pin and its input limits.
- [docs/generic/commands.md](../../generic/commands.md) — the `d` / `D` table
  entries and the `:N` grammar.
