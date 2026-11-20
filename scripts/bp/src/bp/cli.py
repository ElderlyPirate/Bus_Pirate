"""Console entry point and command wiring."""

from __future__ import annotations

import copy
from pathlib import Path

import click

from bp.commands import bbio, eeprom, i2c, lcd, onewire, raw_wire, script, spi, uart, version, xsvf
from bp.config import AppContext, load_config
from bp.errors import BpError, ConfigError
from bp.logging_config import configure_logging


class _HostSetupClickException(click.ClickException):
    """Click exception for host, transport, or setup failures that exit with status 2."""

    exit_code = 2


class BusPirateGroup(click.Group):
    """Click group that maps expected domain failures to CLI errors."""

    def invoke(self, ctx: click.Context) -> object:
        """Run a subcommand and translate expected domain failures."""
        try:
            return super().invoke(ctx)
        except BpError as exc:
            if exc.exit_code == 2:
                raise _HostSetupClickException(str(exc)) from exc
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
    try:
        config = load_config(config_path)
    except ConfigError as exc:
        if ctx.invoked_subcommand == "xsvf":
            raise _HostSetupClickException(str(exc)) from exc
        raise
    ctx.obj = AppContext(config=config)


cli.add_command(version.version_command)
cli.add_command(bbio.bbio_group)
cli.add_command(raw_wire.raw_wire_group)
cli.add_command(i2c.i2c_group)
cli.add_command(spi.spi_group)
cli.add_command(uart.uart_group)
cli.add_command(onewire.onewire_group)
cli.add_command(eeprom.eeprom_group)
cli.add_command(lcd.lcd_test_command)
cli.add_command(xsvf.xsvf_command)

_raw_script_alias = copy.copy(script.script_command)
_raw_script_alias.name = "raw-script"
cli.add_command(script.script_command)
cli.add_command(_raw_script_alias)


def main() -> None:
    """Run the ``bp`` console script."""
    cli(prog_name="bp")
