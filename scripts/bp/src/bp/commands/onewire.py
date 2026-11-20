# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""``bp onewire`` command group for binary 1-Wire operations."""

from __future__ import annotations

from contextlib import AbstractContextManager
from pathlib import Path

import click

from bp.commands import get_app_context
from bp.commands.inputs import resolve_hex_data
from bp.errors import CommandError
from bp.hextext import format_hex_text, write_hex_file
from bp.protocol.onewire import (
    OneWireRom,
    OneWireSession,
    OneWireSessionFactory,
    OneWireSpeed,
    open_onewire_session,
)
from bp.types import ConnectionSettings

_SPEED_MAP = {
    "standard": OneWireSpeed.STANDARD,
    "overdrive": OneWireSpeed.OVERDRIVE,
}


def _prepare(session: OneWireSession, speed: OneWireSpeed, *, power: bool, pullups: bool) -> None:
    session.configure_speed(speed)
    session.configure_peripherals(power=power, pullups=pullups)


def parse_rom(value: str | None) -> OneWireRom | None:
    """Parse an eight-byte ROM from hex, accepting ``:``, ``-``, or space separators."""
    if value is None:
        return None
    cleaned = value.replace(":", "").replace("-", "").replace(" ", "")
    if len(cleaned) != 16:
        raise CommandError(f"1-Wire ROM must contain 16 hex digits, got {value!r}")
    try:
        data = bytes.fromhex(cleaned)
    except ValueError as exc:
        raise CommandError(f"Invalid 1-Wire ROM {value!r}: {exc}") from exc
    return OneWireRom(data)


def scan_onewire(
    connection: ConnectionSettings,
    *,
    alarm: bool,
    speed: OneWireSpeed,
    power: bool,
    pullups: bool,
    session_factory: OneWireSessionFactory,
) -> tuple[OneWireRom, ...]:
    """Run a ROM or alarm search."""
    with session_factory(connection) as session:
        _prepare(session, speed, power=power, pullups=pullups)
        return session.alarm_search() if alarm else session.search_rom()


def read_onewire_rom(
    connection: ConnectionSettings,
    *,
    speed: OneWireSpeed,
    power: bool,
    pullups: bool,
    session_factory: OneWireSessionFactory,
) -> OneWireRom:
    """Read the ROM of the single device on the bus."""
    with session_factory(connection) as session:
        _prepare(session, speed, power=power, pullups=pullups)
        return session.read_rom()


def read_onewire_scratchpad(
    connection: ConnectionSettings,
    rom: OneWireRom | None,
    *,
    length: int,
    speed: OneWireSpeed,
    power: bool,
    pullups: bool,
    session_factory: OneWireSessionFactory,
) -> bytes:
    """Read a device scratchpad."""
    with session_factory(connection) as session:
        _prepare(session, speed, power=power, pullups=pullups)
        return session.read_scratchpad(rom, length=length)


def write_onewire_scratchpad(
    connection: ConnectionSettings,
    rom: OneWireRom | None,
    data: bytes,
    *,
    speed: OneWireSpeed,
    power: bool,
    pullups: bool,
    session_factory: OneWireSessionFactory,
) -> None:
    """Write a device scratchpad."""
    with session_factory(connection) as session:
        _prepare(session, speed, power=power, pullups=pullups)
        session.write_scratchpad(rom, data)


def copy_onewire_scratchpad(
    connection: ConnectionSettings,
    rom: OneWireRom | None,
    *,
    speed: OneWireSpeed,
    power: bool,
    pullups: bool,
    session_factory: OneWireSessionFactory,
) -> None:
    """Copy a device scratchpad."""
    with session_factory(connection) as session:
        _prepare(session, speed, power=power, pullups=pullups)
        session.copy_scratchpad(rom)


def read_onewire_memory(
    connection: ConnectionSettings,
    rom: OneWireRom | None,
    *,
    address: int,
    length: int,
    speed: OneWireSpeed,
    power: bool,
    pullups: bool,
    session_factory: OneWireSessionFactory,
) -> bytes:
    """Read device memory."""
    with session_factory(connection) as session:
        _prepare(session, speed, power=power, pullups=pullups)
        return session.read_memory(rom, address=address, length=length)


def read_onewire_status(
    connection: ConnectionSettings,
    rom: OneWireRom | None,
    *,
    address: int,
    speed: OneWireSpeed,
    power: bool,
    pullups: bool,
    session_factory: OneWireSessionFactory,
) -> int:
    """Read one device status byte."""
    with session_factory(connection) as session:
        _prepare(session, speed, power=power, pullups=pullups)
        return session.read_status(rom, address=address)


def send_onewire(
    connection: ConnectionSettings,
    data: bytes,
    rom: OneWireRom | None,
    *,
    skip_rom: bool,
    speed: OneWireSpeed,
    power: bool,
    pullups: bool,
    session_factory: OneWireSessionFactory,
) -> None:
    """Reset the bus, optionally select a device, and send raw command bytes."""
    if rom is not None and skip_rom:
        raise CommandError("Supply either --rom or --skip-rom, not both")
    with session_factory(connection) as session:
        _prepare(session, speed, power=power, pullups=pullups)
        session.reset()
        if rom is not None:
            session.match_rom(rom)
        elif skip_rom:
            session.skip_rom()
        session.write_bytes(data)


def reset_onewire(
    connection: ConnectionSettings,
    *,
    speed: OneWireSpeed,
    power: bool,
    pullups: bool,
    session_factory: OneWireSessionFactory,
) -> None:
    """Reset the 1-Wire bus."""
    with session_factory(connection) as session:
        _prepare(session, speed, power=power, pullups=pullups)
        session.reset()


def _rom_text(rom: OneWireRom) -> str:
    status = "CRC OK" if rom.crc_valid else "CRC INVALID"
    return f"{rom.hex} family=0x{rom.family:02X} {status}"


def _factory() -> OneWireSessionFactory:
    def factory(connection: ConnectionSettings) -> AbstractContextManager[OneWireSession]:
        return open_onewire_session(connection)

    return factory


@click.group("onewire")
def onewire_group() -> None:
    """Binary 1-Wire reset, ROM search/read/match/skip, scratchpad, and memory."""


@onewire_group.command("reset")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def reset_command(
    ctx: click.Context,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Perform a 1-Wire bus reset."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    reset_onewire(
        connection,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo("1-Wire bus reset complete.")


@onewire_group.command("scan")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def scan_command(
    ctx: click.Context,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Enumerate all 1-Wire devices with a ROM search."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    devices = scan_onewire(
        connection,
        alarm=False,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    if not devices:
        click.echo("No 1-Wire devices found.")
        return
    for rom in devices:
        click.echo(_rom_text(rom))


@onewire_group.command("alarm-scan")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def alarm_scan_command(
    ctx: click.Context,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Enumerate 1-Wire devices that are in alarm state."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    devices = scan_onewire(
        connection,
        alarm=True,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    if not devices:
        click.echo("No 1-Wire devices are in alarm state.")
        return
    for rom in devices:
        click.echo(_rom_text(rom))


@onewire_group.command("read-rom")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def read_rom_command(
    ctx: click.Context,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read the ROM of the single device on the bus."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    rom = read_onewire_rom(
        connection,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(_rom_text(rom))


@onewire_group.command("read-scratchpad")
@click.option(
    "--rom",
    "rom_text",
    type=str,
    default=None,
    help="8-byte device ROM in hex; omit to use SKIP ROM.",
)
@click.option("--length", type=click.IntRange(1, 0x10000), default=9, show_default=True, help="Scratchpad bytes.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(dir_okay=False, writable=True, path_type=Path),
    default=None,
    help="Optional text-hex output file.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def read_scratchpad_command(
    ctx: click.Context,
    rom_text: str | None,
    length: int,
    file_path: Path | None,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read a 1-Wire device scratchpad."""
    rom = parse_rom(rom_text)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    data = read_onewire_scratchpad(
        connection,
        rom,
        length=length,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    if file_path is not None:
        write_hex_file(file_path, data)
        click.echo(f"Read {len(data)} scratchpad bytes to {file_path}.")
    else:
        click.echo(format_hex_text(data).strip())


@onewire_group.command("write-scratchpad")
@click.option(
    "--rom",
    "rom_text",
    type=str,
    default=None,
    help="8-byte device ROM in hex; omit to use SKIP ROM.",
)
@click.option("--data", type=str, default=None, help="Whitespace-separated hex bytes to write.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    default=None,
    help="Text-hex file to write.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def write_scratchpad_command(
    ctx: click.Context,
    rom_text: str | None,
    data: str | None,
    file_path: Path | None,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Write a 1-Wire device scratchpad."""
    rom = parse_rom(rom_text)
    payload = resolve_hex_data(data, file_path, label="data")
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    write_onewire_scratchpad(
        connection,
        rom,
        payload,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(f"Wrote {len(payload)} scratchpad bytes.")


@onewire_group.command("copy-scratchpad")
@click.option(
    "--rom",
    "rom_text",
    type=str,
    default=None,
    help="8-byte device ROM in hex; omit to use SKIP ROM.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def copy_scratchpad_command(
    ctx: click.Context,
    rom_text: str | None,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Copy a 1-Wire device scratchpad to memory."""
    rom = parse_rom(rom_text)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    copy_onewire_scratchpad(
        connection,
        rom,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo("Scratchpad copy command sent.")


@onewire_group.command("read-memory")
@click.option(
    "--rom",
    "rom_text",
    type=str,
    default=None,
    help="8-byte device ROM in hex; omit to use SKIP ROM.",
)
@click.option("--address", type=click.IntRange(0, 0xFFFF), default=0, show_default=True, help="Memory address.")
@click.option("--length", type=click.IntRange(1, 0x10000), required=True, help="Number of bytes to read.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(dir_okay=False, writable=True, path_type=Path),
    default=None,
    help="Optional text-hex output file.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def read_memory_command(
    ctx: click.Context,
    rom_text: str | None,
    address: int,
    length: int,
    file_path: Path | None,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read 1-Wire device memory."""
    rom = parse_rom(rom_text)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    data = read_onewire_memory(
        connection,
        rom,
        address=address,
        length=length,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    if file_path is not None:
        write_hex_file(file_path, data)
        click.echo(f"Read {len(data)} memory bytes to {file_path}.")
    else:
        click.echo(format_hex_text(data).strip())


@onewire_group.command("read-status")
@click.option(
    "--rom",
    "rom_text",
    type=str,
    default=None,
    help="8-byte device ROM in hex; omit to use SKIP ROM.",
)
@click.option("--address", type=click.IntRange(0, 0xFFFF), default=0, show_default=True, help="Status address.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def read_status_command(
    ctx: click.Context,
    rom_text: str | None,
    address: int,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read one 1-Wire device status byte."""
    rom = parse_rom(rom_text)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    status = read_onewire_status(
        connection,
        rom,
        address=address,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(f"Status: 0x{status:02X} ({status})")


@onewire_group.command("send")
@click.option(
    "--rom",
    "rom_text",
    type=str,
    default=None,
    help="8-byte device ROM in hex; omit to use SKIP ROM.",
)
@click.option("--skip-rom", is_flag=True, help="Send SKIP ROM instead of MATCH ROM.")
@click.option("--data", type=str, default=None, help="Whitespace-separated hex bytes to send.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    default=None,
    help="Text-hex file to send.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="standard",
    show_default=True,
    help="1-Wire bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def send_command(
    ctx: click.Context,
    rom_text: str | None,
    skip_rom: bool,
    data: str | None,
    file_path: Path | None,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Reset the bus, select a device, and send raw command bytes."""
    rom = parse_rom(rom_text)
    payload = resolve_hex_data(data, file_path, label="data")
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    send_onewire(
        connection,
        payload,
        rom,
        skip_rom=skip_rom,
        speed=_SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(f"Sent {len(payload)} 1-Wire byte(s): {format_hex_text(payload).strip()}")
