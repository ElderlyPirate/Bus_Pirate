# XSVF host players

Part of the XSVF host-tool set.  This page owns the two PC programs that
stream an XSVF file: what ships, which build to use, the options, the serial
settings, the chain scan, the output, and the exit status.  The firmware side
is [player.md](player.md); conversion is [converter.md](converter.md).

## Scope

Covered: the Windows and cross-platform host players, their files, options,
port settings, output and exit status.  Not covered: the firmware player's
protocol ([player.md](player.md)), the SVF→XSVF translation
([converter.md](converter.md)), and the end-to-end sequence
([workflow.md](workflow.md)).

## What a host player does

1. Opens the serial port at the requested speed, 8N1, no flow control.
2. Sends NUL bytes until the board answers `BBIO1`.
3. Sends `0x18` and waits for the four-byte `XSV1` marker.
4. Optionally sends `0x01` (`-r`) to reset the chain.
5. Optionally sends `0x02` (`-x`) and reads the framed reply.
6. Sends `0x03` and streams the file, one chunk per ready byte.
7. Stops when the file is used up or the player returns a result.  A result
   stops either source build; a player error returns `1`, and no further
   chunk is sent.  `No reply.... Quitting.` occurs only when the player gives
   no result.

The scan is framed and drained before `0x03`, so `-x` with `-f` is safe in a
source build ([the chain scan](#the-chain-scan--x)).  The tracked legacy
prebuilt EXEs still use the old unsized scan; with those, run `-x` alone,
then reset before the file run.

The chunk handshake and the result byte are in
[player.md](player.md#playing-a-stream-0x03).  Do not pre-enter the binary
layer or the player: the host player does both.

## The two builds

| | Windows player | Cross-platform player |
|---|---|---|
| Program | `BP4_Full_bitbang_XSVFplayer_Win.exe` | `BPXSVFplayer` (Linux), `BPXSVFplayer.exe` (Windows) |
| Banner | `BusPirate XSVF Player V.02` | `BusPirate XSVF Player V.01` |
| Folder | `scripts/BP4_XSVFplayer_Win/` | `scripts/BPXSVFPlayer/` |
| Ready binary | legacy prebuilt; build source for current behavior | legacy prebuilt; build source for current behavior |
| Build files | Dev-C++ project, Windows makefile | `Makefile`, Code::Blocks project |
| `-f` with `-x`/`-r` | not required; `-x`/`-r` run alone and return `0` | not required; `-x`/`-r` run alone and return `0` |
| Exit status | `0`, `1`, `2`; see below | `0`, `1`, `2`; see below |
| Entry `ok` text | prints `ok` after each entry step | no `ok` after the two entry lines; run `ok` suffixes still appear |

The two programs are the same tool in two builds.  The Windows build adds the
`ok` words; both source builds accept the same options and return the same
`0`/`1`/`2` status.  The tracked executables are legacy; see
[Sources vs prebuilt binaries](#sources-vs-prebuilt-binaries).

## Sources vs prebuilt binaries

The tracked executables (`BP4_Full_bitbang_XSVFplayer_Win.exe`,
`package/BPXSVFplayer`, `package/BPXSVFplayer.exe`) are the **legacy**
builds; they have not been rebuilt for the host fixes.  They do not include
the current host behavior: the framed and drained chain scan, the
early-result stop, the `0`/`1`/`2` exit mapping, or the clean bad-file exit.

Build the player from `scripts/BP4_XSVFplayer_Win/` with the Dev-C++ project
or Windows makefile, or from `scripts/BPXSVFPlayer/` with its `Makefile` or
Code::Blocks project.  A source build is the current player described on
these pages.

## What ships

| Item | Windows player folder | Cross-platform folder |
|---|---|---|
| player | `BP4_Full_bitbang_XSVFplayer_Win.exe` (legacy) | `package/BPXSVFplayer`, `package/BPXSVFplayer.exe` (legacy) |
| sample XSVF | `xc95test.xsvf`, `VHDL-CPLDIntro2Ledbutton.xsvf` | `xc95test.xsvf` (also in `package/`) |
| converter (Windows) | `SVF2XSVF/` with `svf2xsvf502.exe`, `mergexsvf.exe`, `svf2XSVF.bat` | `SVF2XSVF/nt/` |
| converter (Unix) | — | `SVF2XSVF/{lin,hp,sol}/` |
| converter ReadMe | — | `SVF2XSVF/ReadMe.txt` |
| build files | Dev-C++ project, Windows makefile | `Makefile`, Code::Blocks project |
| example calls | `run.bat`, `run2.bat` | `package/run.bat` |

Both sample files are 53289 bytes; `xc95test.xsvf` is the same file in both
trees.  They are prebuilt examples; a real run needs a file built for your
target chain.

The bundled batch files hard-code a port: `run.bat` and `run2.bat` use
`com10`, and the cross-platform `package/run.bat` uses `com18`.  Edit the port
and the file name before using them.  They combine `-x` with `-f`; that is
safe in a source build, but with a tracked legacy prebuilt, run `-x` alone,
then reset before the file, or rebuild
([the chain scan](#the-chain-scan--x)).  The Windows folder also
carries `run_log.bat`, which is stale: it calls `BP4_XSVF_Win`, an executable
that is not shipped, and uses `com13`.  Treat the batch files as syntax
examples, not as run recipes.

## Choosing a player

- **Windows:** build either player from source (the tracked `.exe` is the
  legacy prebuilt).  The Windows build prints the step-by-step `ok` lines and
  the result byte in hex; the cross-platform build prints the result name
  only.  Both source builds stop at an early player error and return `1`.
- **Linux:** build `BPXSVFplayer` from `scripts/BPXSVFPlayer/` with the
  `Makefile`; the `package/BPXSVFplayer` binary is the legacy prebuilt.
- **Building:** the Windows folder carries the Dev-C++ project; the
  cross-platform folder carries a `Makefile` and a Code::Blocks project.  No
  firmware change is needed for either.

## Options

Both host players parse the same options; the order does not matter.  The
examples assume a source build; the tracked prebuilt executables are legacy
([above](#sources-vs-prebuilt-binaries)).

```
BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f design.xsvf -r
BPXSVFplayer -p /dev/ttyACM0 -s 115200 -f design.xsvf -r
```

| Option | Argument | Meaning |
|---|---|---|
| `-p` | device | serial port, e.g. `COM5` or `/dev/ttyACM0`.  Required. |
| `-s` | speed | host port speed, default `115200`.  Host-side only. |
| `-f` | file | XSVF file to stream.  Required only to play a file. |
| `-x` | — | send `0x02` and print a chain scan; no `-f` needed |
| `-r` | — | send `0x01` and reset the chain; no `-f` needed |

- **`-f` is required only to play a file.**  Both source builds run
  `-x`/`-r` without `-f` and return `0`; the help text calls `-f` optional for
  both.  `No file specified. Need an input xsvf file` and exit `2` occur only
  when none of `-f`, `-x` or `-r` is given.  The cross-platform build ends a
  stand-alone scan/reset with `Thank you for playing!`; the Windows build
  stops after it.
- There is no `-h`.  Run the program with no arguments to print the banner and
  the help; an unknown option prints ` Invalid argument` and the help.
- `-r` runs before `-x`, and both run before the file.
- `-x` sends `0x02`, reads the count byte, and then exactly `count` payload
  bytes; the line is complete.  `-x` with `-f` is safe in a source build.  A
  failed scan prints `Chain scan: incomplete reply (bounded timeout)` or
  `Chain scan: failed to send the command` and returns `2` before the file is
  played.  The tracked legacy prebuilt EXEs use the old unsized scan; with
  those, run `-x` alone, then reset before the file run.  The first byte is
  the byte count (`4 × devices`); the IDCODE bytes follow, four per device,
  nearest-TDO device first.
- `-s` does not change the Bus Pirate's own rate.  On v4 the USB CDC link is
  host-controlled; leave the default.

## Serial settings

| Setting | Value |
|---|---|
| data bits | 8 |
| parity | none |
| stop bits | 1 |
| flow control | none |
| speed | `-s`, default 115200 |

The host players configure the port themselves.  Close the terminal program
before running one: the port must be free, and Windows opens it exclusively.
On Linux the port is usually `/dev/ttyACM0`; the user must be allowed to open
it (commonly the `dialout` group).

The board must be at the terminal prompt (`HiZ>` or another mode prompt) so
the NUL entry can reach the binary layer.  If it is in a configuration menu,
or still in the firmware player from a previous run, reset it first.

## The chain scan (`-x`)

The scan is optional and informational; the file is what configures the
target.  The reply is one count byte equal to four times the number of
devices, then four IDCODE bytes per device, least-significant byte first,
nearest-TDO device first.

```
 Chain Scan Result: 04 93 40 60 59
```

That is one device: count `04`, then four IDCODE bytes.  Read them
least-significant byte first: `93 40 60 59` is the example IDCODE
`0x59604093`.  For two devices the count is `08` and eight bytes follow.

The current source builds read the count byte and exactly `count` payload
bytes.  A fragmented reply is read to the end, and an incomplete reply returns
`2` before `0x03`.  `-x` with `-f` is therefore safe in a source build.

**Legacy prebuilt.**  The tracked prebuilt EXEs still use the old unsized
scan.  With one of those, run `-x` alone, then reset before the file run (or
rebuild); a short line can leave bytes that the run reads as its result.

| Symptom | Cause |
|---|---|
| count `00`, no IDCODE bytes | TDO stuck high, or no power on the target |
| count `FC`, all-zero bytes | TDO stuck low; the probe hit its 63-device cap |
| count `FC`, plausible IDCODEs | a real chain over 63 devices; only the first 63 are reported |
| `Chain scan: incomplete reply (bounded timeout)` | the count or payload did not arrive within five attempts; check TDO, power and wiring |
| `Chain scan: failed to send the command` | the `0x02` write failed; check the port |
| a short line (legacy prebuilt only) | the old unsized read returned early; run `-x` alone or rebuild |

Both current source builds give up after five attempts.  The tracked legacy
prebuilt prints ` Got no reply for a Chain scan` instead.

## Reading the output

| Line | Meaning |
|---|---|
| `Entering binary mode ... ok` | the NUL entry reached `BBIO1` (cross-platform: `Entering binary mode...`, no `ok`) |
| `Switching to XSVF mode ... ok` | `0x18` was accepted and `XSV1` arrived (cross-platform: no `ok`) |
| `Performing Reset..` / `Done` | `-r` sent `0x01` |
| `Performing Chain Scan..` | `-x` sent `0x02` |
| `Chain Scan Result:` | count byte, then IDCODE bytes, nearest-TDO first |
| `File is N bytes, Read N bytes` | the host loaded the whole file |
| `Entering XSVF Player Mode` | `0x03` was sent |
| `Waiting for first data request...` | waiting for the player's `0xFF` |
| `Sending N Bytes (total)...` | one chunk; the number in parentheses is the running total after the chunk, hex |
| `End of operation reply: <name>` | the result byte; an early player error stops either source build and returns `1` |
| `Success!` | result `0x00` |
| `XSVF_READY_FOR_DATA` + `End of file reached without a completion result.` | the file ended before completion; the run returns `2` |
| `No reply.... Quitting.` | the player gave no result; the player may still be running |

Windows appends `(0xNN)` to the `End of operation reply` line; the
cross-platform build prints the name only.

A file run has finished when it prints `Thank you for playing!` and returns
to the shell.  That does **not** mean the firmware player has finished:
`No reply.... Quitting.` also ends this way.  A long `XRUNTEST` or `XWAIT`
can outlast the host's patience; the player may still complete the run, so
the target may already be configured.  A raw-serial host can abort it with
`0x0F` during the wait.  Wait, check the target, then reset before replaying.

## Exit status and the next run

| Build | Status |
|---|---|
| both source builds | `0` success; `1` player-result error; `2` host/transport/setup failure |

- `0` is an `XSVF_ERROR_NONE` result, or a stand-alone `-x`/`-r` operation
  that ran and closed cleanly.
- `1` is a player error result.
- `2` is a host, transport or setup failure: no reply, incomplete chain scan,
  bad or empty file, `End of file reached without a completion result.`, or a
  serial or player-start failure.

The mapping is for the current source builds.  The tracked legacy prebuilt
EXEs use the old behavior; see
[Sources vs prebuilt binaries](#sources-vs-prebuilt-binaries).

The success check is `End of operation reply: XSVF_ERROR_NONE` (Windows adds
`(0x00)`) followed by `Success!`; do not trust `0` alone.

The host players close the port without sending `0x0F`.  After a normal run
the board stays in the firmware player loop with the regulator (VREG) on:
**press reset** to leave it.  A raw-serial host can send `0x0F` twice instead,
but only once the player is back at its command loop — a result was printed,
or the ~5 s stall expired ([player.md](player.md#exit)).  The host players
cannot take that step: they have already closed the port.  The exact procedure
is in [workflow.md](workflow.md#6-leave-the-player).

A second run cannot enter the binary layer until the board is out of the
player.  If you start one anyway, the host gives up during its entry retries
and prints ` Unable to activate BP4 Binary mode` (Windows) or
` Unable to activate the BP binary mode` (cross-platform).

Both source builds require the v4 firmware player.  On v3 the `0x18` byte is
answered with `0x00`, and the tool then stops with the Windows message
` Unable to activate XSVF player mode` or the cross-platform message
` Unable to activate the XSVF player mode`, returning `2`.  The board is
left in the binary layer: press reset; a raw-serial host can send `0x0F`
([workflow.md](workflow.md#6-leave-the-player)).

## Worked command — Windows

This is a source-build transcript.  The tracked prebuilt EXE is the legacy
build and prints the old output.

```
> BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f xc95test.xsvf -r
-----------------------------------------------------------------------------

 BusPirate XSVF Player V.02
 http://www.dangerousprototypes.com

-----------------------------------------------------------------------------
 Entering binary mode ... ok
 Switching to XSVF mode ... ok
 Performing Reset..
 Done

 File is 53289 bytes,  Read 53289 bytes
 Opening Bus Pirate on COM5 at 115200bps, using XSVF file xc95test.xsvf
 Entering XSVF Player Mode
 Waiting for first data request...ok
 Sending 4096 Bytes (1000)...ok
 Sending 4096 Bytes (2000)...ok
 ...
 Sending 41 Bytes (D029)...ok
 End of operation reply: XSVF_ERROR_NONE (0x00)
 Success!
 Thank you for playing! :-)
```

The middle `Sending` lines are omitted.  The chunk counts are hex: `1000` is
4096 bytes, and the last full chunk totals `D000` (53248).  The final 41
bytes bring the total to `D029` (53289), the whole file.

With `-r` the reset block is present.  Without it, the run goes from
`Switching to XSVF mode ... ok` to the file-size line.  Add `-x` for a chain
scan in a source build; with a legacy prebuilt, scan separately.

## Worked command — cross-platform

This is a source-build transcript.  The `package/` binaries are the legacy
prebuilt.

```
$ BPXSVFplayer -p /dev/ttyACM0 -s 115200 -f xc95test.xsvf -r
-----------------------------------------------------------------------------

 BusPirate XSVF Player V.01
 http://www.dangerousprototypes.com

-----------------------------------------------------------------------------
 Entering binary mode...
 Switching to XSVF mode
 Performing Reset..
 Done

 File is 53289 bytes,  Read 53289 bytes
 Opening Bus Pirate on /dev/ttyACM0 at 115200bps, using XSVF file xc95test.xsvf
 Entering XSVF Player Mode
 Waiting for first data request...ok
 Sending 4096 Bytes (1000)...ok
 ...
 Sending 41 Bytes (D029)...ok
 End of operation reply: XSVF_ERROR_NONE
 Success!
 Thank you for playing! :-)
```

The middle `Sending` lines are omitted.  The cross-platform build omits the
`ok` after the two entry lines and prints the result name without the hex
suffix.  A probe-only run is in
[workflow.md](workflow.md#a-probe-only-run); both source builds support it.

## Related docs

| Doc | Why |
|---|---|
| [player.md](player.md) | the firmware player, handshake and result bytes |
| [converter.md](converter.md) | producing the XSVF file |
| [workflow.md](workflow.md) | the end-to-end sequence and worked sessions |
| [wiring.md](wiring.md) | wires, rails and levels |
| [limits-and-errors.md](limits-and-errors.md) | limits, messages, recovery |
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player's byte protocol |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and framing |
