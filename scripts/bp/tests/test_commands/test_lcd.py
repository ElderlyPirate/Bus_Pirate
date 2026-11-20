"""Tests for the LCD adapter v2 console command."""

from __future__ import annotations

from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager

import pytest
from click.testing import CliRunner

from bp.cli import cli
from bp.commands import lcd as lcd_module
from bp.errors import ProtocolError
from bp.protocol.spi import LcdTestSession
from bp.types import ConnectionSettings


class FakeLcdSession:
    """Record LCD cycles without any SPI hardware."""

    def __init__(self, failure: ProtocolError | None = None) -> None:
        self.failure = failure
        self.cycles = 0

    def run_cycle(self) -> None:
        if self.failure is not None:
            raise self.failure
        self.cycles += 1


def _factory(session: FakeLcdSession) -> Callable[[ConnectionSettings], AbstractContextManager[LcdTestSession]]:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[LcdTestSession, None, None]:
        del connection
        yield session

    return factory


def test_one_shot_lcd_test_runs_one_cycle(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeLcdSession()
    monkeypatch.setattr(lcd_module, "open_lcd_session", _factory(session))
    result = CliRunner().invoke(cli, ["lcd-test", "--port", "COM9"])
    assert result.exit_code == 0
    assert session.cycles == 1
    assert "cycle 1/1 complete" in result.output
    assert "Ran 1 LCD adapter v2 test cycle(s)." in result.output


def test_repeat_lcd_test_runs_requested_cycles(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeLcdSession()
    monkeypatch.setattr(lcd_module, "open_lcd_session", _factory(session))
    result = CliRunner().invoke(cli, ["lcd-test", "--port", "COM9", "--repeat", "3"])
    assert result.exit_code == 0
    assert session.cycles == 3
    assert "cycle 3/3 complete" in result.output


def test_interactive_lcd_test_prompts_and_quits(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeLcdSession()
    monkeypatch.setattr(lcd_module, "open_lcd_session", _factory(session))
    result = CliRunner().invoke(cli, ["lcd-test", "--port", "COM9", "--interactive"], input="q\n")
    assert result.exit_code == 0
    assert session.cycles == 1
    assert "Connect another adapter" in result.output


def test_interactive_lcd_test_runs_until_quit(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeLcdSession()
    monkeypatch.setattr(lcd_module, "open_lcd_session", _factory(session))
    result = CliRunner().invoke(cli, ["lcd-test", "--port", "COM9", "--interactive"], input="\nq\n")
    assert result.exit_code == 0
    assert session.cycles == 2


def test_interactive_rejects_repeat(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeLcdSession()
    monkeypatch.setattr(lcd_module, "open_lcd_session", _factory(session))
    result = CliRunner().invoke(cli, ["lcd-test", "--port", "COM9", "--interactive", "--repeat", "2"])
    assert result.exit_code == 2
    assert "--interactive cannot be combined" in result.output
    assert session.cycles == 0


def test_lcd_test_requires_a_port(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeLcdSession()
    monkeypatch.setattr(lcd_module, "open_lcd_session", _factory(session))
    result = CliRunner().invoke(cli, ["lcd-test"])
    assert result.exit_code == 1
    assert "No serial port specified" in result.output
    assert session.cycles == 0


def test_lcd_protocol_failure_exits_nonzero(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeLcdSession(failure=ProtocolError("SPI failure"))
    monkeypatch.setattr(lcd_module, "open_lcd_session", _factory(session))
    result = CliRunner().invoke(cli, ["lcd-test", "--port", "COM9"])
    assert result.exit_code == 1
    assert "SPI failure" in result.output


def test_lcd_test_rejects_invalid_repeat() -> None:
    result = CliRunner().invoke(cli, ["lcd-test", "--port", "COM9", "--repeat", "0"])
    assert result.exit_code == 2
    assert "0 is not in the range x>=1" in result.output
