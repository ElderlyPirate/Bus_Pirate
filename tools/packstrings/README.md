# Packed firmware messages

The three `bus_pirate_*_strings.txt` files are authoritative. Common messages
are included by each board catalog. The flag is `0` for buffer output or `1`
for one appended CRLF; payloads must be non-NUL byte strings. Each assembly
message has its own `.text.<identifier>` section for linker garbage collection.

Regenerate both boards from the repository root:

```sh
python tools/packstrings/packstrings.py tools/packstrings/bus_pirate_v3_strings.txt Firmware/core_messages/messages_v3 v3
python tools/packstrings/packstrings.py tools/packstrings/bus_pirate_v4_strings.txt Firmware/core_messages/messages_v4 v4
python Firmware/tests/test_message_catalog.py
```

Includes resolve relative to their source file, not the current directory.
Output is UTF-8 without BOM, with LF line endings. The gate compares normalized
text so a Git working tree's CRLF conversion does not create a false mismatch;
all actual payload bytes, flags and identifiers are checked independently.

The firmware build consumes committed assembly; it does not silently regenerate
it. Commit the inputs and the regenerated `.h`/`.s` artifacts together. The
regression gate checks that both boards reproduce exactly, including ordering
and control-byte spelling. Malformed rows/flags, unknown escapes, invalid or
duplicate identifiers, embedded NUL and non-byte payloads are errors before
output is written.

`.pasciz` is the PIC24 three-bytes-per-program-word representation, not a
Pascal length-prefixed string. The assembler appends NUL to each quoted
operand (DS50002106, 5.3.5), so the generator emits exactly one quoted literal
per message; fixed-width octal escapes preserve arbitrary non-NUL bytes.
Firmware accessors
take even table addresses and restore TBLPAG; their public bounds contract and
instruction proof are in `Firmware/core_messages/README.md`.
