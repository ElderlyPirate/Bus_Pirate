# SVF to XSVF conversion

Part of the XSVF set.  This page covers how an SVF file becomes the XSVF file
the firmware player reads, which device families fit the player, and the
shift-size check.  The host command is in [players.md](players.md); the
firmware side is in [player.md](player.md).

## Scope

Covered: choosing between iMPACT and the external translator, the
translator's options and family recipes, the 400-bit gate, and merging a
chain.  Not covered: generating the SVF (your toolchain's job) or streaming
the result ([workflow.md](workflow.md)).

## The converter is external

The firmware player reads **XSVF 5.01**, not SVF.  `bp` does not parse SVF
and does not install a converter.  Two routes produce XSVF:

| Route | When to use |
|---|---|
| Xilinx iMPACT | preferred: it writes XSVF directly for the device or chain |
| `svf2xsvf502` | you already have an SVF and a copy of the Xilinx translator |

`svf2xsvf502` is Xilinx's standalone SVF-to-XSVF translator.  It is not part
of this repository, and no download is provided here.  If you do not already
have it, use iMPACT; both tools come from the discontinued ISE 14.7-era
toolchain, so either one must already be on hand.  Do not pass a `.svf` file
to `bp xsvf`.

A file for a multi-device chain can be built per device and combined with
`mergexsvf`, another external Xilinx utility ([below](#merging-a-chain)).

## Check the fit before converting

Only the **XC9500/XL/XV** class fits this player.  The player holds the
longest single DR or IR shift in a 50-byte buffer, so the largest shift must
be **400 bits or less**.  Other families translate correctly but stop with
`XSVF_ERROR_DATAOVERFLOW` during the run.

| Family | Translator options | Fits this player? |
|---|---|---|
| XC9500/XL/XV | none | **yes**; largest shift about 256 bits |
| CoolRunner/II | `-r 0 -extensions -xwait` | no; about 1035 bits |
| XC18V00/XCF00S | `-r 0` | no; about 8192 bits |
| XCF00P | `-r 0 -extensions` | no; PROM-class stream |
| Spartan-3AN and indirect SPI flash | `-fpga -rlen 10000` | no; configuration stream |
| Spartan-II or later, Virtex or later | `-fpga -extensions` | no; about 1024 bits |
| XC4000+/Spartan/Spartan-XL | `-fpga -rlen 1024` | no; about 1024 bits |
| System ACE MPM/SC | `-r 0 -xwait` | no; configuration stream |

The 256-bit figure is one device; a multi-device chain adds shift length, so
check the largest `XSDRSIZE` in the listing even for XC9500 devices.

For a family in the "no" rows, keep the original SVF and use OpenOCD's own
`svf` command through its `0x06` path, or another programmer.  OpenOCD plays
SVF, not XSVF ([OpenOCD set](../openocd/README.md)).

## Translator syntax

```
svf2xsvf502 [options] -i input.svf -o output.xsvf [-a output.txt]
```

| Option | Meaning |
|---|---|
| `-i file` | input SVF |
| `-o file` | output XSVF |
| `-a file` | optional text listing of the generated commands |
| `-d` | delete or overwrite pre-existing output files |
| `-w` | same as `-d` in the translator's documentation |
| `-r N` | maximum retry count, N times (XC9500/XL/XV only) |
| `-fpga` | special translation for FPGA bitstreams |
| `-extensions` | add special TAP command extensions (CoolRunner/II only) |
| `-xwait` | use the inline `XWAIT` command (CoolRunner/II only) |
| `-minxstate` | minimize `XSTATE` commands |
| `-v2` | backward-compatible XSVF for an XSVF v2 player |
| `-xprintsrc N` | print comments into the XSVF, N = 0 to 3 |

- **Leave `-v2` off.**  The firmware player supports XSVF 5.01; `-v2` is for
  a v2 player.
- `-r 0` disables retries.  A non-zero retry count is meaningful only for
  XC9500/XL/XV; the other recipes pass `-r 0`.
- `-a` is optional.  It is needed for the shift check below.
- `-rlen` appears in the FPGA recipes but not in the option block; use it as
  the recipe shows.  Your copy's own help is authoritative for the exact
  option set.

## Checking the shift size

`XSDRSIZE` is the DR shift length in bits, including the header and trailer
bits the translator adds.  `XSIR` and `XSIR2` carry IR shift lengths; check
those too.

1. Convert with `-a`.
2. Find the largest `XSDRSIZE` in the listing.
3. Divide by 8 and round up; the result must be 50 or less.

Examples:

| Largest value | Bytes (`ceil(bits / 8)`) | Fits 50-byte buffer? |
|---|---|---|
| `XSDRSIZE 256` | 32 | yes |
| `XSDRSIZE 1035` | 130 | no |
| `XSDRSIZE 8192` | 1024 | no |

A listing excerpt looks like this:

```
XSDRSIZE 256
```

An XSVF written directly by iMPACT has no listing.  Keep the SVF and convert
it with `-a` for the check, inspect the XSVF with a parser, or treat
`XSVF_ERROR_DATAOVERFLOW` as the check after a run.  Keeping the SVF is the
simplest way to get the number before wiring the target.

## Merging a chain

`mergexsvf` combines per-device XSVF files into one stream in the listed
order and inserts the reset and end-state settings between them:

```
mergexsvf -d -o chain.xsvf -i part1.xsvf -i part2.xsvf
```

| Option | Meaning |
|---|---|
| `-o file` | merged output, required |
| `-i file` | input file; repeat for each device, in order |
| `-d` | overwrite the output without warning |
| `-v2` | v2-compatible output; only if the inputs were made with `-v2` |

The merged file is still subject to the 400-bit shift limit, so each part
must fit.  `mergexsvf` is part of the external Xilinx tool set, not this
repository.

## Worked commands

Convert an SVF for an XC9500-class device and play it:

```
$ svf2xsvf502 -d -i design.svf -o design.xsvf -a design.txt
$ bp xsvf -p COM5 -f design.xsvf -r --leave
```

On Windows the translator is usually `svf2xsvf502.exe`; on Linux use the
Linux build.  The translator writes the XSVF and, with `-a`, the text
listing; confirm both files exist before wiring the target.

A two-device chain:

```
$ mergexsvf -d -o chain.xsvf -i part1.xsvf -i part2.xsvf
$ bp xsvf -p COM5 -f chain.xsvf -r --leave
```

## Limits and out of scope

- **400-bit single shift**, 4096-byte chunks:
  [limits-and-errors.md](limits-and-errors.md).
- **XC9500/XL/XV only.**  The other families need a different player.
- **No converter download.**  Acquisition is outside this repository.
- **No SVF playback in the firmware player.**  OpenOCD is the SVF path.

## Related docs

| Doc | Why |
|---|---|
| [players.md](players.md) | the host command and its options |
| [workflow.md](workflow.md) | the end-to-end sequence |
| [limits-and-errors.md](limits-and-errors.md) | the shift limit and result messages |
| [player.md](player.md) | the firmware player |
| [../openocd/README.md](../openocd/README.md) | the larger-device and SVF alternative |
