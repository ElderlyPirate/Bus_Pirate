"""``bp script`` / ``bp raw-script`` ASCII-hex binary command loop."""

from __future__ import annotations

from dataclasses import dataclass, replace
from pathlib import Path

import click

from bp.commands import get_app_context
from bp.errors import CommandError
from bp.hextext import read_hex_file
from bp.protocol.bbio import BbioSession
from bp.transport import TransportFactory, open_transport
from bp.types import ConnectionSettings

SCRIPT_MAX_REPLY_BYTES = 256


@dataclass(frozen=True, slots=True)
class ScriptReply:
    """One byte sent through the binary command loop and its reply."""

    index: int
    sent: int
    reply: bytes


def run_raw_script(
    connection: ConnectionSettings,
    data: bytes,
    *,
    max_reply_bytes: int = SCRIPT_MAX_REPLY_BYTES,
    reset_after: bool = False,
    transport_factory: TransportFactory = open_transport,
) -> tuple[ScriptReply, ...]:
    """Enter binary bitbang and send each byte as one binary command.

    The reply is whatever the device sends before the serial timeout elapses.

    Raises:
        CommandError: If the script is empty or the reply limit is invalid.
        ProtocolError: If binary entry fails or the optional reset is rejected.
    """
    if not data:
        raise CommandError("Raw script contains no bytes")
    if max_reply_bytes < 1:
        raise ValueError("Raw-script reply limit must be positive")
    replies: list[ScriptReply] = []
    with transport_factory(connection) as transport:
        bbio = BbioSession(transport)
        bbio.enter_binary()
        for index, value in enumerate(data, start=1):
            transport.write(bytes([value]))
            reply = transport.read_until_idle(max_bytes=max_reply_bytes)
            replies.append(ScriptReply(index=index, sent=value, reply=reply))
        if reset_after:
            bbio.reset()
    return tuple(replies)


def _format_reply(reply: bytes) -> str:
    return " ".join(f"{value:02X}" for value in reply) if reply else "(none)"


def raw_script_command(
    file_path: Path,
    reply_timeout: float,
    max_reply_bytes: int,
    reset_after: bool,
    port: str | None,
    speed: int | None,
    ctx: click.Context,
) -> None:
    """Implementation shared by the ``script`` and ``raw-script`` names."""
    payload = read_hex_file(file_path)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    timed_connection = replace(connection, read_timeout=reply_timeout)
    replies = run_raw_script(
        timed_connection,
        payload,
        max_reply_bytes=max_reply_bytes,
        reset_after=reset_after,
        transport_factory=open_transport,
    )
    for reply in replies:
        click.echo(f"{reply.index:03d} | raw bytes text: {reply.sent:02X} tx: {reply.sent:3d} rx: {_format_reply(reply.reply)}")
    if reset_after:
        click.echo("Reset the Bus Pirate to its user terminal.")
    click.echo(f"Exec: ASCII Script...Success! ({len(replies)} bytes)")


@click.command("script")
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    required=True,
    help="Whitespace-separated ASCII-hex file.",
)
@click.option(
    "--reply-timeout",
    type=click.FloatRange(min=0.001),
    default=0.1,
    show_default=True,
    help="Seconds to wait for each command reply.",
)
@click.option(
    "--max-reply-bytes",
    type=click.IntRange(1, 0x10000),
    default=SCRIPT_MAX_REPLY_BYTES,
    show_default=True,
    help="Maximum reply bytes to print for one command.",
)
@click.option("--reset", "reset_after", is_flag=True, help="Reset to the user terminal after the script.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def script_command(
    ctx: click.Context,
    file_path: Path,
    reply_timeout: float,
    max_reply_bytes: int,
    reset_after: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Run an ASCII-hex file through the binary command loop."""
    raw_script_command(file_path, reply_timeout, max_reply_bytes, reset_after, port, speed, ctx)
