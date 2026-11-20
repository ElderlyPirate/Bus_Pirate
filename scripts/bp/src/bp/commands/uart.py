# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""``bp uart`` command group for binary UART send, receive, and bridge setup."""

from __future__ import annotations

from collections.abc import Callable
from contextlib import AbstractContextManager
from dataclasses import replace
from pathlib import Path

import click

from bp.commands import get_app_context
from bp.commands.inputs import resolve_hex_data
from bp.hextext import format_hex_text, write_hex_file
from bp.protocol.uart import (
    UartConfig,
    UartDataBits,
    UartSession,
    UartSpeed,
    open_uart_session,
)
from bp.types import ConnectionSettings

_BAUD_MAP = {
    "300": UartSpeed.BPS_300,
    "1200": UartSpeed.BPS_1200,
    "2400": UartSpeed.BPS_2400,
    "4800": UartSpeed.BPS_4800,
    "9600": UartSpeed.BPS_9600,
    "19200": UartSpeed.BPS_19200,
    "38400": UartSpeed.BPS_38400,
    "57600": UartSpeed.BPS_57600,
    "115200": UartSpeed.BPS_115200,
    "31250": UartSpeed.BPS_31250,
}

_DATA_MAP = {
    "8n": UartDataBits.EIGHT_NONE,
    "8e": UartDataBits.EIGHT_EVEN,
    "8o": UartDataBits.EIGHT_ODD,
    "9n": UartDataBits.NINE_NONE,
}

UartSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[UartSession]]


def _prepare(
    session: UartSession,
    baud: UartSpeed,
    config: UartConfig,
    *,
    power: bool,
    pullups: bool,
) -> None:
    session.configure_speed(baud)
    session.configure(config)
    session.configure_peripherals(power=power, pullups=pullups)


def configure_uart(
    connection: ConnectionSettings,
    baud: UartSpeed,
    config: UartConfig,
    *,
    power: bool,
    pullups: bool,
    session_factory: UartSessionFactory,
) -> None:
    """Enter UART mode and apply baud, frame format, and peripheral settings."""
    with session_factory(connection) as session:
        _prepare(session, baud, config, power=power, pullups=pullups)


def manual_speed_uart(
    connection: ConnectionSettings,
    brg: int,
    config: UartConfig,
    *,
    power: bool,
    pullups: bool,
    session_factory: UartSessionFactory,
) -> None:
    """Enter UART mode with a raw BRG value and apply the frame format."""
    with session_factory(connection) as session:
        session.configure_manual_speed(brg)
        session.configure(config)
        session.configure_peripherals(power=power, pullups=pullups)


def send_uart(
    connection: ConnectionSettings,
    data: bytes,
    baud: UartSpeed,
    config: UartConfig,
    *,
    power: bool,
    pullups: bool,
    session_factory: UartSessionFactory,
) -> None:
    """Send bytes over UART after applying baud and frame settings."""
    with session_factory(connection) as session:
        _prepare(session, baud, config, power=power, pullups=pullups)
        session.write_bytes(data)


def receive_uart(
    connection: ConnectionSettings,
    count: int,
    baud: UartSpeed,
    config: UartConfig,
    *,
    power: bool,
    pullups: bool,
    session_factory: UartSessionFactory,
) -> bytes:
    """Receive bytes from UART after applying baud and frame settings."""
    with session_factory(connection) as session:
        _prepare(session, baud, config, power=power, pullups=pullups)
        return session.receive(count)


def bridge_uart(
    connection: ConnectionSettings,
    baud: UartSpeed,
    config: UartConfig,
    *,
    power: bool,
    pullups: bool,
    session_factory: UartSessionFactory,
) -> None:
    """Enter the firmware transparent UART bridge."""
    with session_factory(connection) as session:
        _prepare(session, baud, config, power=power, pullups=pullups)
        session.enter_bridge_mode()


def _config(data_bits: str, stop_bits: int, polarity: str, output: str) -> UartConfig:
    return UartConfig(
        data_bits=_DATA_MAP[data_bits],
        stop_bits=stop_bits,
        inverted=polarity == "inverted",
        normal_output=output == "normal",
    )


def _factory() -> UartSessionFactory:
    def factory(connection: ConnectionSettings) -> AbstractContextManager[UartSession]:
        return open_uart_session(connection)

    return factory


@click.group("uart")
def uart_group() -> None:
    """Binary UART configuration, send, receive, and bridge setup."""


@uart_group.command("config")
@click.option(
    "--baud",
    type=click.Choice(tuple(_BAUD_MAP)),
    default="9600",
    show_default=True,
    help="UART baud rate.",
)
@click.option("--data-bits-parity", "data_bits", type=click.Choice(tuple(_DATA_MAP)), default="8n", show_default=True)
@click.option("--stop-bits", type=click.Choice(("1", "2")), default="1", show_default=True)
@click.option("--polarity", type=click.Choice(("normal", "inverted")), default="normal", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def config_command(
    ctx: click.Context,
    baud: str,
    data_bits: str,
    stop_bits: str,
    polarity: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Enter UART mode, apply the configuration, and return to bitbang."""
    config = _config(data_bits, int(stop_bits), polarity, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    configure_uart(
        connection,
        _BAUD_MAP[baud],
        config,
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(f"UART configured: {baud} bps, {data_bits}, {stop_bits} stop bit(s), {polarity} polarity, {output}.")


@uart_group.command("manual-speed")
@click.option("--brg", type=click.IntRange(0, 0xFFFF), required=True, help="Raw 16-bit BRG value.")
@click.option("--data-bits-parity", "data_bits", type=click.Choice(tuple(_DATA_MAP)), default="8n", show_default=True)
@click.option("--stop-bits", type=click.Choice(("1", "2")), default="1", show_default=True)
@click.option("--polarity", type=click.Choice(("normal", "inverted")), default="normal", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def manual_speed_command(
    ctx: click.Context,
    brg: int,
    data_bits: str,
    stop_bits: str,
    polarity: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Enter UART mode using a raw 16-bit BRG value."""
    config = _config(data_bits, int(stop_bits), polarity, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    manual_speed_uart(
        connection,
        brg,
        config,
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(f"UART configured with BRG 0x{brg:04X} ({brg}).")


@uart_group.command("send")
@click.option("--data", type=str, default=None, help="Whitespace-separated hex bytes to send.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    default=None,
    help="Text-hex file to send.",
)
@click.option(
    "--baud",
    type=click.Choice(tuple(_BAUD_MAP)),
    default="9600",
    show_default=True,
    help="UART baud rate.",
)
@click.option("--data-bits-parity", "data_bits", type=click.Choice(tuple(_DATA_MAP)), default="8n", show_default=True)
@click.option("--stop-bits", type=click.Choice(("1", "2")), default="1", show_default=True)
@click.option("--polarity", type=click.Choice(("normal", "inverted")), default="normal", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def send_command(
    ctx: click.Context,
    data: str | None,
    file_path: Path | None,
    baud: str,
    data_bits: str,
    stop_bits: str,
    polarity: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Send hex bytes over UART."""
    payload = resolve_hex_data(data, file_path, label="data")
    config = _config(data_bits, int(stop_bits), polarity, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    send_uart(
        connection,
        payload,
        _BAUD_MAP[baud],
        config,
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo(f"Sent {len(payload)} UART byte(s): {format_hex_text(payload).strip()}")


@uart_group.command("receive")
@click.option("--length", "count", type=click.IntRange(1, 0x10000), required=True, help="Number of bytes to receive.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(dir_okay=False, writable=True, path_type=Path),
    default=None,
    help="Optional text-hex output file.",
)
@click.option(
    "--timeout",
    type=click.FloatRange(min=0.001),
    default=5.0,
    show_default=True,
    help="Serial read timeout in seconds.",
)
@click.option(
    "--baud",
    type=click.Choice(tuple(_BAUD_MAP)),
    default="9600",
    show_default=True,
    help="UART baud rate.",
)
@click.option("--data-bits-parity", "data_bits", type=click.Choice(tuple(_DATA_MAP)), default="8n", show_default=True)
@click.option("--stop-bits", type=click.Choice(("1", "2")), default="1", show_default=True)
@click.option("--polarity", type=click.Choice(("normal", "inverted")), default="normal", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def receive_command(
    ctx: click.Context,
    count: int,
    file_path: Path | None,
    timeout: float,
    baud: str,
    data_bits: str,
    stop_bits: str,
    polarity: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Receive bytes from UART into hex output or a file."""
    config = _config(data_bits, int(stop_bits), polarity, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    timed_connection = replace(connection, read_timeout=timeout)
    data = receive_uart(
        timed_connection,
        count,
        _BAUD_MAP[baud],
        config,
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    if file_path is not None:
        write_hex_file(file_path, data)
        click.echo(f"Received {len(data)} bytes to {file_path}.")
    else:
        click.echo(format_hex_text(data).strip())


@uart_group.command("bridge")
@click.option("--yes", "confirmed", is_flag=True, help="Confirm entering the bridge, which only a hardware reset exits.")
@click.option(
    "--baud",
    type=click.Choice(tuple(_BAUD_MAP)),
    default="9600",
    show_default=True,
    help="UART baud rate.",
)
@click.option("--data-bits-parity", "data_bits", type=click.Choice(tuple(_DATA_MAP)), default="8n", show_default=True)
@click.option("--stop-bits", type=click.Choice(("1", "2")), default="1", show_default=True)
@click.option("--polarity", type=click.Choice(("normal", "inverted")), default="normal", show_default=True)
@click.option("--output", type=click.Choice(("normal", "open-drain")), default="normal", show_default=True)
@click.option("--power/--no-power", default=False, show_default=True, help="Drive target power.")
@click.option("--pullups/--no-pullups", default=False, show_default=True, help="Enable on-board pull-ups.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def bridge_command(
    ctx: click.Context,
    confirmed: bool,
    baud: str,
    data_bits: str,
    stop_bits: str,
    polarity: str,
    output: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Enter the firmware UART bridge; only a Bus Pirate reset exits it."""
    if not confirmed:
        raise click.UsageError("UART bridge mode only exits on a Bus Pirate reset; pass --yes to confirm")
    config = _config(data_bits, int(stop_bits), polarity, output)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    bridge_uart(
        connection,
        _BAUD_MAP[baud],
        config,
        power=power,
        pullups=pullups,
        session_factory=_factory(),
    )
    click.echo("UART bridge active. Reset the Bus Pirate hardware to leave bridge mode.")
