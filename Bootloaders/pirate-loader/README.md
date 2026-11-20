# pirate-loader

Shared source-built host tool for the v3 ds30 and v4 CDC loaders. Supports the
PIC24FJ64GA002 (ID 212), PIC24FJ256GB106 (241), and GA004 v3 clones (213 or
Sandbox ID `02` / success byte `13`). Unsupported alternate-device firmware
IDs are rejected rather than guessed to be v4.

```sh
cmake -S . -B build
cmake --build build --config Release
build/pirate-loader --dev=/dev/ttyACM0 --hello
build/pirate-loader --dev=/dev/ttyACM0 --hex=application.hex
```

Windows uses `COMx`; Visual Studio places the executable in `build/Release/`.
Use `PIRATE_LOADER_STATIC=OFF` to disable static runtime linking. GCC/Clang
and MSVC receive their own linker/runtime flags; builds enable warnings as
errors. The old `test.sh`/`test.bat` are **live hello probes**, not offline
regression tests. A v4 hello probe also requests restart, as historically.

The tool validates HEX digits, record checksums and shapes, alignment, full
record bounds, EOF, packed destination capacity and the detected device's
image extent before the first erase. Table/program addresses differ from HEX
addresses by a factor of two. v3's original reset GOTO is copied to `0xA7FC`
and the physical reset points to `0xA800`. Configuration/loader writes rejected
by the device are reported as skipped, not verified writes. Success requires
the device's row readback; there is no separate USB read command.

Portability fixes include the real 115200 Windows baud rate, pointer-sized
handles, POSIX CS8 ordering, bounded Windows serial writes, complete command
writes, and explicit wire-byte conversions. Native tests are under
`../tests/`; they never open a serial device. `--simulate` uses the v4-size
default and does not identify a connected board or qualify a hardware flash.
