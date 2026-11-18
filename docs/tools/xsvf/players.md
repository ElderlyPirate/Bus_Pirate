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
5. Optionally sends `0x02` (`-x`) and prints the reply.
6. Sends `0x03` and streams the file, one chunk per ready byte.
7. Stops when the file is used up, or at a result on the cross-platform
   build; the Windows build sends one more chunk after an early error and then
   ends with `No reply.... Quitting.`

Points 5 and 6 share one port.  A host that sends both must read the scan
reply in full before `0x03`; the shipped players do not, so run `-x` alone
([the chain scan](#the-chain-scan--x)).

The chunk handshake and the result byte are in
[player.md](player.md#playing-a-stream-0x03).  Do not pre-enter the binary
layer or the player: the host player does both.

## The two builds

| | Windows player | Cross-platform player |
|---|---|---|
| Program | `BP4_Full_bitbang_XSVFplayer_Win.exe` | `BPXSVFplayer` (Linux), `BPXSVFplayer.exe` (Windows) |
| Banner | `BusPirate XSVF Player V.02` | `BusPirate XSVF Player V.01` |
| Folder | `scripts/BP4_XSVFplayer_Win/` | `scripts/BPXSVFPlayer/` |
| Ready binary | yes, in the folder | yes, under `package/` |
| Build files | Dev-C++ project, Windows makefile | `Makefile`, Code::Blocks project |
| `-f` with `-x`/`-r` | required to continue; the tool stops after the reset/scan | optional; the tool performs them alone |
| Exit status | `0` after any run; `-1` for setup failure | `0`, `1`, `2` or `-1`; see below |
| Entry `ok` text | prints `ok` after each entry step | no `ok` after the two entry lines; run `ok` suffixes still appear |

The two programs are the same tool in two builds.  The Windows build adds the
`ok` words and a stricter `-f` rule; the cross-platform build returns an exit
status (see [Exit status and the next run](#exit-status-and-the-next-run)).

## What ships

| Item | Windows player folder | Cross-platform folder |
|---|---|---|
| player | `BP4_Full_bitbang_XSVFplayer_Win.exe` | `package/BPXSVFplayer`, `package/BPXSVFplayer.exe` |
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
and the file name before using them, and drop `-x`: all three combine it with
`-f`, which is the unsafe combination in
[the chain scan](#the-chain-scan--x).  The Windows folder also carries
`run_log.bat`, which is stale: it calls `BP4_XSVF_Win`, an executable that is
not shipped, and uses `com13`.  Treat the batch files as syntax examples, not
as run recipes.

## Choosing a player

- **Windows:** either build runs a clean file.  The Windows player is the
  ready executable and prints the step-by-step `ok` lines.  The cross-platform
  `package/BPXSVFplayer.exe` stops on an early error and returns a status.
- **Linux:** use `package/BPXSVFplayer`, or build it with the `Makefile`.
- **Building:** the Windows folder carries the Dev-C++ project; the
  cross-platform folder carries a `Makefile` and a Code::Blocks project.  No
  firmware change is needed for either.

## Options

Both host players parse the same options; the order does not matter.

```
BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f design.xsvf -r
BPXSVFplayer -p /dev/ttyACM0 -s 115200 -f design.xsvf -r
```

| Option | Argument | Meaning |
|---|---|---|
| `-p` | device | serial port, e.g. `COM5` or `/dev/ttyACM0`.  Required. |
| `-s` | speed | host port speed, default `115200`.  Host-side only. |
| `-f` | file | XSVF file to stream.  See the per-build rule below. |
| `-x` | — | send `0x02` and print a chain scan; run it without `-f` |
| `-r` | — | send `0x01` and reset the chain before the file |

- **`-f` differs by build.**  The cross-platform player performs `-x` and
  `-r` on their own.  The Windows player runs them and then stops with
  ` No file specified. Need an input xsvf file`; the help text's "`-f` is
  optional" is true only for the cross-platform build.  The Windows player
  performs the scan before it stops, so an `-x`-only invocation still prints
  the scan.
- There is no `-h`.  Run the program with no arguments to print the banner and
  the help; an unknown option prints ` Invalid argument` and the help.
- `-r` runs before `-x`, and both run before the file.
- `-x` prints one read and does not use the count byte to size it, so the
  line is informational and can be short.  Do not combine it with `-f`; run
  it alone and reset before the file run
  ([the chain scan](#the-chain-scan--x)).  The first byte is the byte count
  (`4 × devices`); the IDCODE bytes follow, four per device, nearest-TDO
  device first.
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
The host prints whatever one read returned, so a long line can be truncated;
do not use it as a chain inventory.

**Do not combine `-x` with `-f` in one invocation.**  Neither build uses the
count byte to size the read, and neither drains the port before sending
`0x03`.  If the read returns fewer bytes than the whole reply (one count
byte plus `count` bytes), the leftovers are read as the run's result.  A
leftover `0x00` prints `XSVF_ERROR_NONE` and `Success!` before the run has
finished: the cross-platform build stops there with the file unsent and
returns `0`, while the Windows build prints the extra reply and then sends
one more chunk.  Other leftovers print a spurious error or an invalid name; the
Windows build can also terminate.  Run `-x` in its own invocation,
press reset (the scan leaves the board in the player), then run the file with
`-r -f`.  `-r` sends no reply, so `-r -f` is safe; only `-x` has the
leftover problem.  A custom host that does both in one session must read the
count byte and exactly `count` more bytes before `0x03`.

The Windows build performs the scan and then stops with
` No file specified. Need an input xsvf file`; that is expected when `-x`
is used alone, and the scan has already run.  The bundled `run.bat`,
`run2.bat` and `package/run.bat` combine `-x` with `-f`; use them as syntax
examples, not as the run recipe.

| Symptom | Cause |
|---|---|
| count `00`, no IDCODE bytes | TDO stuck high, or no power on the target |
| count `FC`, all-zero bytes | TDO stuck low; the probe hit its 63-device cap |
| count `FC`, plausible IDCODEs | a real chain over 63 devices; only the first 63 are reported |
| ` Got no reply for a Chain scan` | the scan did not answer within the wait |
| a short line | the read returned before the whole reply; do not follow it with `-f` |

The Windows build appends `res` and `timeout_counter` to the no-reply line;
the cross-platform build appends `(bounded timeout)`.  Both give up after five
reads.

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
| `End of operation reply: <name>` | the result byte read after the run/stream; an early error stops the cross-platform build, but the Windows build sends one more chunk |
| `Success!` | result `0x00` |
| `XSVF_READY_FOR_DATA` + `End of file reached.` | the file ended while the player still wanted data |
| `No reply.... Quitting.` | the host stopped waiting; the player may still be running |

The host has finished when it prints `Thank you for playing!` and returns to
the shell.  That does **not** mean the firmware player has finished:
`No reply.... Quitting.` also ends this way, and the Windows build reaches it
after an early result because it sends one more chunk.  A long `XRUNTEST` or
`XWAIT` can outlast the host's patience; the player may still complete the
run, so the target may already be configured.  A raw-serial host can abort it
with `0x0F` during the wait.  Wait, check the target, then reset before
replaying.

## Exit status and the next run

| Build | Status |
|---|---|
| Windows player | `0` after any run, whatever the result; `-1` for a setup failure |
| cross-platform player | `0`, `1`, `2` or `-1`; the values are below |

- `0` is a clean result, **or** `End of file reached.` on the cross-platform
  build.  The latter produced no result byte; treat it as a failure even
  though the status is `0`.
- `1` is a firmware error result.
- `2` is a transport failure: no reply, a chain scan with no reply, or a
  reset write that failed.
- `-1` is a setup failure: port, `BBIO1`, `XSV1`, file open, or a run with
  neither `-f` nor `-x`/`-r`.  A Unix shell reports it as `255`.
- On the Windows build, `-x`/`-r` without `-f` prints
  ` No file specified. Need an input xsvf file` and still exits `0`.

On the Windows build, a bad `-f` path is not reported this way: it prints
`Error opening file` and may terminate
([limits-and-errors.md](limits-and-errors.md#host-messages-and-states)).

The success check is `End of operation reply: XSVF_ERROR_NONE` followed by
`Success!`; do not trust `0` alone.

The Windows build does not stop on an early error result: it prints the
result, sends one more chunk, then ends with `No reply.... Quitting.` and
still returns `0`.  The cross-platform build stops at the result and returns
`1`.  The failure transcript in
[workflow.md](workflow.md#a-run-that-fails-the-tdo-check) uses the
cross-platform build for that reason.

The host players close the port without sending `0x0F`.  After a normal run
the board stays in the firmware player loop with the regulator (VREG) on:
**press reset** to leave it.  A raw-serial host can send `0x0F` twice instead,
but only once the player is back at its command loop — a result was printed,
or the ~5 s stall expired ([player.md](player.md#exit)).  The shipped players
cannot take that step: they have already closed the port.  The exact procedure
is in [workflow.md](workflow.md#6-leave-the-player).

A second run cannot enter the binary layer until the board is out of the
player.  If you start one anyway, the host gives up during its entry retries
and prints ` Unable to activate BP4 Binary mode` (Windows) or
` Unable to activate the BP binary mode` (cross-platform).

Both builds require the v4 firmware player.  On v3 the `0x18` byte is
answered with `0x00`, and the tool then stops, with the Windows message
` Unable to activate XSVF player mode` or the cross-platform message
` Unable to activate the XSVF player mode`.  The board is left in the binary
layer: press reset; a raw-serial host can send `0x0F`
([workflow.md](workflow.md#6-leave-the-player)).

## Worked command — Windows

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
 End of operation reply: XSVF_ERROR_NONE
 Success!
 Thank you for playing! :-)
```

The middle `Sending` lines are omitted.  The chunk counts are hex: `1000` is
4096 bytes, and the last full chunk totals `D000` (53248).  The final 41
bytes bring the total to `D029` (53289), the whole file.

With `-r` the reset block is present.  Without it, the run goes from
`Switching to XSVF mode ... ok` to the file-size line.  A chain scan is a
separate invocation ([above](#the-chain-scan--x)).

## Worked command — cross-platform

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
`ok` after the two entry lines.  A probe-only run on this build is in
[workflow.md](workflow.md#a-probe-only-run).

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
