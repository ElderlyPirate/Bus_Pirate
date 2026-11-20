# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""``bp xsvf`` command for the Bus Pirate v4 XSVF player."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import click

from bp.commands import get_app_context
from bp.errors import CommandError, TransportError, XsvfHostError
from bp.protocol.xsvf import (
    ChainScan,
    XsvfPlayReport,
    XsvfSessionFactory,
    open_xsvf_session,
    result_name,
)
from bp.types import ConnectionSettings


@dataclass(frozen=True, slots=True)
class XsvfRunReport:
    """Completed XSVF operations, in the order the player executed them."""

    reset: bool
    scan: ChainScan | None
    play: XsvfPlayReport | None


def read_xsvf_file(path: Path) -> bytes:
    """Read an XSVF image, rejecting unreadable and empty files before the port is opened."""
    try:
        data = path.read_bytes()
    except OSError as exc:
        raise XsvfHostError(f"Could not read XSVF file {path}: {exc}") from exc
    if not data:
        raise XsvfHostError(f"XSVF file {path} is empty")
    return data


def run_xsvf(
    connection: ConnectionSettings,
    data: bytes | None,
    *,
    reset: bool,
    scan: bool,
    session_factory: XsvfSessionFactory,
) -> XsvfRunReport:
    """Enter the player and run the requested reset, scan, and stream operations."""
    if data is None and not reset and not scan:
        raise XsvfHostError("No XSVF file or player operation was selected")
    if data is not None and not data:
        raise XsvfHostError("XSVF stream is empty")
    try:
        with session_factory(connection) as session:
            if reset:
                session.reset_chain()
            scan_result = session.scan_chain() if scan else None
            play_result = session.play(data) if data is not None else None
    except TransportError as exc:
        raise XsvfHostError(str(exc)) from exc
    return XsvfRunReport(reset=reset, scan=scan_result, play=play_result)


@click.command("xsvf")
@click.option("--port", "-p", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option(
    "--file",
    "-f",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    default=None,
    help="XSVF file to stream; required unless --scan or --reset is used.",
)
@click.option("--speed", "-s", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.option("--scan", "-x", is_flag=True, help="Probe the JTAG chain and print its count and IDCODE bytes.")
@click.option("--reset", "-r", is_flag=True, help="Reset the JTAG chain before scanning or streaming.")
@click.pass_context
def xsvf_command(
    ctx: click.Context,
    port: str | None,
    file_path: Path | None,
    speed: int | None,
    scan: bool,
    reset: bool,
) -> None:
    """Play an XSVF file or run a JTAG chain scan/reset with a Bus Pirate v4."""
    if file_path is None and not scan and not reset:
        raise click.UsageError("No file specified; pass --file, --scan, or --reset")
    try:
        connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    except CommandError as exc:
        raise XsvfHostError(str(exc)) from exc

    data = read_xsvf_file(file_path) if file_path is not None else None
    if data is not None:
        click.echo(f"File is {len(data)} bytes.")
    click.echo(f"Opening Bus Pirate on {connection.port} at {connection.speed} bps.")

    report = run_xsvf(
        connection,
        data,
        reset=reset,
        scan=scan,
        session_factory=open_xsvf_session,
    )

    if report.reset:
        click.echo("Performing Reset.. Done")
    if report.scan is not None:
        values = "".join(f" {value:02X}" for value in report.scan.raw)
        click.echo(f"Chain Scan Result:{values}")
    if report.play is not None:
        click.echo(f"Sent {report.play.bytes_sent} bytes in {report.play.chunks} chunk(s).")
        click.echo(f"End of operation reply: {result_name(report.play.result)}")
        click.echo("Success!")
    click.echo("Thank you for playing! :-)")
