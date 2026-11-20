# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the ``bp bbio`` command services and Click wiring."""

from __future__ import annotations

from collections.abc import Generator
from contextlib import contextmanager

import pytest
from click.testing import CliRunner

from bp.cli import cli
from bp.commands import bbio as bbio_module
from bp.errors import CommandError
from bp.protocol.bbio import BitbangSession, BitbangSessionFactory, SelfTestReport
from bp.types import ConnectionSettings


class FakeBitbangSession:
    """Record bitbang operations without a serial port."""

    def __init__(self, *, state: int = 0x41, error_count: int = 0) -> None:
        self.state = state
        self.error_count = error_count
        self.calls: list[tuple[str, object]] = []
        self.written = b""
        self.adc = 0x0123

    def version(self) -> bytes:
        self.calls.append(("version", None))
        return b"BBIO1"

    def configure_directions(self, mask: int) -> int:
        self.calls.append(("directions", mask))
        return self.state

    def set_pins(self, mask: int) -> int:
        self.calls.append(("pins", mask))
        return self.state

    def read_pins(self) -> int:
        self.calls.append(("read", None))
        return self.state

    def set_power(self, enabled: bool) -> int:
        self.calls.append(("power", enabled))
        return self.state

    def set_pullups(self, enabled: bool) -> int:
        self.calls.append(("pullups", enabled))
        return self.state

    def set_aux(self, high: bool) -> int:
        self.calls.append(("aux", high))
        return self.state

    def set_cs(self, high: bool) -> int:
        self.calls.append(("cs", high))
        return self.state

    def write_bits(self, value: int, count: int, *, lsb_first: bool = False) -> None:
        self.calls.append(("bits", (value, count, lsb_first)))

    def read_bits(self, count: int, *, lsb_first: bool = False) -> int:
        self.calls.append(("read_bits", (count, lsb_first)))
        return 0

    def write_bytes(self, data: bytes, *, lsb_first: bool = False) -> None:
        self.calls.append(("write", (data, lsb_first)))
        self.written = data

    def read_bytes(self, count: int, *, lsb_first: bool = False) -> bytes:
        self.calls.append(("read_bytes", (count, lsb_first)))
        return bytes(range(count))

    def self_test(self, *, long_test: bool = False) -> SelfTestReport:
        self.calls.append(("selftest", long_test))
        return SelfTestReport(error_count=self.error_count, long_test=long_test)

    def read_adc(self) -> int:
        self.calls.append(("adc", None))
        return self.adc

    def setup_pwm(self, *, prescaler: int, duty: int, period: int) -> None:
        self.calls.append(("pwm", (prescaler, duty, period)))

    def clear_pwm(self) -> None:
        self.calls.append(("clear_pwm", None))

    def reset_to_terminal(self) -> None:
        self.calls.append(("reset", None))


def _factory(
    session: FakeBitbangSession,
    captured: list[tuple[ConnectionSettings, bool]],
) -> BitbangSessionFactory:
    @contextmanager
    def factory(
        connection: ConnectionSettings,
        *,
        reset_on_exit: bool = True,
    ) -> Generator[BitbangSession, None, None]:
        captured.append((connection, reset_on_exit))
        yield session

    return factory


def test_configure_pins_applies_direction_state_and_flags() -> None:
    session = FakeBitbangSession()
    captured: list[tuple[ConnectionSettings, bool]] = []
    connection = ConnectionSettings(port="COM9")
    result = bbio_module.configure_bitbang_pins(
        connection,
        direction=0x13,
        state=0x01,
        power=True,
        pullups=True,
        aux=True,
        cs=False,
        session_factory=_factory(session, captured),
    )
    assert result == 0x41
    assert session.calls == [
        ("read", None),
        ("directions", 0x13),
        ("pins", 0x71),
    ]
    assert captured == [(connection, True)]


def test_configure_pins_without_options_only_reads() -> None:
    session = FakeBitbangSession()
    bbio_module.configure_bitbang_pins(
        ConnectionSettings(port="COM9"),
        direction=None,
        state=None,
        power=False,
        pullups=False,
        aux=False,
        cs=False,
        session_factory=_factory(session, []),
    )
    assert session.calls == [("read", None)]


def test_self_test_success_and_failure() -> None:
    connection = ConnectionSettings(port="COM9")
    report = bbio_module.run_bitbang_self_test(
        connection,
        long_test=True,
        session_factory=_factory(FakeBitbangSession(), []),
    )
    assert report == SelfTestReport(error_count=0, long_test=True)
    with pytest.raises(CommandError, match="2 error"):
        bbio_module.run_bitbang_self_test(
            connection,
            long_test=False,
            session_factory=_factory(FakeBitbangSession(error_count=2), []),
        )


def test_adc_and_pwm_services() -> None:
    connection = ConnectionSettings(port="COM9")
    session = FakeBitbangSession()
    factory = _factory(session, [])
    assert bbio_module.read_bitbang_adc(connection, session_factory=factory) == 0x0123
    bbio_module.setup_bitbang_pwm(connection, prescaler=1, duty=2, period=3, session_factory=factory)
    bbio_module.clear_bitbang_pwm(connection, session_factory=factory)
    assert session.calls == [
        ("adc", None),
        ("pwm", (1, 2, 3)),
        ("clear_pwm", None),
    ]


def test_write_and_read_services_apply_power_and_pullups() -> None:
    connection = ConnectionSettings(port="COM9")
    writer = FakeBitbangSession()
    bbio_module.write_bitbang_bytes(
        connection,
        b"\xaa\xbb",
        lsb_first=True,
        power=True,
        pullups=True,
        session_factory=_factory(writer, []),
    )
    assert writer.calls == [("power", True), ("pullups", True), ("write", (b"\xaa\xbb", True))]

    reader = FakeBitbangSession()
    assert (
        bbio_module.read_bitbang_bytes(
            connection,
            3,
            lsb_first=False,
            power=False,
            pullups=False,
            session_factory=_factory(reader, []),
        )
        == b"\x00\x01\x02"
    )
    assert reader.calls == [("read_bytes", (3, False))]


def test_cli_help_lists_bitbang_commands() -> None:
    result = CliRunner().invoke(cli, ["bbio", "--help"])
    assert result.exit_code == 0
    for command in ("enter", "reset", "version", "pins", "selftest", "adc", "pwm", "write", "read"):
        assert command in result.output


def test_cli_enter_and_reset_use_expected_cleanup_modes(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeBitbangSession()
    captured: list[tuple[ConnectionSettings, bool]] = []
    monkeypatch.setattr(bbio_module, "open_bitbang_session", _factory(session, captured))

    entered = CliRunner().invoke(cli, ["bbio", "enter", "--port", "COM9"])
    reset = CliRunner().invoke(cli, ["bbio", "reset", "--port", "COM9"])

    assert entered.exit_code == 0
    assert "Entered binary bitbang mode" in entered.output
    assert reset.exit_code == 0
    assert [item[1] for item in captured] == [False, True]


def test_cli_pins_prints_state_and_descriptions(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeBitbangSession(state=0x41)
    monkeypatch.setattr(bbio_module, "open_bitbang_session", _factory(session, []))
    result = CliRunner().invoke(
        cli,
        ["bbio", "pins", "--direction", "19", "--state", "1", "--power", "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert "Pin state: 0x41" in result.output
    assert "CS" in result.output
    assert "POWER" in result.output


def test_cli_selftest_failure_exits_one(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(bbio_module, "open_bitbang_session", _factory(FakeBitbangSession(error_count=1), []))
    result = CliRunner().invoke(cli, ["bbio", "selftest", "--port", "COM9"])
    assert result.exit_code == 1
    assert "1 error" in result.output


def test_cli_adc_and_pwm_commands(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeBitbangSession()
    monkeypatch.setattr(bbio_module, "open_bitbang_session", _factory(session, []))
    adc = CliRunner().invoke(cli, ["bbio", "adc", "--port", "COM9"])
    pwm = CliRunner().invoke(
        cli,
        ["bbio", "pwm", "--prescaler", "1", "--duty", "2", "--period", "3", "--port", "COM9"],
    )
    cleared = CliRunner().invoke(cli, ["bbio", "pwm", "--clear", "--port", "COM9"])
    assert adc.exit_code == 0
    assert "0x0123" in adc.output
    assert pwm.exit_code == 0
    assert cleared.exit_code == 0
    assert ("pwm", (1, 2, 3)) in session.calls
    assert ("clear_pwm", None) in session.calls


def test_cli_pwm_requires_all_parameters_or_clear() -> None:
    missing = CliRunner().invoke(cli, ["bbio", "pwm", "--prescaler", "1", "--port", "COM9"])
    conflict = CliRunner().invoke(cli, ["bbio", "pwm", "--clear", "--duty", "2", "--port", "COM9"])
    assert missing.exit_code == 2
    assert "--prescaler, --duty, and --period" in missing.output
    assert conflict.exit_code == 2
    assert "--clear cannot be combined" in conflict.output


def test_cli_write_and_read(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeBitbangSession()
    monkeypatch.setattr(bbio_module, "open_bitbang_session", _factory(session, []))
    write = CliRunner().invoke(cli, ["bbio", "write", "--data", "AA BB", "--port", "COM9"])
    read = CliRunner().invoke(cli, ["bbio", "read", "--count", "2", "--port", "COM9"])
    assert write.exit_code == 0
    assert "AA BB" in write.output
    assert read.exit_code == 0
    assert "00 01" in read.output


def test_cli_write_rejects_invalid_hex_and_missing_source() -> None:
    invalid = CliRunner().invoke(cli, ["bbio", "write", "--data", "GG", "--port", "COM9"])
    missing = CliRunner().invoke(cli, ["bbio", "write", "--port", "COM9"])
    assert invalid.exit_code == 1
    assert "Invalid hex token" in invalid.output
    assert missing.exit_code == 1
    assert "Supply --data or --file" in missing.output


def test_cli_rejects_out_of_range_direction_and_state() -> None:
    direction = CliRunner().invoke(cli, ["bbio", "pins", "--direction", "32", "--port", "COM9"])
    state = CliRunner().invoke(cli, ["bbio", "pins", "--state", "128", "--port", "COM9"])
    assert direction.exit_code == 2
    assert state.exit_code == 2
