"""``bp eeprom`` commands for external I2C EEPROM images."""

from __future__ import annotations

from pathlib import Path

import click

from bp.commands import get_app_context
from bp.errors import CommandError
from bp.hextext import read_hex_file, write_hex_file
from bp.protocol.i2c import EepromSessionFactory, EepromWriteReport, open_eeprom_session
from bp.types import AddressWidth, ChipAddress, ConnectionSettings, EepromLayout


def read_eeprom_to_file(
    connection: ConnectionSettings,
    layout: EepromLayout,
    length: int,
    path: Path,
    *,
    session_factory: EepromSessionFactory,
) -> bytes:
    """Read an EEPROM span and write it to a text-hex file."""
    try:
        layout.ensure_span(length)
    except ValueError as exc:
        raise CommandError(str(exc)) from exc
    with session_factory(connection) as session:
        data = session.read_eeprom(layout, length)
    write_hex_file(path, data)
    return data


def write_eeprom_from_file(
    connection: ConnectionSettings,
    layout: EepromLayout,
    path: Path,
    *,
    session_factory: EepromSessionFactory,
) -> EepromWriteReport:
    """Parse a text-hex file and write it to an EEPROM without page writes."""
    data = read_hex_file(path)
    try:
        layout.ensure_span(len(data))
    except ValueError as exc:
        raise CommandError(str(exc)) from exc
    with session_factory(connection) as session:
        return session.write_eeprom(layout, data)


def verify_eeprom_from_file(
    connection: ConnectionSettings,
    layout: EepromLayout,
    path: Path,
    *,
    session_factory: EepromSessionFactory,
) -> int:
    """Verify an EEPROM span against a text-hex file and return its length."""
    data = read_hex_file(path)
    try:
        layout.ensure_span(len(data))
    except ValueError as exc:
        raise CommandError(str(exc)) from exc
    with session_factory(connection) as session:
        session.verify_eeprom(layout, data)
    return len(data)


def _layout(chip: int, address_width: str, memory_address: int) -> EepromLayout:
    try:
        return EepromLayout(
            chip=ChipAddress(chip),
            address_width=AddressWidth(int(address_width)),
            memory_address=memory_address,
        )
    except ValueError as exc:
        raise click.BadParameter(str(exc)) from exc


@click.group("eeprom")
def eeprom_group() -> None:
    """Read, write, or verify an external I2C EEPROM."""


@eeprom_group.command("read")
@click.option("--chip", type=click.IntRange(0, 7), required=True, help="EEPROM chip-select address (0-7).")
@click.option(
    "--address-width",
    type=click.Choice(("1", "2")),
    default="2",
    show_default=True,
    help="Number of memory-address bytes: 1 or 2.",
)
@click.option(
    "--memory-address",
    type=click.IntRange(0, 0xFFFF),
    default=0,
    show_default=True,
    help="Starting EEPROM memory address.",
)
@click.option(
    "--length",
    type=click.IntRange(1, 0x10000),
    required=True,
    help="Number of bytes to read.",
)
@click.option(
    "--file",
    "file_path",
    type=click.Path(dir_okay=False, writable=True, path_type=Path),
    required=True,
    help="Text-hex output file.",
)
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def read_command(
    ctx: click.Context,
    chip: int,
    address_width: str,
    memory_address: int,
    length: int,
    file_path: Path,
    port: str | None,
    speed: int | None,
) -> None:
    """Read an EEPROM span into a text-hex file."""
    layout = _layout(chip, address_width, memory_address)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    data = read_eeprom_to_file(
        connection,
        layout,
        length,
        file_path,
        session_factory=open_eeprom_session,
    )
    click.echo(f"Read {len(data)} bytes from 0x{layout.memory_address:04X} to {file_path}.")


@eeprom_group.command("write")
@click.option("--chip", type=click.IntRange(0, 7), required=True, help="EEPROM chip-select address (0-7).")
@click.option(
    "--address-width",
    type=click.Choice(("1", "2")),
    default="2",
    show_default=True,
    help="Number of memory-address bytes: 1 or 2.",
)
@click.option(
    "--memory-address",
    type=click.IntRange(0, 0xFFFF),
    default=0,
    show_default=True,
    help="Starting EEPROM memory address.",
)
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    required=True,
    help="Text-hex input file.",
)
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def write_command(
    ctx: click.Context,
    chip: int,
    address_width: str,
    memory_address: int,
    file_path: Path,
    port: str | None,
    speed: int | None,
) -> None:
    """Write a text-hex file to an EEPROM one byte at a time."""
    layout = _layout(chip, address_width, memory_address)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    report = write_eeprom_from_file(
        connection,
        layout,
        file_path,
        session_factory=open_eeprom_session,
    )
    written = report.bytes_written
    retries = report.retries
    click.echo(f"Wrote {written} bytes at 0x{layout.memory_address:04X} with {retries} write retries.")


@eeprom_group.command("verify")
@click.option("--chip", type=click.IntRange(0, 7), required=True, help="EEPROM chip-select address (0-7).")
@click.option(
    "--address-width",
    type=click.Choice(("1", "2")),
    default="2",
    show_default=True,
    help="Number of memory-address bytes: 1 or 2.",
)
@click.option(
    "--memory-address",
    type=click.IntRange(0, 0xFFFF),
    default=0,
    show_default=True,
    help="Starting EEPROM memory address.",
)
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    required=True,
    help="Text-hex expected-content file.",
)
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def verify_command(
    ctx: click.Context,
    chip: int,
    address_width: str,
    memory_address: int,
    file_path: Path,
    port: str | None,
    speed: int | None,
) -> None:
    """Compare an EEPROM span with a text-hex file."""
    layout = _layout(chip, address_width, memory_address)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    length = verify_eeprom_from_file(
        connection,
        layout,
        file_path,
        session_factory=open_eeprom_session,
    )
    click.echo(f"Verified {length} bytes at 0x{layout.memory_address:04X} against {file_path}.")
