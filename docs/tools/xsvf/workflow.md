# XSVF workflow — from file to configured target

Part of the XSVF set.  This page is the end-to-end sequence: get a file,
install `bp`, wire the target, probe the chain, play the file, and leave the
player.  The command reference is in [players.md](players.md); conversion is
in [converter.md](converter.md); wiring is in [wiring.md](wiring.md); failures
are in [limits-and-errors.md](limits-and-errors.md).

## 1. Get the XSVF file

- **Preferred:** generate the XSVF directly with Xilinx iMPACT for the device
  or devices in the chain.
- **If you have only an SVF:** translate it with the external
  `svf2xsvf502` utility ([converter.md](converter.md)).
- **Check the shift size before wiring.**  The largest single DR or IR shift
  must be **400 bits or less**.  In a converter text listing, find the largest
  `XSDRSIZE` value; `ceil(bits / 8)` must be 50 or less.  A file that exceeds
  it stops with `XSVF_ERROR_DATAOVERFLOW`.
- A `.svf` file cannot be passed to `bp xsvf`.  The firmware reads XSVF only.

## 2. Install `bp` and find the port

`bp` is the host frontend; it is not a packaged download.  From the tool
directory, create and activate a virtual environment, then install it
([players.md](players.md#install-and-port)):

```
cd scripts/bp
python -m venv .venv
# Windows cmd:        .venv\Scripts\activate.bat
# Windows PowerShell: .venv\Scripts\Activate.ps1
# Linux/macOS:        source .venv/bin/activate
python -m pip install -e .
```

Find the board's serial port.  `bp ports` does not open or reset anything:

```
$ bp ports
COM1  ----:----  Communications Port (COM1)
COM5  04D8:FB00  USB Serial Device (COM5)  [Bus Pirate v4 application]
```

`04D8:FB00` is a v4 running its application; `04D8:FAFF` is a v4 in the
bootloader.  An FTDI bridge (`0403:6001`) is only a possible v3, not proof of
one.  Close any terminal program before running `bp xsvf`; Windows opens the
port exclusively.

Confirm the board before a run:

| `bp version` field | What to check |
|---|---|
| `Hardware type:` | reads `Bus Pirate v4` |
| `Firmware version:` | the current `ElderlyPirate Firmware v9.0` image includes the player |

If the board is in a menu or still in the player from an earlier run, press
reset first.  `bp` must see the terminal prompt before it can send the binary
entry sequence.

## 3. Wire the target

| Bus Pirate | JTAG |
|---|---|
| MOSI | TDI |
| MISO | TDO |
| CLK | TCK |
| CS | TMS |
| GND | GND |

- **No pull-up is needed.**  TDI, TCK and TMS are 3.3 V push-pull outputs;
  TDO is an input.
- **Entry switches VREG on.**  It drives the `3.3V` and `5.0V` header rails.
  Wire the target's VCC to **one rail only, never both**, and disconnect the
  target's own supply first.
- **3.3 V push-pull only.**  A 1.8 V or 2.5 V target cannot use this path; a
  5 V target may not see 3.3 V as a valid high.  Check its input threshold.
- SRST and TRST are not driven by the player.  Release any leftover reset
  level before the run ([wiring.md](wiring.md#reset-lines-and-the-unused-pins)).

## 4. Probe before playing

A scan answers "is there a chain, and what does it report?" before any
configuration.  It is optional and does not configure the target.

```
$ bp xsvf -p COM5 --scan --leave
Opening Bus Pirate on COM5 at 115200 bps.
Chain Scan Result: 04 93 40 60 59
Returned to the Bus Pirate terminal.
Thank you for playing! :-)
```

- The first byte is a count: four times the number of devices.  `04` is one
  device; `08` is two.
- The bytes after the count are IDCODE bytes, least-significant byte first,
  nearest-TDO device first.  The example is the IDCODE `0x59604093`.
- `00` with no payload means TDO stayed high: no power, TDO not connected, or
  an empty chain.  `FC` followed by 252 zero bytes means TDO stayed low and
  the probe reached its 63-device clamp.  Both are valid replies and exit
  `0`, because they describe the wiring, not a tool failure.
- A reset is not required before a scan; the probe resets the chain itself.
  Add `-r` if you also want an explicit reset first.
- The example uses `--leave` so the next command can enter the board.  Without
  it the board stays in the player; press reset before the next command.

To reset only, without a scan or file:

```
$ bp xsvf -p COM5 -r --leave
Opening Bus Pirate on COM5 at 115200 bps.
Performing Reset.. Done
Returned to the Bus Pirate terminal.
Thank you for playing! :-)
```

## 5. Play the file

```
$ bp xsvf -p COM5 -f design.xsvf -r --leave
File is 53289 bytes.
Opening Bus Pirate on COM5 at 115200 bps.
Performing Reset.. Done
Sent 53289 bytes in 14 chunk(s).
End of operation reply: XSVF_ERROR_NONE
Success!
Returned to the Bus Pirate terminal.
Thank you for playing! :-)
```

- `-r` is optional.  The stream starts from a reset TAP anyway; `-r` adds an
  explicit reset and leaves the chain in Run-Test/Idle before the file.
- The file goes out in chunks of at most 4096 bytes.  The count in
  `Sent ... chunk(s)` is the number of chunks; 53289 bytes is 13 full chunks
  plus a 41-byte last chunk.
- The success check is `End of operation reply: XSVF_ERROR_NONE` followed by
  `Success!`.  `Thank you for playing! :-)` alone only means the host stopped.
- `--leave` is opt-in: without it the board stays in the player and the rails
  stay on.

**The host finishing is not the firmware player finishing.**  A long
`XRUNTEST` or `XWAIT` can outlast the host's wait.  If the command times out,
the player may still complete the run, so the target may already be
configured.  Wait, check the target, then reset before replaying.

A failed run looks like this (exit `1`):

```
$ bp xsvf -p COM5 -f design.xsvf -r
File is 53289 bytes.
Opening Bus Pirate on COM5 at 115200 bps.
Error: End of operation reply: XSVF_ERROR_TDOMISMATCH (0x02); the captured TDO did not match the expected TDO
```

The result lines are printed only after every requested operation succeeds,
so a failure prints no `Performing Reset..`, `Sent`, or `Success!` line even
if the reset was sent on the wire.  The board is back in the player loop.

## 6. Leave the player and recover the terminal

| Exit | When it works | How |
|---|---|---|
| `--leave` | the same successful reset/scan/file invocation | add it to the run |
| reset button | always | press reset; the terminal returns and VREG goes off |
| `0x0F` twice | player command loop only | a raw-serial host writes `0x0F`, waits, writes `0x0F` |
| `0x0F` during a run | never | consumed as data or as a busy byte |

`--leave` sends the pair only after every requested operation succeeds.  A
player result error (exit `1`) or an earlier host failure (exit `2`) skips
it.  A failed leave exits `2` with `Leaving XSVF player: ...`; the run itself
may have succeeded, so check the target.  **`--leave` cannot rescue a board
left in the player by an earlier command:** a new invocation cannot enter
binary mode until the board is already out.

The raw-serial fallback, as one line (needs `pyserial`, which `bp` installs;
run it with the Python from step 2):

```
python -c "import time,serial;p=serial.Serial('COM5',115200);p.write(b'\x0f');time.sleep(0.5);p.write(b'\x0f');time.sleep(0.5);p.close()"
```

Use `/dev/ttyACM0` on Linux.  The first `0x0F` resets the chain and returns
to `BBIO1` silently; the second runs the terminal reset, replies `0x01`,
switches VREG off and prints the banner.  Reopen the terminal afterwards;
press Enter if the prompt is not shown.

| After the command stopped at | What the player is doing | Exit |
|---|---|---|
| `Error: ...` (player result) | command loop, regulator on | reset, or the raw pair |
| `Timed out waiting for the XSVF player result...` | possibly still running | wait, check the target, then reset |
| `End of file reached without a completion result` | waiting for a chunk | wait about 5 s, then the raw pair or reset |
| `Returned to the Bus Pirate terminal.` | terminal | reopen the terminal |

While the player owns the port, `W` cannot be reached.  After a reset, enter
a mode (`m`) and then `W` to power the rails from the terminal; `W` also runs
that mode's setup and is refused in `HiZ>`.

## 7. If it fails

The command prints a result name or a host message.  The mapping and actions
are in [limits-and-errors.md](limits-and-errors.md).  The common causes are a
file for a different chain, swapped TDI/TDO or TCK/TMS, a missing ground, or
the target's VCC not reaching it.

To check the regulator, press reset, enter a mode (`m`), then `W`.  If a rail
does not come up, `W` prints `VREG too low, is there a short?` and
`POWER SUPPLIES OFF`; clear the short or excess load, then `W` again.  From
inside the player the only symptom is a target that does not answer.  The
player itself cannot select a rail voltage or turn VREG off.

## Related docs

| Doc | Why |
|---|---|
| [players.md](players.md) | options, serial settings, output and exit status |
| [converter.md](converter.md) | producing and checking the XSVF file |
| [wiring.md](wiring.md) | wires, rails, levels |
| [limits-and-errors.md](limits-and-errors.md) | limits, messages, recovery |
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player and stream framing |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and framing |
