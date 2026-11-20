# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""``bp raw-wire`` command group for binary raw 2-wire and 3-wire mode."""

from __future__ import annotations

from pathlib import Path

import click

from bp.commands import get_app_context
from bp.commands.inputs import resolve_hex_data
from bp.hextext import format_hex_text
from bp.protocol.raw_wire import (
    RawWireConfig,
    RawWireSession,
    RawWireSessionFactory,
    RawWireSpeed,
    open_raw_wire_session,
)
from bp.types import ConnectionSettings

_SPEED_MAP = {
    "5k": RawWireSpeed.KHZ_5,
    "50k": RawWireSpeed.KHZ_50,
    "100k": RawWireSpeed.KHZ_100,
    "400k": RawWireSpeed.KHZ_400,
}


def _config(output: str, wires: str, lsb_first: bool) -> RawWireConfig:
    return RawWireConfig(output_3v3=output == "normal", three_wire=wires == "3", lsb_first=lsb_first)


def _prepare(
    session: RawWireSession,
    config: RawWireConfig,
    speed: RawWireSpeed,
    *,
    power: bool,
    pullups: bool,
) -> None:
    session.configure(config)
    session.set_speed(speed)
    if power or pullups:
        session.configure_peripherals(power=power, pullups=pullups)


def configure_raw_wire(
    connection: ConnectionSettings,
    config: RawWireConfig,
    speed: RawWireSpeed,
    *,
    power: bool,
    pullups: bool,
    session_factory: RawWireSessionFactory,
) -> None:
    """Enter raw-wire mode and apply its configuration."""
    with session_factory(connection) as session:
        _prepare(session, config, speed, power=power, pullups=pullups)


def write_raw_wire(
    connection: ConnectionSettings,
    data: bytes,
    config: RawWireConfig,
    speed: RawWireSpeed,
    *,
    power: bool,
    pullups: bool,
    use_cs: bool,
    start: bool,
    stop: bool,
    session_factory: RawWireSessionFactory,
) -> None:
    """Write bytes in raw-wire mode, optionally framing them with CS/start/stop."""
    with session_factory(connection) as session:
        _prepare(session, config, speed, power=power, pullups=pullups)
        if use_cs:
            session.set_cs(low=True)
        if start:
            session.start()
        session.write_bytes(data)
        if stop:
            session.stop()
        if use_cs:
            session.set_cs(low=False)


def read_raw_wire(
    connection: ConnectionSettings,
    count: int,
    config: RawWireConfig,
    speed: RawWireSpeed,
    *,
    power: bool,
    pullups: bool,
    use_cs: bool,
    start: bool,
    stop: bool,
    session_factory: RawWireSessionFactory,
) -> bytes:
    """Read bytes in raw-wire mode, optionally framing them with CS/start/stop."""
    with session_factory(connection) as session:
        _prepare(session, config, speed, power=power, pullups=pullups)
        if use_cs:
            session.set_cs(low=True)
        if start:
            session.start()
        data = session.read_bytes(count)
        if stop:
            session.stop()
        if use_cs:
            session.set_cs(low=False)
        return data


def read_raw_wire_bit(
    connection: ConnectionSettings,
    config: RawWireConfig,
    speed: RawWireSpeed,
    *,
    power: bool,
    pullups: bool,
    session_factory: RawWireSessionFactory,
) -> bool:
    """Read one raw-wire bit."""
    with session_factory(connection) as session:
        _prepare(session, config, speed, power=power, pullups=pullups)
        return session.read_bit()


def peek_raw_wire(
    connection: ConnectionSettings,
    config: RawWireConfig,
    speed: RawWireSpeed,
    *,
    power: bool,
    pullups: bool,
    session_factory: RawWireSessionFactory,
) -> bool:
    """Read the raw-wire input pin without clocking."""
    with session_factory(connection) as session:
        _prepare(session, config, speed, power=power, pullups=pullups)
        return session.peek()


def clock_raw_wire(
    connection: ConnectionSettings,
    ticks: int,
    config: RawWireConfig,
    speed: RawWireSpeed,
    *,
    power: bool,
    pullups: bool,
    session_factory: RawWireSessionFactory,
) -> None:
    """Emit raw-wire clock ticks."""
    with session_factory(connection) as session:
        _prepare(session, config, speed, power=power, pullups=pullups)
        session.bulk_clock(ticks)


def _resolve_data(data: str | None, file_path: Path | None) -> bytes:
    return resolve_hex_data(data, file_path, label="data")


@click.group("raw-wire")
def raw_wire_group() -> None:
    """Binary raw 2-wire/3-wire configuration, byte, bit, and clock operations."""


@raw_wire_group.command("config")
@click.option(
    "--output",
    type=click.Choice(("normal", "open-drain")),
    default="normal",
    show_default=True,
    help="Output type: 3.3 V normal or open-drain.",
)
@click.option(
    "--wires",
    type=click.Choice(("2", "3")),
    default="3",
    show_default=True,
    help="Raw-wire mode: two or three wires.",
)
@click.option("--lsb-first", is_flag=True, help="Send or assemble bytes least-significant bit first.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="50k",
    show_default=True,
    help="Raw-wire clock speed.",
)
@click.option("--power", is_flag=True, help="Enable the on-board power supplies.")
@click.option("--pullups", is_flag=True, help="Enable the on-board pull-up resistors.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def config_command(
    ctx: click.Context,
    output: str,
    wires: str,
    lsb_first: bool,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Enter raw-wire mode, apply the configuration, and return to bitbang."""
    config = _config(output, wires, lsb_first)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    configure_raw_wire(
        connection,
        config,
        _SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=open_raw_wire_session,
    )
    order = "LSB first" if config.lsb_first else "MSB first"
    click.echo(f"Raw-wire configured: {wires} wires, {output}, {order}, {bus_speed}.")


@raw_wire_group.command("write")
@click.option("--data", type=str, default=None, help="Whitespace-separated hex bytes to write.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    default=None,
    help="Text-hex file to write.",
)
@click.option(
    "--output",
    type=click.Choice(("normal", "open-drain")),
    default="normal",
    show_default=True,
    help="Output type: 3.3 V normal or open-drain.",
)
@click.option(
    "--wires",
    type=click.Choice(("2", "3")),
    default="3",
    show_default=True,
    help="Raw-wire mode: two or three wires.",
)
@click.option("--lsb-first", is_flag=True, help="Send or assemble bytes least-significant bit first.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="50k",
    show_default=True,
    help="Raw-wire clock speed.",
)
@click.option("--cs/--no-cs", "use_cs", default=None, help="Assert CS low around the transfer; defaults to on for 3-wire.")
@click.option("--start", is_flag=True, help="Emit a 2-wire start condition before the data.")
@click.option("--stop", is_flag=True, help="Emit a 2-wire stop condition after the data.")
@click.option("--power", is_flag=True, help="Enable the on-board power supplies.")
@click.option("--pullups", is_flag=True, help="Enable the on-board pull-up resistors.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def write_command(
    ctx: click.Context,
    data: str | None,
    file_path: Path | None,
    output: str,
    wires: str,
    lsb_first: bool,
    bus_speed: str,
    use_cs: bool | None,
    start: bool,
    stop: bool,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Write bytes in raw-wire mode."""
    payload = _resolve_data(data, file_path)
    config = _config(output, wires, lsb_first)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    write_raw_wire(
        connection,
        payload,
        config,
        _SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        use_cs=(wires == "3") if use_cs is None else use_cs,
        start=start,
        stop=stop,
        session_factory=open_raw_wire_session,
    )
    click.echo(f"Wrote {len(payload)} raw-wire byte(s): {format_hex_text(payload).strip()}")


@raw_wire_group.command("read")
@click.option("--count", type=click.IntRange(1, 0x10000), required=True, help="Number of bytes to read.")
@click.option(
    "--output",
    type=click.Choice(("normal", "open-drain")),
    default="normal",
    show_default=True,
    help="Output type: 3.3 V normal or open-drain.",
)
@click.option(
    "--wires",
    type=click.Choice(("2", "3")),
    default="3",
    show_default=True,
    help="Raw-wire mode: two or three wires.",
)
@click.option("--lsb-first", is_flag=True, help="Send or assemble bytes least-significant bit first.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="50k",
    show_default=True,
    help="Raw-wire clock speed.",
)
@click.option("--cs/--no-cs", "use_cs", default=None, help="Assert CS low around the transfer; defaults to on for 3-wire.")
@click.option("--start", is_flag=True, help="Emit a 2-wire start condition before the data.")
@click.option("--stop", is_flag=True, help="Emit a 2-wire stop condition after the data.")
@click.option("--power", is_flag=True, help="Enable the on-board power supplies.")
@click.option("--pullups", is_flag=True, help="Enable the on-board pull-up resistors.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def read_command(
    ctx: click.Context,
    count: int,
    output: str,
    wires: str,
    lsb_first: bool,
    bus_speed: str,
    use_cs: bool | None,
    start: bool,
    stop: bool,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read bytes in raw-wire mode."""
    config = _config(output, wires, lsb_first)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    payload = read_raw_wire(
        connection,
        count,
        config,
        _SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        use_cs=(wires == "3") if use_cs is None else use_cs,
        start=start,
        stop=stop,
        session_factory=open_raw_wire_session,
    )
    click.echo(format_hex_text(payload).strip())


@raw_wire_group.command("bit")
@click.option(
    "--output",
    type=click.Choice(("normal", "open-drain")),
    default="normal",
    show_default=True,
    help="Output type: 3.3 V normal or open-drain.",
)
@click.option(
    "--wires",
    type=click.Choice(("2", "3")),
    default="3",
    show_default=True,
    help="Raw-wire mode: two or three wires.",
)
@click.option("--lsb-first", is_flag=True, help="Send or assemble bytes least-significant bit first.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="50k",
    show_default=True,
    help="Raw-wire clock speed.",
)
@click.option("--power", is_flag=True, help="Enable the on-board power supplies.")
@click.option("--pullups", is_flag=True, help="Enable the on-board pull-up resistors.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def bit_command(
    ctx: click.Context,
    output: str,
    wires: str,
    lsb_first: bool,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read one raw-wire bit."""
    config = _config(output, wires, lsb_first)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    value = read_raw_wire_bit(
        connection,
        config,
        _SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=open_raw_wire_session,
    )
    click.echo(f"Bit: {1 if value else 0}")


@raw_wire_group.command("peek")
@click.option(
    "--output",
    type=click.Choice(("normal", "open-drain")),
    default="normal",
    show_default=True,
    help="Output type: 3.3 V normal or open-drain.",
)
@click.option(
    "--wires",
    type=click.Choice(("2", "3")),
    default="3",
    show_default=True,
    help="Raw-wire mode: two or three wires.",
)
@click.option("--lsb-first", is_flag=True, help="Send or assemble bytes least-significant bit first.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="50k",
    show_default=True,
    help="Raw-wire clock speed.",
)
@click.option("--power", is_flag=True, help="Enable the on-board power supplies.")
@click.option("--pullups", is_flag=True, help="Enable the on-board pull-up resistors.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def peek_command(
    ctx: click.Context,
    output: str,
    wires: str,
    lsb_first: bool,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Read the raw-wire input pin without clocking."""
    config = _config(output, wires, lsb_first)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    value = peek_raw_wire(
        connection,
        config,
        _SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=open_raw_wire_session,
    )
    click.echo(f"Pin: {1 if value else 0}")


@raw_wire_group.command("clock")
@click.option("--ticks", type=click.IntRange(1, 16), required=True, help="Number of clock ticks (1-16).")
@click.option(
    "--output",
    type=click.Choice(("normal", "open-drain")),
    default="normal",
    show_default=True,
    help="Output type: 3.3 V normal or open-drain.",
)
@click.option(
    "--wires",
    type=click.Choice(("2", "3")),
    default="3",
    show_default=True,
    help="Raw-wire mode: two or three wires.",
)
@click.option("--lsb-first", is_flag=True, help="Send or assemble bytes least-significant bit first.")
@click.option(
    "--bus-speed",
    type=click.Choice(tuple(_SPEED_MAP)),
    default="50k",
    show_default=True,
    help="Raw-wire clock speed.",
)
@click.option("--power", is_flag=True, help="Enable the on-board power supplies.")
@click.option("--pullups", is_flag=True, help="Enable the on-board pull-up resistors.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def clock_command(
    ctx: click.Context,
    ticks: int,
    output: str,
    wires: str,
    lsb_first: bool,
    bus_speed: str,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Emit raw-wire clock ticks."""
    config = _config(output, wires, lsb_first)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    clock_raw_wire(
        connection,
        ticks,
        config,
        _SPEED_MAP[bus_speed],
        power=power,
        pullups=pullups,
        session_factory=open_raw_wire_session,
    )
    click.echo(f"Emitted {ticks} clock tick(s).")
