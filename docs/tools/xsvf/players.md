# The host player — `bp xsvf`

Part of the XSVF set.  This page is the reference for the PC command that
streams an XSVF file, probes the chain or resets it: installation, options,
serial settings, output, exit statuses and the opt-in leave.  The firmware
side is [player.md](player.md); the end-to-end sequence is
[workflow.md](workflow.md).

## Scope

Covered: `bp xsvf` and the `bp` frontend it belongs to.  Not covered: the
firmware player's byte protocol ([player.md](player.md)), conversion
([converter.md](converter.md)) and wiring ([wiring.md](wiring.md)).

## What the command does

1. Reads the whole XSVF file and rejects a missing or empty file before the
   serial port is opened.
2. Enters binary bitbang (`BBIO1`), then the player (`XSV1`).
3. Sends the chain reset if `--reset` was given.
4. Runs the chain probe if `--scan` was given.
5. Streams the file if `--file` was given.
6. If `--leave` was given, returns the board to the terminal.

The steps always run in that order, whatever order the options are written
in.  Do not pre-enter binary mode or the player; the command does both.

## Install and port

`bp` is installed from the tool directory; Python 3.12 or newer is required.

```
cd scripts/bp
python -m venv .venv
```

Activate it, then install the package and its dependencies:

| Shell | Activate |
|---|---|
| Windows cmd | `.venv\Scripts\activate.bat` |
| Windows PowerShell | `.venv\Scripts\Activate.ps1` |
| Linux/macOS | `source .venv/bin/activate` |

```
python -m pip install -e .
```

Then check the port:

```
$ bp ports
COM1  ----:----  Communications Port (COM1)
COM5  04D8:FB00  USB Serial Device (COM5)  [Bus Pirate v4 application]
```

`bp ports` only lists; it does not open a port or choose one.  `bp xsvf`
requires `--port` or a configured `port`.  `bp version` reads the banner and
should report `Hardware type: Bus Pirate v4`; the current
`ElderlyPirate Firmware v9.0` image includes the player.

An optional YAML file supplies defaults.  `--config` is a root option and
must come before `xsvf`:

```
port: COM6
speed: 115200
```

```
bp --config bp.yaml xsvf --scan
```

An explicit option wins over the file.  Unknown keys are rejected.

## Options

| Option | Argument | Meaning |
|---|---|---|
| `--port`, `-p` | device | serial port, for example `COM5` or `/dev/ttyACM0` |
| `--file`, `-f` | file | XSVF image to stream; optional when `--scan` or `--reset` is used |
| `--speed`, `-s` | integer | host serial speed, default `115200`; host side only |
| `--scan`, `-x` | — | probe the chain and print its count and IDCODE bytes |
| `--reset`, `-r` | — | reset the chain before scanning or streaming |
| `--leave` | — | return to the terminal after a successful run |
| `--help` | — | print the command help |

- **At least one operation is required.**  `--file`, `--scan` or `--reset`
  must be present; otherwise the command is a usage error and exits `2`.
- `--reset` and `--scan` run without a file.  `--file` may be combined with
  either or both.
- `--reset` is optional before a file: the stream starts from a reset TAP
  anyway.  It adds an explicit reset and leaves the chain in Run-Test/Idle
  first.
- `--speed` does not change the player's TCK rate.  On v4 the USB link rate
  is host metadata; leave the default.
- `--leave` is opt-in and runs only after every requested operation
  succeeds ([below](#leave-the-player)).

## Serial settings

| Setting | Value |
|---|---|
| data bits | 8 |
| parity | none |
| stop bits | 1 |
| flow control | none |
| speed | `--speed`, default 115200 |

The command configures the port itself.  Close any terminal program first;
Windows opens the port exclusively.  On Linux the port is usually
`/dev/ttyACM0`, and the user must be allowed to open it (commonly the
`dialout` group).

The board must be at the terminal prompt (`HiZ>` or another mode prompt) so
that the binary entry can reach it.  If it is in a menu or already in the
player, press reset first.

## Reading the output

| Line | Meaning |
|---|---|
| `File is 53289 bytes.` | the file was read; printed only with `--file` |
| `Opening Bus Pirate on COM5 at 115200 bps.` | the connection settings |
| `Performing Reset.. Done` | `--reset` ran |
| `Chain Scan Result: 04 93 40 60 59` | `--scan` reply: count byte, then IDCODE bytes |
| `Sent 53289 bytes in 14 chunk(s).` | the whole file was streamed in 14 chunks |
| `End of operation reply: XSVF_ERROR_NONE` | the player's result byte |
| `Success!` | result `0x00` |
| `Returned to the Bus Pirate terminal.` | `--leave` succeeded |
| `Thank you for playing! :-)` | the command finished; always the last line on success |

A clean file run:

```
$ bp xsvf -p COM5 -f design.xsvf -r
File is 53289 bytes.
Opening Bus Pirate on COM5 at 115200 bps.
Performing Reset.. Done
Sent 53289 bytes in 14 chunk(s).
End of operation reply: XSVF_ERROR_NONE
Success!
Thank you for playing! :-)
```

The chunk count is decimal: 53289 bytes is 13 chunks of 4096 plus 41 bytes,
so 14 chunks.  The success check is the `XSVF_ERROR_NONE` line followed by
`Success!`; `Thank you for playing! :-)` alone only says the host stopped.

A player error prints one `Error:` line and no result lines:

```
Error: End of operation reply: XSVF_ERROR_TDOMISMATCH (0x02); the captured TDO did not match the expected TDO
```

The result lines are printed after all requested operations return, so a
failure suppresses the reset, scan and sent lines even when those steps ran.

## Exit status

| Status | Meaning |
|---|---|
| `0` | success: an `XSVF_ERROR_NONE` stream, or a completed scan/reset |
| `1` | the player returned a non-success result byte |
| `2` | usage, host, transport or setup failure |

- `0` covers a stand-alone scan or reset as well as a file run.  A script
  that must prove a file was played should pass `--file` and check for
  `Success!`, not only the status.
- `1` is a player result: a malformed or wrong-target stream, a TDO mismatch,
  a shift overflow, or the host stream stalling.  The board is back in the
  player loop.
- `2` is a host or setup failure: no port, no reply, an incomplete scan, a
  bad or empty file, a wrong banner, a missing result, EOF before completion,
  or a failed leave.  The board may be in `BBIO1`, in the player, or still
  busy; see [limits-and-errors.md](limits-and-errors.md).

On v3 the player selection is answered with `0x00`, so `bp xsvf` reports that
the board did not enter XSVF mode, exits `2`, and leaves the board in
`BBIO1`.  A v4 that gives the same message is not running an image with the
player; flash it.  Press reset, or send `0x0F` from a raw serial host,
afterwards.

## The chain scan

`--scan` sends the player's chain-probe command (`0x02`) and prints the
reply on one line:

```
Chain Scan Result: 04 93 40 60 59
```

- The first byte is the count, equal to four times the number of devices.
- Four IDCODE bytes follow per device, least-significant byte first, the
  device nearest TDO first.  The example is the IDCODE `0x59604093`.
- The command reads the count byte and exactly that many payload bytes, with
  a bounded number of retries.  An incomplete reply exits `2` before any
  file is streamed, so `--scan` with `--file` is safe.
- `00` with no payload (TDO high, no target power, or an empty chain) and
  `FC` followed by 252 zero bytes (TDO low, 63-device clamp) are valid
  replies and exit `0`.  They are wiring observations, not stream errors.
- The probe reports at most 63 devices.  A longer chain is truncated, not
  wrapped.

A count that is not a multiple of four, or larger than 252 bytes, is
rejected as a host error; the firmware cannot produce it, so it indicates a
desynchronised or damaged reply.

## Leave the player

By default the command leaves the board in the firmware player, with VREG on
and the target rails powered.  The serial port is closed, but the terminal
prompt does not return.

| Exit | When it works | How |
|---|---|---|
| `--leave` | the same successful invocation | add it to the run |
| reset button | always | press reset; VREG goes off |
| `0x0F` twice | player command loop only | a raw-serial host writes the pair |

`--leave` sends `0x0F` once to leave the player for `BBIO1` (no reply), then
`0x0F` again for the terminal reset, requires the `0x01` acknowledgement and
drains the banner.  It adds `Returned to the Bus Pirate terminal.`  It is
opt-in because a `0x0F` sent while a run is still in progress can be consumed
as stream data.

`--leave` is not attempted after a player error (exit `1`) or a host failure
(exit `2`), because the player may still be busy.  It also cannot rescue a
board left in the player by an earlier command: a new invocation cannot enter
binary mode until the board is out.  Use the reset button or the raw pair in
[workflow.md](workflow.md#6-leave-the-player-and-recover-the-terminal).

## The removed C players

Earlier releases shipped two C programs, a Windows build
(`BP4_Full_bitbang_XSVFplayer_Win.exe`) and a cross-platform build
(`BPXSVFplayer`).  They were removed after `bp xsvf` replaced them.  A
reader following older instructions that name those files, a `package/`
folder, or a bundled converter folder is looking at a tree that no longer
exists; this set documents `bp xsvf` only.

## Related docs

| Doc | Why |
|---|---|
| [workflow.md](workflow.md) | the end-to-end sequence and worked sessions |
| [player.md](player.md) | the firmware player, handshake and result bytes |
| [converter.md](converter.md) | producing the XSVF file |
| [wiring.md](wiring.md) | wires, rails and levels |
| [limits-and-errors.md](limits-and-errors.md) | limits, messages, recovery |
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player's byte protocol |
