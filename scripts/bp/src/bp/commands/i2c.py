# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""``bp i2c`` command group for binary I2C control and register helpers."""

from __future__ import annotations

from collections.abc import Callable
from contextlib import AbstractContextManager
from pathlib import Path

import click

from bp.commands import get_app_context
from bp.commands.inputs import resolve_hex_data
from bp.hextext import format_hex_text, write_hex_file
from bp.protocol.i2c import (
    I2C_MAX_ADDRESS,
    I2cControlSession,
    I2cScanHit,
    I2cSpeed,
    open_i2c_session,
)
from bp.types import ConnectionSettings

_SPEED_MAP = {
    "5k": I2cSpeed.KHZ_5,
    "50k": I2cSpeed.KHZ_50,
    "100k": I2cSpeed.KHZ_100,
    "400k": I2cSpeed.KHZ_400,
}

I2cSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[I2cControlSession]]


def scan_i2c(
    connection: ConnectionSettings,
    *,
    speed: I2cSpeed,
    start: int,
    end: int,
    session_factory: I2cSessionFactory,
) -> tuple[I2cScanHit, ...]:
    """Probe an address range and return every acknowledgement."""
    with session_factory(connection) as session:
        session.configure_speed(speed)
        return session.scan(start=start, end=end)


def get_i2c_byte(
    connection: ConnectionSettings,
    *,
    speed: I2cSpeed,
    address: int,
    register: int,
    session_factory: I2cSessionFactory,
) -> int:
    """Read one register byte from an I2C device."""
    with session_factory(connection) as session:
        session.configure_speed(speed)
        return session.device(address).read_byte(register)


def set_i2c_byte(
    connection: ConnectionSettings,
    *,
    speed: I2cSpeed,
    address: int,
    register: int,
    value: int,
    session_factory: I2cSessionFactory,
) -> None:
    """Write one register byte to an I2C device."""
    with session_factory(connection) as session:
        session.configure_speed(speed)
        session.device(address).write_byte(register, value)


def get_i2c_word(
    connection: ConnectionSettings,
    *,
    speed: I2cSpeed,
    address: int,
    register: int,
    session_factory: I2cSessionFactory,
) -> int:
    """Read a big-endian 16-bit register value from an I2C device."""
    with session_factory(connection) as session:
        session.configure_speed(speed)
        return session.device(address).read_word(register)


def set_i2c_word(
    connection: ConnectionSettings,
    *,
    speed: I2cSpeed,
    address: int,
    register: int,
    value: int,
    session_factory: I2cSessionFactory,
) -> None:
    """Write a big-endian 16-bit register value to an I2C device."""
    with session_factory(connection) as session:
        session.configure_speed(speed)
        session.device(address).write_word(register, value)


def send_i2c_command(
    connection: ConnectionSettings,
    *,
    speed: I2cSpeed,
    address: int,
    command: int,
    session_factory: I2cSessionFactory,
) -> None:
    """Send a one-byte command to an I2C device."""
    with session_factory(connection) as session:
        session.configure_speed(speed)
        session.device(address).command(command)


def read_i2c_block(
    connection: ConnectionSettings,
    *,
    speed: I2cSpeed,
    address: int,
    register: int | None,
    length: int,
    session_factory: I2cSessionFactory,
) -> bytes:
    """Read sequential bytes from an I2C device."""
    with session_factory(connection) as session:
        session.configure_speed(speed)
        return session.device(address).read_block(register, length)


def write_i2c_block(
    connection: ConnectionSettings,
    *,
    speed: I2cSpeed,
    address: int,
    register: int | None,
    data: bytes,
    session_factory: I2cSessionFactory,
) -> None:
    """Write sequential bytes to an I2C device."""
    with session_factory(connection) as session:
        session.configure_speed(speed)
        session.device(address).write_block(register, data)


def _format_hit(hit: I2cScanHit) -> str:
    parts = [f"0x{hit.address:02X} (write 0x{hit.write_address:02X}, read 0x{hit.read_address:02X})"]
    if hit.write_ack:
        parts.append("W")
    if hit.read_ack:
        parts.append("R")
    return " ".join(parts)


@click.group("i2c")
def i2c_group() -> None:
    """Binary I2C scan, register, command, block-read, and block-write operations."""


@i2c_group.command("scan")
@click.option(
    "--start",
    type=click.IntRange(0, I2C_MAX_ADDRESS),
    default=0,
    show_default=True,
    help="First 7-bit address.",
)
@click.option(
    "--end",
    type=click.IntRange(0, I2C_MAX_ADDRESS),
    default=I2C_MAX_ADDRESS,
    show_default=True,
    help="Last 7-bit address.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="100k",
    show_default=True,
    help="I2C bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def scan_command(
    ctx: click.Context,
    start: int,
    end: int,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Probe 7-bit I2C addresses and print every acknowledgement."""
    if start > end:
        raise click.UsageError("--start must not be greater than --end")
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    factory = _factory(power=power, pullups=pullups)
    hits = scan_i2c(connection, speed=_SPEED_MAP[bus_speed], start=start, end=end, session_factory=factory)
    if not hits:
        click.echo(f"No I2C devices acknowledged addresses 0x{start:02X} through 0x{end:02X}.")
        return
    for hit in hits:
        click.echo(_format_hit(hit))


@i2c_group.command("get-byte")
@click.option(
    "--address",
    type=click.IntRange(0, I2C_MAX_ADDRESS),
    required=True,
    help="7-bit I2C device address (0-127).",
)
@click.option(
    "--register",
    type=click.IntRange(0, 0xFF),
    required=True,
    help="One-byte register address.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="100k",
    show_default=True,
    help="I2C bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def get_byte_command(
    ctx: click.Context,
    address: int,
    register: int,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read one I2C register byte."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    value = get_i2c_byte(
        connection,
        speed=_SPEED_MAP[bus_speed],
        address=address,
        register=register,
        session_factory=_factory(power=power, pullups=pullups),
    )
    click.echo(f"0x{value:02X} ({value})")


@i2c_group.command("set-byte")
@click.option(
    "--address",
    type=click.IntRange(0, I2C_MAX_ADDRESS),
    required=True,
    help="7-bit I2C device address (0-127).",
)
@click.option(
    "--register",
    type=click.IntRange(0, 0xFF),
    required=True,
    help="One-byte register address.",
)
@click.option("--value", type=click.IntRange(0, 0xFF), required=True, help="Byte value.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="100k",
    show_default=True,
    help="I2C bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def set_byte_command(
    ctx: click.Context,
    address: int,
    register: int,
    value: int,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Write one I2C register byte."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    set_i2c_byte(
        connection,
        speed=_SPEED_MAP[bus_speed],
        address=address,
        register=register,
        value=value,
        session_factory=_factory(power=power, pullups=pullups),
    )
    click.echo(f"Wrote 0x{value:02X} to register 0x{register:02X} at address 0x{address:02X}.")


@i2c_group.command("get-word")
@click.option(
    "--address",
    type=click.IntRange(0, I2C_MAX_ADDRESS),
    required=True,
    help="7-bit I2C device address (0-127).",
)
@click.option(
    "--register",
    type=click.IntRange(0, 0xFF),
    required=True,
    help="One-byte register address.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="100k",
    show_default=True,
    help="I2C bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def get_word_command(
    ctx: click.Context,
    address: int,
    register: int,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read a big-endian 16-bit I2C register value."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    value = get_i2c_word(
        connection,
        speed=_SPEED_MAP[bus_speed],
        address=address,
        register=register,
        session_factory=_factory(power=power, pullups=pullups),
    )
    click.echo(f"0x{value:04X} ({value})")


@i2c_group.command("set-word")
@click.option(
    "--address",
    type=click.IntRange(0, I2C_MAX_ADDRESS),
    required=True,
    help="7-bit I2C device address (0-127).",
)
@click.option(
    "--register",
    type=click.IntRange(0, 0xFF),
    required=True,
    help="One-byte register address.",
)
@click.option("--value", type=click.IntRange(0, 0xFFFF), required=True, help="16-bit value.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="100k",
    show_default=True,
    help="I2C bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def set_word_command(
    ctx: click.Context,
    address: int,
    register: int,
    value: int,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Write a big-endian 16-bit I2C register value."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    set_i2c_word(
        connection,
        speed=_SPEED_MAP[bus_speed],
        address=address,
        register=register,
        value=value,
        session_factory=_factory(power=power, pullups=pullups),
    )
    click.echo(f"Wrote 0x{value:04X} to register 0x{register:02X} at address 0x{address:02X}.")


@i2c_group.command("command")
@click.option(
    "--address",
    type=click.IntRange(0, I2C_MAX_ADDRESS),
    required=True,
    help="7-bit I2C device address (0-127).",
)
@click.option("--command", "command_value", type=click.IntRange(0, 0xFF), required=True, help="Command byte.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="100k",
    show_default=True,
    help="I2C bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def command_command(
    ctx: click.Context,
    address: int,
    command_value: int,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Send a one-byte I2C command."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    send_i2c_command(
        connection,
        speed=_SPEED_MAP[bus_speed],
        address=address,
        command=command_value,
        session_factory=_factory(power=power, pullups=pullups),
    )
    click.echo(f"Sent command 0x{command_value:02X} to address 0x{address:02X}.")


@i2c_group.command("read")
@click.option(
    "--address",
    type=click.IntRange(0, I2C_MAX_ADDRESS),
    required=True,
    help="7-bit I2C device address (0-127).",
)
@click.option("--register", type=click.IntRange(0, 0xFF), default=None, help="Optional one-byte register address.")
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
    default="100k",
    show_default=True,
    help="I2C bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def read_command(
    ctx: click.Context,
    address: int,
    register: int | None,
    length: int,
    file_path: Path | None,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read sequential bytes from an I2C device."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    data = read_i2c_block(
        connection,
        speed=_SPEED_MAP[bus_speed],
        address=address,
        register=register,
        length=length,
        session_factory=_factory(power=power, pullups=pullups),
    )
    if file_path is not None:
        write_hex_file(file_path, data)
        click.echo(f"Read {len(data)} bytes from address 0x{address:02X} to {file_path}.")
    else:
        click.echo(format_hex_text(data).strip())


@i2c_group.command("write")
@click.option(
    "--address",
    type=click.IntRange(0, I2C_MAX_ADDRESS),
    required=True,
    help="7-bit I2C device address (0-127).",
)
@click.option("--register", type=click.IntRange(0, 0xFF), default=None, help="Optional one-byte register address.")
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
    default="100k",
    show_default=True,
    help="I2C bus speed.",
)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=True, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def write_command(
    ctx: click.Context,
    address: int,
    register: int | None,
    data: str | None,
    file_path: Path | None,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Write sequential bytes to an I2C device."""
    payload = resolve_hex_data(data, file_path, label="data")
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    write_i2c_block(
        connection,
        speed=_SPEED_MAP[bus_speed],
        address=address,
        register=register,
        data=payload,
        session_factory=_factory(power=power, pullups=pullups),
    )
    click.echo(f"Wrote {len(payload)} bytes to address 0x{address:02X}: {format_hex_text(payload).strip()}")


def _factory(power: bool, pullups: bool) -> I2cSessionFactory:
    def factory(connection: ConnectionSettings) -> AbstractContextManager[I2cControlSession]:
        return open_i2c_session(connection, power=power, pullups=pullups)

    return factory
