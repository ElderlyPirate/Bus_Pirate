# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""``bp ports`` command for listing serial ports."""

from __future__ import annotations

import click

from bp.ports import list_serial_ports


@click.command("ports")
def ports_command() -> None:
    """List serial ports and mark likely Bus Pirate devices without connecting."""
    ports = list_serial_ports()
    if not ports:
        click.echo("No serial ports found.")
        return
    for port in ports:
        marker = f"  [{port.bus_pirate_kind.value}]" if port.bus_pirate_kind is not None else ""
        click.echo(f"{port.device}  {port.vid_pid or '----:----'}  {port.description}{marker}")
