"""Shared command-layer input and output helpers."""

from __future__ import annotations

from pathlib import Path

from bp.errors import CommandError
from bp.hextext import format_hex_text, parse_hex_text, read_hex_file


def resolve_hex_data(
    data: str | None,
    file_path: Path | None,
    *,
    label: str = "data",
) -> bytes:
    """Return hex bytes from an inline argument or a text-hex file.

    Exactly one source must be supplied.

    Raises:
        CommandError: If both or neither source is supplied, or the selected
            source is invalid.
    """
    if data is not None and file_path is not None:
        raise CommandError(f"Supply either --{label} or --file, not both")
    if data is not None:
        return parse_hex_text(data)
    if file_path is None:
        raise CommandError(f"Supply --{label} or --file")
    return read_hex_file(file_path)


def read_binary_file(path: Path, *, label: str) -> bytes:
    """Read a binary input file, wrapping filesystem errors.

    Raises:
        CommandError: If the file cannot be read or is empty.
    """
    try:
        data = path.read_bytes()
    except OSError as exc:
        raise CommandError(f"Could not read {label} file {path}: {exc}") from exc
    if not data:
        raise CommandError(f"{label} file {path} is empty")
    return data


def write_binary_file(path: Path, data: bytes, *, label: str) -> None:
    """Write a binary output file, wrapping filesystem errors.

    Raises:
        CommandError: If the file cannot be written.
    """
    try:
        path.write_bytes(data)
    except OSError as exc:
        raise CommandError(f"Could not write {label} file {path}: {exc}") from exc


def hex_preview(data: bytes, *, limit: int = 32) -> str:
    """Return an uppercase hex preview of at most ``limit`` bytes."""
    if not data:
        return ""
    preview = format_hex_text(data[:limit]).strip()
    if len(data) > limit:
        preview = f"{preview} ..."
    return preview
