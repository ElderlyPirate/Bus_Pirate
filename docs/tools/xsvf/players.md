# XSVF players — Windows and cross-platform

Part of the XSVF host-tool set (`docs/tools/xsvf/`).  This page owns the two
host programs that stream an XSVF file: what ships with each, which to use,
the command-line options, the serial settings, and worked commands.  The
firmware side is in [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md); the
converter is in [converter.md](converter.md).

## Scope

Covered: the Windows and cross-platform players, their files, options, port
settings and output.  Not covered: the firmware player's in-mode protocol
(link above), the SVF→XSVF translation ([converter.md](converter.md)), and
the end-to-end sequence ([workflow.md](workflow.md)).

## What the host player does

The host player is a PC program; the Bus Pirate does the JTAG work.  On
start it:

1. Opens the serial port at the requested speed, 8N1, no flow control.
2. Sends NUL bytes until the Bus Pirate answers `BBIO1` (the 20-NUL binary
   entry sequence).
3. Sends `0x18` and waits for the four-byte `XSV1` marker.
4. Optionally sends `0x01` (`-r`, chain reset).
5. Optionally sends `0x02` (`-x`, chain scan) and prints the reply.
6. Sends `0x03` and streams the file in chunks; the `0xFF` request and the
   16-bit length framing are in the
   [JTAG page](../../protocol/jtag/xsvf.md).
7. Stops when the file is used up and prints the reply it read.

All of this is the host player's own; do not pre-enter the binary layer or
the firmware player.  The in-mode framing (the `0xFF` handshake, the
4096-byte buffer, the result bytes) is owned by the
[JTAG page](../../protocol/jtag/xsvf.md).

## The two builds

| | Windows player | Cross-platform player |
|---|---|---|
| Program | `BP4_Full_bitbang_XSVFplayer_Win.exe` | `BPXSVFplayer` (Linux), `BPXSVFplayer.exe` (Windows) |
| Banner | `BusPirate XSVF Player V.02` | `BusPirate XSVF Player V.01` |
| Folder | `scripts/BP4_XSVFplayer_Win/` | `scripts/BPXSVFPlayer/` |
| Ready binary | yes, in the folder | yes, under `package/` |
| Build files | Dev-C++ `.dev` + `.layout`, a Windows makefile | portable sources, `Makefile`, Code::Blocks `.cbp` |
| Options | same table below | same table below |
| `-x` on a dead chain | gives up after five tries | no overall limit; on Linux it retries indefinitely |

The two programs are the same tool in two builds.  The Windows one has a
bounded chain-scan wait and prints `ok` after its entry steps; the
cross-platform one is the portable build with prebuilt Linux and Windows
binaries.

## What ships

| Item | Windows player folder | Cross-platform folder |
|---|---|---|
| player | `BP4_Full_bitbang_XSVFplayer_Win.exe` | `package/BPXSVFplayer`, `package/BPXSVFplayer.exe` |
| sample XSVF | `xc95test.xsvf`, `VHDL-CPLDIntro2Ledbutton.xsvf` | `xc95test.xsvf` (also in `package/`) |
| converter | `SVF2XSVF/` (`svf2xsvf502.exe`, `mergexsvf.exe`, `svf2XSVF.bat`) | `SVF2XSVF/{hp,lin,nt,sol}/…`, `SVF2XSVF/ReadMe.txt` |
| build files | `.dev`, `.layout`, Windows makefile | sources, `Makefile`, `.cbp` |
| example calls | `run.bat`, `run2.bat` | `package/run.bat` |

The bundled batch files are examples with a hard-coded port (`com10`,
`com18`); edit the port and file names before using them.  The samples are
prebuilt example streams; a real run needs a file built for your target
chain.

The Windows folder also carries `run_log.bat`, which is stale and
unsupported: it calls `BP4_XSVF_Win` (not shipped; the executable is
`BP4_Full_bitbang_XSVFplayer_Win.exe`) and uses `com13`.  Use `run.bat` or
`run2.bat` instead.

## Choosing a player

- **Windows:** prefer the Windows player — it is the ready executable and its
  `-x` chain scan gives up after five tries.  The cross-platform
  `package/BPXSVFplayer.exe` has the same options if you prefer the portable
  build, but its `-x` has no overall limit; on Linux it retries
  indefinitely.
- **Linux:** use `package/BPXSVFplayer`, or build it with the `Makefile`.
  Do not use `-x` on a chain that may not answer: the portable scan never
  gives up on Linux.
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
| `-s` | speed | host port speed, default `115200`.  Host-side only; it does not change the Bus Pirate's baud. |
| `-f` | file | XSVF file to stream.  Required in practice (see below). |
| `-x` | — | send `0x02` and print a chain scan before the file |
| `-r` | — | send `0x01` and reset the chain before the file |

- **The help text is misleading about `-f`.**  It marks `-f` optional for
  `-x` and `-r`, but the program exits with `No file specified. Need an
  input xsvf file` if `-f` is missing.  Always pass a file.
- There is no `-h`.  Run the program with no arguments to print the help
  (banner + usage); an unknown option prints `Invalid argument` and the help.
- `-x` prints one read of the reply and does not use the count byte to size
  the read, so the line is informational.  The first byte is the byte count
  (`4 × devices`); the IDCODEs follow, four bytes per device in chain order.
- `-r` runs before `-x` and before the file.

## Serial settings

| Setting | Value |
|---|---|
| data bits | 8 |
| parity | none |
| stop bits | 1 |
| flow control | none |
| speed | `-s`, default 115200 |

The host players set the host port themselves.  On v4 the USB CDC rate is host
metadata; on v3 the firmware's own baud would have to match, but there is no
v3 player to talk to.  Close the terminal program before running a host
tool: the port must be free (Windows opens it exclusively).

The board must be at the terminal prompt (`HiZ>` or another mode prompt) so
the NUL entry can reach the binary layer.  If it is in a configuration menu
or still in the firmware player from a previous run, reset it first.

## Worked command — Windows

```
> BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f xc95test.xsvf -x -r
-----------------------------------------------------------------------------

 BusPirate XSVF Player V.02
 http://www.dangerousprototypes.com

-----------------------------------------------------------------------------
 Entering binary mode ... ok
 Switching to XSVF mode ... ok
 Performing Reset..
 Done

 Performing Chain Scan..
 Chain Scan Result: 04 30 06 09 00

 File is 53289 bytes,  Read 53289 bytes
 Opening Bus Pirate on COM5 at 115200bps, using XSVF file xc95test.xsvf
 Entering XSVF Player Mode
 Waiting for first data request...ok
 Sending 4096 Bytes (1000)...ok
 Sending 4096 Bytes (2000)...ok
 (chunk lines elided)
 Sending 41 Bytes (D029)...ok
 End of operation reply: XSVF_ERROR_NONE
 Success!
 Thank you for playing! :-)

>
```

The `04` is the byte count (`4 × 1 device`); the four bytes after it are the
device's IDCODE, in chain order.  The chunk offsets are hex, so `1000` is
4096 bytes; the final `D029` is 53289, the whole file.  Elided lines repeat
the `Sending 4096 Bytes (…)...ok` pattern.

With only `-r` (no `-x`), the `Performing Chain Scan..` lines are absent.
With neither, the run goes straight from `Switching to XSVF mode` to the
file-size line, `Opening Bus Pirate …`, and `Entering XSVF Player Mode`.

## Worked command — cross-platform

```
> BPXSVFplayer -p /dev/ttyACM0 -s 115200 -f xc95test.xsvf -r
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
 (chunk lines elided)
 Sending 41 Bytes (D029)...ok
 End of operation reply: XSVF_ERROR_NONE
 Success!
 Thank you for playing! :-)
```

The portable build omits the `ok` after the entry lines.  Its `-x` also
prints a `Waiting for a chain scan reply` line before the result; the scan
has no overall limit, so on Linux it keeps retrying if the chain does not
answer.  Do not use `-x` on a chain that may not answer.

## Reading the output

| Line | Meaning |
|---|---|
| `Entering binary mode ... ok` | the NUL entry reached `BBIO1` |
| `Switching to XSVF mode ... ok` | `0x18` was accepted and `XSV1` arrived |
| `Performing Reset..` / `Done` | `-r` sent `0x01` |
| `Performing Chain Scan..` | `-x` sent `0x02` |
| `Chain Scan Result:` | count byte (`4 × devices`), then the IDCODE bytes |
| `File is N bytes, Read N bytes` | the host loaded the whole file |
| `Entering XSVF Player Mode` | `0x03` was sent |
| `Waiting for first data request...` | waiting for the firmware player's `0xFF` |
| `Sending N Bytes (offset)...` | one chunk; offset is hex |
| `End of operation reply: <name>` | the reply read after the file |
| `Success!` | result `0x00` |
| `XSVF_READY_FOR_DATA` + `End of file reached.` | the file ended while the firmware player still wanted data; no result was read |
| `No reply.... Quitting.` | the host tool stopped waiting; the firmware player may still be running |

The host tool has finished when it prints `Thank you for playing!` and
returns to the shell prompt.  That does **not** mean the firmware player has
finished: `No reply.... Quitting.` also ends this way.  A long
`XRUNTEST`/`XWAIT` can outlast the host's patience; the firmware player may
still complete the run, so the target may already be configured, and `0x0F`
sent during the wait aborts it.  Wait, check the target, then reset before
replaying.  On success the firmware has
played the whole stream; the tool does not verify the target's behaviour.

## Exit and availability

The host tools close the port without sending `0x0F`.  After a normal run the
board stays in the firmware player loop with the regulator on: **press
reset** to leave it.  `0x0F` twice works only after the firmware player has
returned to its command loop — a result was printed, or the ~5 s stall has
expired ([workflow.md](workflow.md#6-leave-the-player)).  If the tool stopped
at `End of file reached.`, press reset, or wait for the ~5 s stall first.  If
it stopped at `No reply.... Quitting.`, press reset: the player may still be
running.  A second run cannot enter the binary layer until the board is out
of the player: the firmware player recognises `0x00` only at its command
loop.

Both builds require the v4 firmware player; on v3 the `0x18` byte is answered
with `0x00` and the tool stops with an "Unable to activate … XSVF player
mode" message.  The board is left in the binary layer: send `0x0F` or press
reset to get the terminal back.  The result names, limits and recovery
actions are in [limits-and-errors.md](limits-and-errors.md).

## Related docs

| Doc | Why |
|---|---|
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player and stream framing |
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | pin map, levels, VREG/VCC |
| [converter.md](converter.md) | producing the XSVF file |
| [workflow.md](workflow.md) | the end-to-end sequence and exit |
| [limits-and-errors.md](limits-and-errors.md) | limits, result messages, recovery |
| [../../protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and framing |
