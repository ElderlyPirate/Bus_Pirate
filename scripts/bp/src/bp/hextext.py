"""Whitespace-separated ASCII-hex file parsing and formatting."""

from __future__ import annotations

import re
from pathlib import Path

from bp.errors import CommandError

HEX_TOKEN = re.compile(r"^(?:0[xX])?([0-9A-Fa-f]{1,2})$")


def parse_hex_text(text: str) -> bytes:
    """Parse whitespace-separated text hex bytes such as ``0xFF AA``.

    Raises:
        CommandError: If a token is not one or two hex digits with an optional
            ``0x``/``0X`` prefix, or the input contains no bytes.
    """
    values = bytearray()
    for index, token in enumerate(text.split(), start=1):
        match = HEX_TOKEN.fullmatch(token)
        if match is None:
            raise CommandError(f"Invalid hex token {token!r} at position {index}")
        values.append(int(match.group(1), 16))
    if not values:
        raise CommandError("Hex file contains no bytes")
    return bytes(values)


def format_hex_text(data: bytes) -> str:
    """Format bytes as uppercase, space-separated two-digit hex."""
    return " ".join(f"{value:02X}" for value in data) + "\n"


def read_hex_file(path: Path) -> bytes:
    """Read and parse a UTF-8 text-hex file, wrapping filesystem and decoding errors.

    Raises:
        CommandError: If the file cannot be read or decoded, or its tokens are
            invalid.
    """
    try:
        text = path.read_text(encoding="utf-8-sig")
    except (OSError, UnicodeError) as exc:
        raise CommandError(f"Could not read hex file {path}: {exc}") from exc
    return parse_hex_text(text)


def write_hex_file(path: Path, data: bytes) -> None:
    """Write bytes as text hex, wrapping filesystem errors.

    Raises:
        CommandError: If the file cannot be written.
    """
    try:
        path.write_text(format_hex_text(data), encoding="ascii", newline="\n")
    except OSError as exc:
        raise CommandError(f"Could not write hex file {path}: {exc}") from exc
