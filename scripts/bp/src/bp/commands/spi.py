# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""``bp spi`` command group for binary SPI control and flash-style reads."""

from __future__ import annotations

from collections.abc import Callable
from contextlib import AbstractContextManager
from pathlib import Path

import click

from bp.commands import get_app_context
from bp.commands.inputs import hex_preview, read_binary_file, resolve_hex_data, write_binary_file
from bp.errors import CommandError
from bp.hextext import format_hex_text
from bp.protocol.spi import SpiConfig, SpiSession, SpiSpeed, open_spi_session
from bp.types import ConnectionSettings

_SPEED_MAP = {
    "30k": SpiSpeed.KHZ_30,
    "50k": SpiSpeed.KHZ_50,
    "125k": SpiSpeed.KHZ_125,
    "250k": SpiSpeed.KHZ_250,
    "1m": SpiSpeed.MHZ_1,
    "1.3m": SpiSpeed.MHZ_1_3,
    "2m": SpiSpeed.MHZ_2,
    "2.6m": SpiSpeed.MHZ_2_6,
    "3.2m": SpiSpeed.MHZ_3_2,
    "4m": SpiSpeed.MHZ_4,
    "5.3m": SpiSpeed.MHZ_5_3,
    "8m": SpiSpeed.MHZ_8,
}

SpiSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[SpiSession]]


def _prepare(
    session: SpiSession,
    speed: SpiSpeed,
    config: SpiConfig,
    *,
    power: bool,
    pullups: bool,
) -> None:
    session.configure_speed(speed)
    session.configure_spi(
        output_3v3=config.output_3v3,
        clock_idle_high=config.clock_idle_high,
        active_to_idle=config.active_to_idle,
        sample_end=config.sample_end,
    )
    session.configure_peripherals(power=power, pullups=pullups)


def configure_spi(
    connection: ConnectionSettings,
    speed: SpiSpeed,
    config: SpiConfig,
    *,
    power: bool,
    pullups: bool,
    session_factory: SpiSessionFactory,
) -> None:
    """Enter SPI mode and apply speed, mode, and peripheral settings."""
    with session_factory(connection) as session:
        _prepare(session, speed, config, power=power, pullups=pullups)


def read_spi_chip_id(
    connection: ConnectionSettings,
    speed: SpiSpeed,
    config: SpiConfig,
    *,
    power: bool,
    pullups: bool,
    session_factory: SpiSessionFactory,
) -> bytes:
    """Read a JEDEC chip ID with ``0x9F`` and three dummy bytes."""
    with session_factory(connection) as session:
        _prepare(session, speed, config, power=power, pullups=pullups)
        session.set_cs(low=True)
        response = session.bulk_transfer(bytes([0x9F, 0xFF, 0xFF, 0xFF]))
        session.set_cs(low=False)
        return response


def transfer_spi(
    connection: ConnectionSettings,
    data: bytes,
    speed: SpiSpeed,
    config: SpiConfig,
    *,
    power: bool,
    pullups: bool,
    session_factory: SpiSessionFactory,
) -> bytes:
    """Clock bytes out with CS asserted and return the received bytes."""
    with session_factory(connection) as session:
        _prepare(session, speed, config, power=power, pullups=pullups)
        session.set_cs(low=True)
        response = session.write_bytes(data)
        session.set_cs(low=False)
        return response


def read_spi(
    connection: ConnectionSettings,
    *,
    opcode: int,
    address: int,
    address_width: int,
    dummy_bytes: int,
    length: int,
    speed: SpiSpeed,
    config: SpiConfig,
    power: bool,
    pullups: bool,
    session_factory: SpiSessionFactory,
) -> bytes:
    """Send an opcode/address/dummy prefix and read a byte block."""
    if address_width < 0 or address_width > 4:
        raise ValueError(f"SPI address width must be 0 through 4, got {address_width}")
    if address < 0 or address >= 1 << (address_width * 8):
        raise ValueError(f"SPI address 0x{address:X} does not fit in {address_width} address byte(s)")
    if dummy_bytes < 0:
        raise ValueError(f"SPI dummy byte count must not be negative, got {dummy_bytes}")
    if length < 1:
        raise ValueError("SPI read length must be positive")
    prefix = bytes([opcode]) + (address.to_bytes(address_width, "big") if address_width else b"") + b"\x00" * dummy_bytes
    with session_factory(connection) as session:
        _prepare(session, speed, config, power=power, pullups=pullups)
        session.set_cs(low=True)
        if prefix:
            session.write_bytes(prefix)
        data = session.read_bytes(length)
        session.set_cs(low=False)
        return data


def write_spi(
    connection: ConnectionSettings,
    *,
    opcode: int | None,
    address: int,
    address_width: int,
    data: bytes,
    speed: SpiSpeed,
    config: SpiConfig,
    power: bool,
    pullups: bool,
    session_factory: SpiSessionFactory,
) -> None:
    """Send an optional opcode/address prefix followed by data bytes."""
    if address_width < 0 or address_width > 4:
        raise ValueError(f"SPI address width must be 0 through 4, got {address_width}")
    if address < 0 or address >= 1 << (address_width * 8):
        raise ValueError(f"SPI address 0x{address:X} does not fit in {address_width} address byte(s)")
    prefix = b""
    if opcode is not None:
        prefix = bytes([opcode]) + (address.to_bytes(address_width, "big") if address_width else b"")
    with session_factory(connection) as session:
        _prepare(session, speed, config, power=power, pullups=pullups)
        session.set_cs(low=True)
        if prefix:
            session.write_bytes(prefix)
        session.write_bytes(data)
        session.set_cs(low=False)


@click.group("spi")
def spi_group() -> None:
    """Binary SPI configuration, chip-ID, transfer, and flash-style read/write."""


def _config(
    clock_polarity: str,
    clock_edge: str,
    sample: str,
    output: str,
) -> SpiConfig:
    return SpiConfig(
        output_3v3=output == "normal",
        clock_idle_high=clock_polarity == "idle-high",
        active_to_idle=clock_edge == "active-to-idle",
        sample_end=sample == "end",
    )


def _factory() -> SpiSessionFactory:
    def factory(connection: ConnectionSettings) -> AbstractContextManager[SpiSession]:
        return open_spi_session(connection)

    return factory


def _require_data_options(data: str | None, file_path: Path | None) -> bytes:
    return resolve_hex_data(data, file_path, label="data")


@spi_group.command("config")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="1m",
    show_default=True,
    help="SPI clock speed.",
)
@click.option("--clock-polarity", type=click.Choice(("idle-low", "idle-high")), default="idle-low", show_default=True)
@click.option(
    "--clock-edge",
    type=click.Choice(("idle-to-active", "active-to-idle")),
    default="active-to-idle",
    show_default=True,
)
@click.option("--sample", type=click.Choice(("middle", "end")), default="middle", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def config_command(
    ctx: click.Context,
    bus_speed: str,
    clock_polarity: str,
    clock_edge: str,
    sample: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Enter SPI mode, apply the configuration, and return to bitbang."""
    config = _config(clock_polarity, clock_edge, sample, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    configure_spi(
        connection,
        _SPEED_MAP[bus_speed],
        config,
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(f"SPI configured: {bus_speed}, {output}, clock {clock_polarity}, edge {clock_edge}, sample {sample}.")


@spi_group.command("chip-id")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="1m",
    show_default=True,
    help="SPI clock speed.",
)
@click.option("--clock-polarity", type=click.Choice(("idle-low", "idle-high")), default="idle-low", show_default=True)
@click.option(
    "--clock-edge",
    type=click.Choice(("idle-to-active", "active-to-idle")),
    default="active-to-idle",
    show_default=True,
)
@click.option("--sample", type=click.Choice(("middle", "end")), default="middle", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def chip_id_command(
    ctx: click.Context,
    bus_speed: str,
    clock_polarity: str,
    clock_edge: str,
    sample: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read a JEDEC chip ID with opcode 0x9F."""
    config = _config(clock_polarity, clock_edge, sample, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    response = read_spi_chip_id(
        connection,
        _SPEED_MAP[bus_speed],
        config,
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(f"Chip ID response: {format_hex_text(response).strip()}")
    if len(response) >= 4:
        click.echo(f"JEDEC ID: {response[1]:02X} {response[2]:02X} {response[3]:02X}")


@spi_group.command("transfer")
@click.option("--data", type=str, default=None, help="Whitespace-separated hex bytes to transfer.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    default=None,
    help="Text-hex file to transfer.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="1m",
    show_default=True,
    help="SPI clock speed.",
)
@click.option("--clock-polarity", type=click.Choice(("idle-low", "idle-high")), default="idle-low", show_default=True)
@click.option(
    "--clock-edge",
    type=click.Choice(("idle-to-active", "active-to-idle")),
    default="active-to-idle",
    show_default=True,
)
@click.option("--sample", type=click.Choice(("middle", "end")), default="middle", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def transfer_command(
    ctx: click.Context,
    data: str | None,
    file_path: Path | None,
    bus_speed: str,
    clock_polarity: str,
    clock_edge: str,
    sample: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Clock bytes out with CS asserted and print the received bytes."""
    payload = _require_data_options(data, file_path)
    config = _config(clock_polarity, clock_edge, sample, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    response = transfer_spi(
        connection,
        payload,
        _SPEED_MAP[bus_speed],
        config,
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(f"Transferred {len(payload)} byte(s).")
    click.echo(f"Received: {format_hex_text(response).strip()}")


@spi_group.command("read")
@click.option("--opcode", type=click.IntRange(0, 0xFF), required=True, help="Read opcode, for example 0x0B or 0x03.")
@click.option("--address", type=click.IntRange(0, 0xFFFFFFFF), default=0, show_default=True, help="Starting address.")
@click.option("--address-width", type=click.IntRange(0, 4), default=3, show_default=True, help="Address bytes.")
@click.option("--dummy-bytes", type=click.IntRange(0, 16), default=0, show_default=True, help="Dummy bytes after the address.")
@click.option("--length", type=click.IntRange(1, 0x1000000), required=True, help="Number of bytes to read.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(dir_okay=False, writable=True, path_type=Path),
    required=True,
    help="Binary output file.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="1m",
    show_default=True,
    help="SPI clock speed.",
)
@click.option("--clock-polarity", type=click.Choice(("idle-low", "idle-high")), default="idle-low", show_default=True)
@click.option(
    "--clock-edge",
    type=click.Choice(("idle-to-active", "active-to-idle")),
    default="active-to-idle",
    show_default=True,
)
@click.option("--sample", type=click.Choice(("middle", "end")), default="middle", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def read_command(
    ctx: click.Context,
    opcode: int,
    address: int,
    address_width: int,
    dummy_bytes: int,
    length: int,
    file_path: Path,
    bus_speed: str,
    clock_polarity: str,
    clock_edge: str,
    sample: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read a flash-style byte range into a binary file."""
    config = _config(clock_polarity, clock_edge, sample, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    try:
        data = read_spi(
            connection,
            opcode=opcode,
            address=address,
            address_width=address_width,
            dummy_bytes=dummy_bytes,
            length=length,
            speed=_SPEED_MAP[bus_speed],
            config=config,
            power=power,
            pullups=pullups,
            session_factory=_factory(),
        )
    except ValueError as exc:
        raise CommandError(str(exc)) from exc
    write_binary_file(file_path, data, label="SPI read")
    click.echo(f"Read {len(data)} bytes from 0x{address:X} to {file_path}.")
    preview = hex_preview(data)
    if preview:
        click.echo(f"First bytes: {preview}")


@spi_group.command("write")
@click.option("--opcode", type=click.IntRange(0, 0xFF), default=None, help="Optional write opcode.")
@click.option("--address", type=click.IntRange(0, 0xFFFFFFFF), default=0, show_default=True, help="Starting address.")
@click.option("--address-width", type=click.IntRange(0, 4), default=3, show_default=True, help="Address bytes.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    required=True,
    help="Binary input file.",
)
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="1m",
    show_default=True,
    help="SPI clock speed.",
)
@click.option("--clock-polarity", type=click.Choice(("idle-low", "idle-high")), default="idle-low", show_default=True)
@click.option(
    "--clock-edge",
    type=click.Choice(("idle-to-active", "active-to-idle")),
    default="active-to-idle",
    show_default=True,
)
@click.option("--sample", type=click.Choice(("middle", "end")), default="middle", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def write_command(
    ctx: click.Context,
    opcode: int | None,
    address: int,
    address_width: int,
    file_path: Path,
    bus_speed: str,
    clock_polarity: str,
    clock_edge: str,
    sample: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Write a binary file after an optional opcode/address prefix."""
    payload = read_binary_file(file_path, label="SPI write")
    config = _config(clock_polarity, clock_edge, sample, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    try:
        write_spi(
            connection,
            opcode=opcode,
            address=address,
            address_width=address_width,
            data=payload,
            speed=_SPEED_MAP[bus_speed],
            config=config,
            power=power,
            pullups=pullups,
            session_factory=_factory(),
        )
    except ValueError as exc:
        raise CommandError(str(exc)) from exc
    click.echo(f"Wrote {len(payload)} bytes from {file_path} at 0x{address:X}.")
