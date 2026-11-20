"""Console entry point and command wiring."""

from __future__ import annotations

from pathlib import Path

import click

from bp.commands import eeprom, lcd, version
from bp.config import AppContext, load_config
from bp.errors import BpError
from bp.logging_config import configure_logging


class BusPirateGroup(click.Group):
    """Click group that maps expected domain failures to CLI errors."""

    def invoke(self, ctx: click.Context) -> object:
        """Run a subcommand and translate expected domain failures."""
        try:
            return super().invoke(ctx)
        except BpError as exc:
            raise click.ClickException(str(exc)) from exc


@click.group(name="bp", cls=BusPirateGroup)
@click.option(
    "--config",
    "config_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    default=None,
    help="Optional YAML configuration file with default 'port' and 'speed' values.",
)
@click.option("--verbose", is_flag=True, help="Enable diagnostic stdlib logging.")
@click.pass_context
def cli(ctx: click.Context, config_path: Path | None, verbose: bool) -> None:
    """Host frontend for Bus Pirate v3/v4 devices."""
    configure_logging(verbose=verbose)
    ctx.obj = AppContext(config=load_config(config_path))


cli.add_command(version.version_command)
cli.add_command(eeprom.eeprom_group)
cli.add_command(lcd.lcd_test_command)


def main() -> None:
    """Run the ``bp`` console script."""
    cli(prog_name="bp")
