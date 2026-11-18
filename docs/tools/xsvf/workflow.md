# XSVF workflow — from file to configured target

Part of the XSVF host-tool set.  This page is the end-to-end sequence: get a
file, wire the target, prepare the link, run a host player, read the result,
and leave the firmware player.  Player options are in
[players.md](players.md), conversion in [converter.md](converter.md), wiring
in [wiring.md](wiring.md), and failures in
[limits-and-errors.md](limits-and-errors.md).

## 1. Get the XSVF file

- Preferred: generate the XSVF directly with iMPACT for the device or devices
  in the chain.
- If you have only an SVF: convert it with `svf2xsvf502`
  ([converter.md](converter.md)).
- **Shift gate:** the largest `XSDRSIZE` (or `XSIR`/`XSIR2` length) in the
  file must be **400 bits or less**; divide by 8 and round up, the result
  must be 50 or less (`ceil(value / 8) ≤ 50`).  The `-a` text listing shows
  the values ([converter.md](converter.md#checking-the-shift-size)).  An
  XSVF written directly by iMPACT has no listing: keep the SVF and convert
  it with `-a` for the check, inspect the XSVF with a parser, or treat
  `XSVF_ERROR_DATAOVERFLOW` (`0x06`) as the check after a run (the run stops
  there; reset and use another host).

Do not feed an `.svf` file to a host player: the firmware player reads XSVF
only.

## 2. Wire the target

| Bus Pirate | JTAG |
|---|---|
| MOSI | TDI |
| MISO | TDO |
| CLK | TCK |
| CS | TMS |
| GND | GND |

- **No pull-up is needed**: the player drives TDI/TCK/TMS push-pull.  SRST
  and TRST are not used.
- **The player powers the target on entry.**  Entry switches on the on-board
  regulator (VREG), which drives the `3.3V` and `5.0V` header rails.  Wire the
  target's VCC to **one rail only, never both**, and disconnect the target's
  own supply first.
- **3.3 V push-pull only.**  A 1.8 V or 2.5 V target cannot use the player;
  a 5 V target may not see 3.3 V as a valid high.  Do not drive a lower
  voltage chain.  Details: [wiring.md](wiring.md).

## 3. Prepare the link

1. Connect the Bus Pirate by USB and note the serial port.
2. Close the terminal program and any other program using the port.
3. Leave the board at the terminal prompt (`HiZ>`).  If it is in a menu or
   still in the firmware player from a previous run, press reset.
4. Check the target header's orientation and ground, and release any leftover
   SRST/TRST level before the run
   ([wiring.md](wiring.md#reset-lines-and-the-unused-pins)).

## 4. Run a host player

Windows:

```
> BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f design.xsvf -r
```

Linux (cross-platform build):

```
$ BPXSVFplayer -p /dev/ttyACM0 -s 115200 -f design.xsvf -r
```

- `-p` is the port, `-s 115200` the normal speed, `-f` the XSVF file.
- Add `-r` to reset the chain first; it is a good default before a run and
  sends no reply, so it is safe with `-f`.
- **Do not add `-x` to this command.**  The scan reply is a variable-length
  read and the shipped players do not drain it before `0x03`; a short read
  leaves bytes that the run reads as its result.  Run the scan in its own
  invocation first ([players.md](players.md#the-chain-scan--x)), press reset,
  then run this command with `-r` only.
- The host player enters `BBIO1` and `XSV1` itself; do not pre-enter them.

The Windows build requires `-f` to continue; with `-x` or `-r` alone it
performs them and then stops with ` No file specified. Need an input xsvf
file`.  The cross-platform build runs `-x` and `-r` alone
([players.md](players.md#options)).

## 5. Watch the run

A clean run ends like this (tail of the full transcript in
[players.md](players.md#worked-command--windows)):

```
 Switching to XSVF mode ... ok
 ...
 Sending 41 Bytes (D029)...ok
 End of operation reply: XSVF_ERROR_NONE
 Success!
 Thank you for playing! :-)
```

The line that matters is `End of operation reply:` followed by
`XSVF_ERROR_NONE`, then `Success!`.  Any other result name is a failed run;
the actions are in [limits-and-errors.md](limits-and-errors.md#result-actions).

**The host player finishing is not the firmware player finishing.**
`Thank you for playing!` means the host stopped.  A long `XRUNTEST` or
`XWAIT` can outlast the host's patience, and the player may still complete
the run, so the target may already be configured.  A raw-serial host can
abort it with `0x0F` during the wait.  Wait, check the target, then reset
before replaying.

### A probe-only run

The cross-platform player performs `-x` without a file.  This answers "is
there a chain, and what does it report?" before any configuration:

```
$ BPXSVFplayer -p /dev/ttyACM0 -x
-----------------------------------------------------------------------------

 BusPirate XSVF Player V.01
 http://www.dangerousprototypes.com

-----------------------------------------------------------------------------
 Entering binary mode...
 Switching to XSVF mode
 Performing Chain Scan..
 Chain Scan Result: 04 93 40 60 59

 Thank you for playing! :-)
```

`04` is the count byte (`4 × 1 device`); the four bytes after it are the
IDCODE, least-significant byte first, nearest-TDO device first; here
`93 40 60 59` is `0x59604093`.  The scan is optional; read it as a wiring
check, not as a chain inventory ([players.md](players.md#the-chain-scan--x)).
The scan leaves the board in the player: press reset before the run command.

### A run that fails the TDO check

A file for a different chain, a swapped TDI/TDO or TCK/TMS pair, a missing
ground, or the target's VCC not on the wired rail all end the same way:

```
$ BPXSVFplayer -p /dev/ttyACM0 -s 115200 -f xc95test.xsvf -r
 Entering binary mode...
 Switching to XSVF mode
 Performing Reset..
 Done

 File is 53289 bytes,  Read 53289 bytes
 Opening Bus Pirate on /dev/ttyACM0 at 115200bps, using XSVF file xc95test.xsvf
 Entering XSVF Player Mode
 Waiting for first data request...ok
 Sending 4096 Bytes (1000)...ok
 Sending 4096 Bytes (2000)...ok
 End of operation reply: XSVF_ERROR_TDOMISMATCH
 Device did not respond as expected: XSVF_ERROR_TDOMISMATCH
 Thank you for playing! :-)
```

`XSVF_ERROR_MAXRETRIES` is the same failure after the file's retry count is
used up.  The cross-platform build stops at the first mismatch, so the chunk
lines end there.  The Windows build prints the same result, then sends one
more chunk; with no ready byte it ends with `No reply.... Quitting.` and
still exits `0`.  Check the wiring and the rail first, then the file.  On the
cross-platform build the board is back in the player loop; on the Windows
build the extra chunk can leave it elsewhere.  Press reset before the next
attempt.

## 6. Leave the player

**The host players do not exit the firmware player.**  After a normal run the
board is still in the player loop with VREG on, and the terminal is gone: no
prompt, and `W` cannot be reached.

| Exit | When it works | How |
|---|---|---|
| reset button | always | press reset; the terminal returns and VREG goes off |
| `0x0F` twice | command loop only | a raw-serial host writes `0x0F`, waits, writes `0x0F` |
| `0x0F` during a run | never | consumed as data or a busy byte |

The command loop is the state after a result was printed, or after the ~5 s
stall expired.  The reset button is the everyday exit.  The `0x0F` path is
for a custom host or a script: the shipped players close the port without
sending it, and a terminal program cannot be assumed to send a raw byte.
With pySerial:

```
python -c "import serial,time
p=serial.Serial('COM5',115200)
p.write(b'\x0f'); time.sleep(0.5)
p.write(b'\x0f'); time.sleep(0.5)
p.close()"
```

Use `/dev/ttyACM0` on Linux.  The first `0x0F` resets the chain and returns
to `BBIO1` silently; the second replies `0x01` and runs the terminal reset,
which switches VREG off and prints the prompt.  Reopen the terminal
afterwards; press Enter if the prompt is not shown.  If the last host line
was `End of file reached.`, wait for the ~5 s stall first.

Do **not** send `0x0F` while the player is still waiting for a chunk or busy
in a timed wait: it is consumed as data or as a busy byte, and two bytes can
leave the board deeper in the player.

| After the tool stopped at | What the player is doing | Exit |
|---|---|---|
| `End of file reached.` | waiting for a chunk | wait ~5 s, then the `0x0F` pair; or press reset |
| `No reply.... Quitting.` | possibly still running | do not replay; wait and check, then press reset |

While the player owns the port, `W` cannot be reached.  Reset to the terminal,
then `W` if you want the terminal to power the rails again — but enter a mode
first, because `W` is refused in `HiZ>`.  The reset also switches VREG off, so
the rails stay off until that `W`.  A target with its own supply can be
reconnected once the board is back at the terminal.  A second run cannot enter
the binary layer until the board is out of the player: the player ignores the
NUL bytes the host uses for entry.

## 7. If it fails

The tool prints a result name.  The mapping and the recovery actions are in
[limits-and-errors.md](limits-and-errors.md).  The common causes are a file
for a different chain, swapped TDI/TDO or TCK/TMS, a missing ground, or the
target's VCC not reaching it — the player drives the `3.3V` and `5.0V` rails
on entry, so check the wired rail and the ground.

To check the regulator, press reset, enter a mode (`m`), then `W`; `W` also
runs that mode's setup.  If it prints `VREG too low, is there a short?`, check
the rail for a short.  `W` is refused in `HiZ>`, so the mode step is required;
from inside the player the only symptom is a target that does not answer.

## Related docs

| Doc | Why |
|---|---|
| [players.md](players.md) | options, serial settings, full transcripts |
| [converter.md](converter.md) | producing and checking the XSVF file |
| [wiring.md](wiring.md) | wires, rails, levels |
| [limits-and-errors.md](limits-and-errors.md) | limits, messages, recovery |
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player and stream framing |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and framing |
