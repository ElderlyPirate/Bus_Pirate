"""``bp lcd-test`` command for the LCD adapter v2 production test."""

from __future__ import annotations

import click

from bp.commands import get_app_context
from bp.protocol.spi import LcdTestSession, open_lcd_session


@click.command("lcd-test")
@click.option("--repeat", type=click.IntRange(min=1), default=1, show_default=True, help="Number of test cycles to run.")
@click.option("--interactive", is_flag=True, help="Prompt between cycles and run until the operator quits.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def lcd_test_command(
    ctx: click.Context,
    repeat: int,
    interactive: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Run the LCD adapter v2 LED/output test."""
    if interactive and repeat != 1:
        raise click.UsageError("--interactive cannot be combined with --repeat greater than 1")
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)

    with open_lcd_session(connection) as session:
        if interactive:
            cycles = _run_interactive(session)
        else:
            cycles = _run_repeated(session, repeat)

    click.echo(f"Ran {cycles} LCD adapter v2 test cycle(s).")


def _run_repeated(session: LcdTestSession, repeat: int) -> int:
    for index in range(1, repeat + 1):
        session.run_cycle()
        click.echo(f"LCD adapter v2 test cycle {index}/{repeat} complete; LED1 should have blinked.")
    return repeat


def _run_interactive(session: LcdTestSession) -> int:
    cycles = 0
    while True:
        session.run_cycle()
        cycles += 1
        click.echo(f"LCD adapter v2 test cycle {cycles} complete; LED1 should have blinked.")
        answer = click.prompt(
            "Connect another adapter and press Enter for another test, or type 'q' to finish",
            default="",
            show_default=False,
        )
        if answer.strip().lower() in {"q", "quit"}:
            return cycles
