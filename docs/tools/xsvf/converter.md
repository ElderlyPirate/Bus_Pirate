# SVF → XSVF conversion

Part of the XSVF host-tool set (`docs/tools/xsvf/`).  This page owns the
translation of an SVF file to the XSVF file the firmware player reads, the
`mergexsvf` combiner, and the shift-size check.  The host players are in
[players.md](players.md); the firmware side is in
[protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md).

## Scope

Covered: when to convert, `svf2xsvf502` syntax and options, the per-family
recipes from the bundled ReadMe, `mergexsvf`, the bundled batch file, and how
to check the largest shift against the firmware player's 400-bit buffer.  Not
covered: generating an SVF (your toolchain's job) or streaming the result
([players.md](players.md)).

## Direct XSVF or SVF?

The bundled ReadMe recommends using the latest ISE iMPACT to generate an XSVF
file directly for the target device in the chain.  Use the converter when you
have only an SVF:

- iMPACT → XSVF: preferred when available.
- SVF → `svf2xsvf502` → XSVF: the bundled translator.
- several devices → one XSVF per device → `mergexsvf` → one chain file.

The firmware player supports the XSVF 5.01 command set.  Leave `-v2` off for
it; that option is only for a v2 player.

## Naming

The bundled ReadMe writes the translator as `svf2xsvf501` (syntax line) and
`svf2xsvf` (examples); the shipped executable is `svf2xsvf502`.  They are the
same translator; this page uses `svf2xsvf502`.  The merge tool prints
`MergeXSVF v1.00` and ships as `mergexsvf`.

## Translation

Basic syntax:

```
svf2xsvf502 [options] -i input.svf -o output.xsvf [-a output.txt]
```

| Option | Meaning |
|---|---|
| `-i file` | input SVF |
| `-o file` | output XSVF |
| `-a file` | optional text listing of the generated commands (debug/analysis) |
| `-d` | overwrite/delete pre-existing output files |
| `-w` | same as `-d` (the ReadMe gives both the same description) |
| `-r N` | maximum retry count, N times (XC9500/XL/XV only) |
| `-fpga` | special translation for FPGA bitstreams (FPGA only) |
| `-extensions` | add special TAP command extensions (CoolRunner/II only) |
| `-xwait` | use the inline `XWAIT` command (CoolRunner/II only) |
| `-minxstate` | minimize `XSTATE` commands |
| `-v2` | backward-compatible XSVF for an XSVF v2 player |
| `-xprintsrc N` | print comments into the XSVF, N = 0–3 |

Two notes from the bundled ReadMe:

- `-a` is optional.  The ReadMe's syntax line shows it without brackets, but
  the bundled `svf2XSVF.bat` runs the translator with only `-i` and `-o`.
  Add `-a` when you want the text listing (for example, to check the shift
  size).
- The FPGA recipes use `-rlen N`, but the ReadMe's option block does not list
  it.  Treat it as part of those recipes, not as a documented general option.

## Per-family recipes

| Family | Options |
|---|---|
| XC9500/XL/XV | none |
| CoolRunner/II | `-r 0 -extensions -xwait` |
| XC18V00/XCF00S | `-r 0` |
| XCF00P | `-r 0 -extensions` |
| Spartan-3AN and indirect SPI flash | `-fpga -rlen 10000` |
| Spartan-II or later, Virtex or later | `-fpga -extensions` (sets `-r 0`) |
| XC4000+/Spartan/Spartan-XL | `-fpga -rlen 1024` (sets `-r 0`) |
| System ACE MPM/SC | `-r 0 -xwait` |

The recipes are the translator's requirements.  The firmware player's
400-bit shift buffer is much smaller: only the XC9500/XL/XV class fits within
it (its largest shift is about 256 bits).  CoolRunner/II (about 1035 bits),
FPGA (about 1024 bits) and XC18V00/XCF00S (about 8192 bits) need longer
shifts and end with `XSVF_ERROR_DATAOVERFLOW`; use another host player, or
OpenOCD's own `svf` command, which plays the original SVF (not XSVF) through
its `0x06` path ([OpenOCD set](../openocd/README.md)).

## Checking the shift size

The firmware player stores the longest single DR/IR shift in a 50-byte
buffer, so the largest shift must be **400 bits or less**.  Use the `-a`
text listing to check it:

1. Convert with `-a`.
2. Find the largest `XSDRSIZE` value in the listing; check any `XSIR` or
   `XSIR2` shift length as well.
3. Divide that bit count by 8 and round up: the result must be 50 or less
   (`ceil(XSDRSIZE / 8) ≤ 50`, i.e. `XSDRSIZE ≤ 400`).
4. If it is larger, the firmware player returns `XSVF_ERROR_DATAOVERFLOW`
   (`0x06`) during the run.

`XSDRSIZE` is the DR shift length in bits; the converter already includes
any header/trailer bits in it.  Use the largest value in the file.

## Merging a chain

`mergexsvf` combines per-device XSVF files into one stream in the listed
order and inserts the reset/`XENDIR`/`XENDDR` settings between them:

```
mergexsvf -d -o chain.xsvf -i part1.xsvf -i part2.xsvf
```

| Option | Meaning |
|---|---|
| `-o file` | merged output (required) |
| `-i file` | input file; repeat for each device, in chain order |
| `-d` | overwrite the output without warning |
| `-v2` | insert commands compatible with a v2 player; only if the inputs were made with `-v2` |

The merged file is still subject to the 400-bit shift limit, so each part
must fit.

## The bundled batch file

`SVF2XSVF/svf2XSVF.bat` (Windows) is an example:

```
svf2xsvf502.exe -i Verilog-CPLDIntro3LEDinverse.svf -o Verilog-CPLDIntro3LEDinverse.xsvf
pause
```

Its input file is not shipped with the tools; edit the names.  The batch also
shows that `-a` is optional.

## Platform folders

The cross-platform tree bundles the converters per platform under
`SVF2XSVF/`: `nt/` is Windows, `lin/` is Linux, `hp/` is HP-UX and `sol/`
is Solaris.  The Windows and Linux builds are the ones most readers need.
The Windows player folder carries the Windows executables
(`svf2xsvf502.exe`, `mergexsvf.exe`) in its `SVF2XSVF/` subfolder, plus the
batch file.  The converter ReadMe is in the cross-platform tree.

## Worked commands

XC9500/XL/XV, converting and then playing (Windows):

```
> svf2xsvf502.exe -d -i design.svf -o design.xsvf -a design.txt
> BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f design.xsvf -r
```

The cross-platform converter on Linux uses the `lin/` builds:

```
$ SVF2XSVF/lin/svf2xsvf502 -d -i design.svf -o design.xsvf -a design.txt
```

For a chain of two devices (Windows):

```
> mergexsvf.exe -d -o chain.xsvf -i part1.xsvf -i part2.xsvf
> BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f chain.xsvf -r
```

On Linux, `SVF2XSVF/lin/mergexsvf` performs the same merge.

Check that the output file exists and, with `-a`, that the text listing was
written.  The listing names each command; its largest `XSDRSIZE` is the value
to compare with the 50-byte limit.

## Related docs

| Doc | Why |
|---|---|
| [players.md](players.md) | the host players and their options |
| [workflow.md](workflow.md) | the end-to-end sequence |
| [limits-and-errors.md](limits-and-errors.md) | the 400-bit limit and result messages |
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player |
| [../openocd/README.md](../openocd/README.md) | the larger-device alternative |
