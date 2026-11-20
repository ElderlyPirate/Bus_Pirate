# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""``bp bbio`` command group for binary bitbang operations."""

from __future__ import annotations

from pathlib import Path

import click

from bp.commands import get_app_context
from bp.commands.inputs import resolve_hex_data
from bp.errors import CommandError
from bp.hextext import format_hex_text
from bp.protocol.bbio import (
    BitbangSessionFactory,
    BitbangState,
    SelfTestReport,
    open_bitbang_session,
)
from bp.types import ConnectionSettings


def describe_state(state: int) -> str:
    """Return a human-readable list of set binary-bitbang state bits."""
    names = [flag.name for flag in BitbangState if flag.name is not None and flag.value and flag.value & state == flag.value]
    return ", ".join(names) if names else "none"


def _apply_flags(state: int, *, power: bool, pullups: bool, aux: bool, cs: bool) -> int:
    mask = state
    if power:
        mask |= int(BitbangState.POWER)
    if pullups:
        mask |= int(BitbangState.PULLUP)
    if aux:
        mask |= int(BitbangState.AUX)
    if cs:
        mask |= int(BitbangState.CS)
    return mask


def configure_bitbang_pins(
    connection: ConnectionSettings,
    *,
    direction: int | None,
    state: int | None,
    power: bool,
    pullups: bool,
    aux: bool,
    cs: bool,
    session_factory: BitbangSessionFactory,
) -> int:
    """Optionally apply direction/state masks and return the pin state."""
    flags_requested = power or pullups or aux or cs
    with session_factory(connection) as session:
        result = session.read_pins()
        if direction is not None:
            result = session.configure_directions(direction)
        if state is not None or flags_requested:
            base_state = state if state is not None else 0
            result = session.set_pins(_apply_flags(base_state, power=power, pullups=pullups, aux=aux, cs=cs))
        return result


def enter_bitbang(connection: ConnectionSettings) -> None:
    """Enter binary bitbang mode and leave the Bus Pirate there."""
    with open_bitbang_session(connection, reset_on_exit=False):
        pass


def reset_bitbang(connection: ConnectionSettings) -> None:
    """Enter binary bitbang mode, then reset to the user terminal."""
    with open_bitbang_session(connection):
        pass


def read_bitbang_version(connection: ConnectionSettings, *, session_factory: BitbangSessionFactory) -> bytes:
    """Request and return the binary-bitbang mode identifier."""
    with session_factory(connection) as session:
        return session.version()


def run_bitbang_self_test(
    connection: ConnectionSettings,
    *,
    long_test: bool,
    session_factory: BitbangSessionFactory,
) -> SelfTestReport:
    """Run the firmware self-test and return its error count."""
    with session_factory(connection) as session:
        report = session.self_test(long_test=long_test)
    if report.error_count:
        raise CommandError(f"Self-test reported {report.error_count} error(s)")
    return report


def read_bitbang_adc(connection: ConnectionSettings, *, session_factory: BitbangSessionFactory) -> int:
    """Read one raw ADC sample."""
    with session_factory(connection) as session:
        return session.read_adc()


def setup_bitbang_pwm(
    connection: ConnectionSettings,
    *,
    prescaler: int,
    duty: int,
    period: int,
    session_factory: BitbangSessionFactory,
) -> None:
    """Configure the AUX PWM output."""
    with session_factory(connection) as session:
        session.setup_pwm(prescaler=prescaler, duty=duty, period=period)


def clear_bitbang_pwm(connection: ConnectionSettings, *, session_factory: BitbangSessionFactory) -> None:
    """Stop and release the AUX PWM output."""
    with session_factory(connection) as session:
        session.clear_pwm()


def write_bitbang_bytes(
    connection: ConnectionSettings,
    data: bytes,
    *,
    lsb_first: bool,
    power: bool,
    pullups: bool,
    session_factory: BitbangSessionFactory,
) -> None:
    """Clock bytes out on MOSI, optionally applying power and pull-ups first."""
    with session_factory(connection) as session:
        if power:
            session.set_power(True)
        if pullups:
            session.set_pullups(True)
        session.write_bytes(data, lsb_first=lsb_first)


def read_bitbang_bytes(
    connection: ConnectionSettings,
    count: int,
    *,
    lsb_first: bool,
    power: bool,
    pullups: bool,
    session_factory: BitbangSessionFactory,
) -> bytes:
    """Clock bytes in from MISO, optionally applying power and pull-ups first."""
    with session_factory(connection) as session:
        if power:
            session.set_power(True)
        if pullups:
            session.set_pullups(True)
        return session.read_bytes(count, lsb_first=lsb_first)


def _resolve_data(data: str | None, file_path: Path | None) -> bytes:
    return resolve_hex_data(data, file_path, label="data")


@click.group("bbio")
def bbio_group() -> None:
    """Binary bitbang entry, pin control, self-test, ADC, PWM, and raw bits."""


@bbio_group.command("enter")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def enter_command(ctx: click.Context, port: str | None, speed: int | None) -> None:
    """Enter binary bitbang mode and leave the Bus Pirate in it."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    enter_bitbang(connection)
    click.echo("Entered binary bitbang mode (BBIO1).")


@bbio_group.command("reset")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def reset_command(ctx: click.Context, port: str | None, speed: int | None) -> None:
    """Reset the Bus Pirate to its user terminal."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    reset_bitbang(connection)
    click.echo("Reset the Bus Pirate to its user terminal.")


@bbio_group.command("version")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def version_command(ctx: click.Context, port: str | None, speed: int | None) -> None:
    """Request the binary-bitbang mode identifier."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    banner = read_bitbang_version(connection, session_factory=open_bitbang_session)
    click.echo(f"Mode identifier: {banner.decode('ascii', errors='replace')}")


@bbio_group.command("pins")
@click.option("--direction", type=click.IntRange(0, 0x1F), default=None, help="Direction mask; set bits are inputs.")
@click.option("--state", type=click.IntRange(0, 0x7F), default=None, help="State mask; set bits are high.")
@click.option("--power", is_flag=True, help="Enable the on-board power supplies.")
@click.option("--pullups", is_flag=True, help="Enable the on-board pull-up resistors.")
@click.option("--aux", is_flag=True, help="Drive AUX high.")
@click.option("--cs", is_flag=True, help="Drive CS high.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def pins_command(
    ctx: click.Context,
    direction: int | None,
    state: int | None,
    power: bool,
    pullups: bool,
    aux: bool,
    cs: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Configure directions/state and read the binary-bitbang pins."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    result = configure_bitbang_pins(
        connection,
        direction=direction,
        state=state,
        power=power,
        pullups=pullups,
        aux=aux,
        cs=cs,
        session_factory=open_bitbang_session,
    )
    click.echo(f"Pin state: 0x{result:02X} ({describe_state(result)}).")


@bbio_group.command("selftest")
@click.option("--long", "long_test", is_flag=True, help="Run the full self-test instead of the short test.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def selftest_command(ctx: click.Context, long_test: bool, port: str | None, speed: int | None) -> None:
    """Run the Bus Pirate self-test and report its error count."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    report = run_bitbang_self_test(connection, long_test=long_test, session_factory=open_bitbang_session)
    label = "full" if report.long_test else "short"
    click.echo(f"{label.capitalize()} self-test completed with {report.error_count} error(s).")


@bbio_group.command("adc")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def adc_command(ctx: click.Context, port: str | None, speed: int | None) -> None:
    """Read one raw ADC sample."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    sample = read_bitbang_adc(connection, session_factory=open_bitbang_session)
    click.echo(f"ADC sample: 0x{sample:04X} ({sample}).")


@bbio_group.command("pwm")
@click.option("--prescaler", type=click.IntRange(0, 3), default=None, help="PWM prescaler (0-3).")
@click.option("--duty", type=click.IntRange(0, 0xFFFF), default=None, help="PWM duty cycle (0-65535).")
@click.option("--period", type=click.IntRange(0, 0xFFFF), default=None, help="PWM period (0-65535).")
@click.option("--clear", "clear_pwm", is_flag=True, help="Stop and release the AUX PWM output.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def pwm_command(
    ctx: click.Context,
    prescaler: int | None,
    duty: int | None,
    period: int | None,
    clear_pwm: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Configure or clear the AUX PWM output."""
    if clear_pwm and any(value is not None for value in (prescaler, duty, period)):
        raise click.UsageError("--clear cannot be combined with --prescaler, --duty, or --period")
    if not clear_pwm and any(value is None for value in (prescaler, duty, period)):
        raise click.UsageError("--prescaler, --duty, and --period are required unless --clear is used")
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    if clear_pwm:
        clear_bitbang_pwm(connection, session_factory=open_bitbang_session)
        click.echo("Cleared the AUX PWM output.")
        return
    if prescaler is None or duty is None or period is None:
        raise click.UsageError("--prescaler, --duty, and --period are required unless --clear is used")
    setup_bitbang_pwm(
        connection,
        prescaler=prescaler,
        duty=duty,
        period=period,
        session_factory=open_bitbang_session,
    )
    click.echo(f"PWM configured: prescaler={prescaler}, duty={duty}, period={period}.")


@bbio_group.command("write")
@click.option("--data", type=str, default=None, help="Whitespace-separated hex bytes to clock out.")
@click.option(
    "--file",
    "file_path",
    type=click.Path(exists=True, dir_okay=False, readable=True, path_type=Path),
    default=None,
    help="Text-hex file to clock out.",
)
@click.option("--lsb-first", is_flag=True, help="Send each byte least-significant bit first.")
@click.option("--power", is_flag=True, help="Enable the on-board power supplies first.")
@click.option("--pullups", is_flag=True, help="Enable the on-board pull-up resistors first.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def write_command(
    ctx: click.Context,
    data: str | None,
    file_path: Path | None,
    lsb_first: bool,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Clock raw bytes out on MOSI with CLK pulses."""
    payload = _resolve_data(data, file_path)
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    write_bitbang_bytes(
        connection,
        payload,
        lsb_first=lsb_first,
        power=power,
        pullups=pullups,
        session_factory=open_bitbang_session,
    )
    click.echo(f"Clocked out {len(payload)} byte(s): {format_hex_text(payload).strip()}")


@bbio_group.command("read")
@click.option("--count", type=click.IntRange(1, 0x10000), required=True, help="Number of bytes to clock in.")
@click.option("--lsb-first", is_flag=True, help="Assemble each byte least-significant bit first.")
@click.option("--power", is_flag=True, help="Enable the on-board power supplies first.")
@click.option("--pullups", is_flag=True, help="Enable the on-board pull-up resistors first.")
@click.option("--port", type=str, default=None, help="Serial port; defaults to the configuration file.")
@click.option("--speed", type=click.IntRange(min=1), default=None, help="Serial speed; defaults to 115200.")
@click.pass_context
def read_command(
    ctx: click.Context,
    count: int,
    lsb_first: bool,
    power: bool,
    pullups: bool,
    port: str | None,
    speed: int | None,
) -> None:
    """Clock raw bytes in from MISO with CLK pulses."""
    connection = get_app_context(ctx).resolve_connection(port=port, speed=speed)
    payload = read_bitbang_bytes(
        connection,
        count,
        lsb_first=lsb_first,
        power=power,
        pullups=pullups,
        session_factory=open_bitbang_session,
    )
    click.echo(format_hex_text(payload).strip())
