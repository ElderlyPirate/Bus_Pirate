# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""``bp version`` command."""

from __future__ import annotations

import click

from bp.commands import get_app_context
from bp.config import default_serial_port
from bp.protocol.bbio import read_version_info
from bp.transport import TransportFactory, open_transport
from bp.types import ConnectionSettings, VersionInfo


def read_version(connection: ConnectionSettings, *, transport_factory: TransportFactory) -> VersionInfo:
    """Read the reset banner through an injected transport factory."""
    with transport_factory(connection) as transport:
        return read_version_info(transport)


@click.command("version")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the platform default or configuration.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def version_command(ctx: click.Context, port: str | None, speed: int | None) -> None:
    """Read the firmware and hardware version banner."""
    connection = get_app_context(ctx).resolve_connection(
        port=port,
        speed=speed,
        fallback_port=default_serial_port(),
    )
    info = read_version(connection, transport_factory=open_transport)
    click.echo(f"Hardware type:    {info.hardware_type}")
    click.echo(f"Firmware version: {info.firmware_version}")
    click.echo(f"Hardware version: {info.hardware_version}")
    click.echo(f"Update URL:       {info.update_url}")
