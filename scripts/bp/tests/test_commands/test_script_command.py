"""Tests for ``bp script`` / ``bp raw-script`` binary command execution."""

from __future__ import annotations

from collections.abc import Generator
from contextlib import contextmanager
from pathlib import Path

import pytest
from click.testing import CliRunner

from bp.cli import cli
from bp.commands import script as script_module
from bp.errors import CommandError
from bp.transport import Transport
from bp.types import ConnectionSettings
from conftest import ScriptedTransport


def test_run_script_enters_binary_and_sends_each_byte() -> None:
    transport = ScriptedTransport([b"BBIO1", b"BBIO1", b"\x01"])
    replies = script_module.run_raw_script(
        ConnectionSettings(port="COM9"),
        b"\x01\x0f",
        transport_factory=lambda connection: _transport(connection, transport),
    )
    assert [(reply.index, reply.sent, reply.reply) for reply in replies] == [
        (1, 0x01, b"BBIO1"),
        (2, 0x0F, b"\x01"),
    ]
    assert transport.writes == [b"\x00" * 21, b"\x01", b"\x0f"]


def test_run_script_can_reset_after() -> None:
    transport = ScriptedTransport([b"BBIO1", b"BBIO1", b"\x01", b"\x01"])
    replies = script_module.run_raw_script(
        ConnectionSettings(port="COM9"),
        b"\x01",
        reset_after=True,
        transport_factory=lambda connection: _transport(connection, transport),
    )
    assert replies[0].reply == b"BBIO1"
    assert transport.writes == [b"\x00" * 21, b"\x01", b"\x0f"]


def test_run_script_returns_empty_reply_when_device_is_silent() -> None:
    transport = ScriptedTransport([b"BBIO1"])
    replies = script_module.run_raw_script(
        ConnectionSettings(port="COM9"),
        b"\x00",
        transport_factory=lambda connection: _transport(connection, transport),
    )
    assert replies[0].reply == b""


def test_run_script_rejects_empty_payload() -> None:
    with pytest.raises(CommandError, match="no bytes"):
        script_module.run_raw_script(ConnectionSettings(port="COM9"), b"")


def test_run_script_rejects_invalid_reply_limit() -> None:
    with pytest.raises(ValueError, match="reply limit must be positive"):
        script_module.run_raw_script(ConnectionSettings(port="COM9"), b"\x01", max_reply_bytes=0)


def test_run_script_passes_reply_timeout_to_connection() -> None:
    captured: list[ConnectionSettings] = []
    transport = ScriptedTransport([b"BBIO1"])
    connection = ConnectionSettings(port="COM9", read_timeout=0.25)
    script_module.run_raw_script(
        connection,
        b"\x00",
        transport_factory=lambda settings: _capture(settings, captured, transport),
    )
    assert captured == [connection]


def test_cli_help_lists_script_aliases() -> None:
    runner = CliRunner()
    help_text = runner.invoke(cli, ["--help"])
    assert "script" in help_text.output
    assert "raw-script" in help_text.output
    assert runner.invoke(cli, ["script", "--help"]).exit_code == 0
    assert runner.invoke(cli, ["raw-script", "--help"]).exit_code == 0


def test_cli_script_prints_replies(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    transport = ScriptedTransport([b"BBIO1", b"BBIO1", b"\x01"])
    monkeypatch.setattr(script_module, "open_transport", lambda connection: _transport(connection, transport))
    source = tmp_path / "script.txt"
    source.write_text("01 0F\n", encoding="ascii")
    result = CliRunner().invoke(cli, ["script", "--file", str(source), "--port", "COM9"])
    assert result.exit_code == 0
    assert "001 | raw bytes text: 01 tx:   1 rx: 42 42 49 4F 31" in result.output
    assert "002 | raw bytes text: 0F tx:  15 rx: 01" in result.output
    assert "Exec: ASCII Script...Success! (2 bytes)" in result.output


def test_cli_raw_script_alias_runs(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    transport = ScriptedTransport([b"BBIO1", b"BBIO1"])
    monkeypatch.setattr(script_module, "open_transport", lambda connection: _transport(connection, transport))
    source = tmp_path / "script.txt"
    source.write_text("01\n", encoding="ascii")
    result = CliRunner().invoke(cli, ["raw-script", "--file", str(source), "--port", "COM9"])
    assert result.exit_code == 0
    assert "Success! (1 bytes)" in result.output


def test_cli_script_reset_flag(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    transport = ScriptedTransport([b"BBIO1", b"BBIO1", b"\x01", b"\x01"])
    monkeypatch.setattr(script_module, "open_transport", lambda connection: _transport(connection, transport))
    source = tmp_path / "script.txt"
    source.write_text("01\n", encoding="ascii")
    result = CliRunner().invoke(cli, ["script", "--file", str(source), "--reset", "--port", "COM9"])
    assert result.exit_code == 0
    assert "Reset the Bus Pirate to its user terminal." in result.output
    assert transport.writes[-1] == b"\x0f"


def test_cli_script_passes_read_timeout(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    captured: list[ConnectionSettings] = []
    transport = ScriptedTransport([b"BBIO1", b"BBIO1"])
    monkeypatch.setattr(
        script_module,
        "open_transport",
        lambda connection: _capture(connection, captured, transport),
    )
    source = tmp_path / "script.txt"
    source.write_text("01\n", encoding="ascii")
    result = CliRunner().invoke(
        cli,
        ["script", "--file", str(source), "--reply-timeout", "0.25", "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert captured[0].read_timeout == 0.25


def test_cli_script_rejects_invalid_hex(tmp_path: Path) -> None:
    source = tmp_path / "bad.txt"
    source.write_text("GG\n", encoding="ascii")
    result = CliRunner().invoke(cli, ["script", "--file", str(source), "--port", "COM9"])
    assert result.exit_code == 1
    assert "Invalid hex token" in result.output


def test_cli_script_rejects_empty_file(tmp_path: Path) -> None:
    source = tmp_path / "empty.txt"
    source.write_text(" \n", encoding="ascii")
    result = CliRunner().invoke(cli, ["script", "--file", str(source), "--port", "COM9"])
    assert result.exit_code == 1
    assert "contains no bytes" in result.output


def test_cli_script_rejects_missing_file() -> None:
    result = CliRunner().invoke(cli, ["script", "--file", "missing.txt", "--port", "COM9"])
    assert result.exit_code == 2
    assert "does not exist" in result.output


def test_cli_script_rejects_invalid_reply_limit(tmp_path: Path) -> None:
    source = tmp_path / "script.txt"
    source.write_text("01\n", encoding="ascii")
    result = CliRunner().invoke(cli, ["script", "--file", str(source), "--max-reply-bytes", "0", "--port", "COM9"])
    assert result.exit_code == 2


@contextmanager
def _transport(connection: ConnectionSettings, transport: ScriptedTransport) -> Generator[Transport, None, None]:
    del connection
    yield transport


@contextmanager
def _capture(
    connection: ConnectionSettings,
    captured: list[ConnectionSettings],
    transport: ScriptedTransport,
) -> Generator[Transport, None, None]:
    captured.append(connection)
    yield transport
