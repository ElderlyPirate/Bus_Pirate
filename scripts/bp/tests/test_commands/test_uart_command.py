# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the ``bp uart`` command services and Click wiring."""

from __future__ import annotations

from collections.abc import Generator
from contextlib import contextmanager
from pathlib import Path

import pytest
from click.testing import CliRunner

from bp.cli import cli
from bp.commands import uart as uart_module
from bp.commands.uart import UartSessionFactory
from bp.protocol.uart import UartConfig, UartDataBits, UartSession, UartSpeed
from bp.types import ConnectionSettings


class FakeUartSession:
    """Record UART operations without a serial port."""

    def __init__(self, received: bytes = b"hello") -> None:
        self.received = received
        self.calls: list[tuple[str, object]] = []
        self.sent = b""

    def configure_speed(self, speed: UartSpeed) -> None:
        self.calls.append(("speed", speed))

    def configure_manual_speed(self, brg: int) -> None:
        self.calls.append(("manual_speed", brg))

    def configure(self, config: UartConfig) -> None:
        self.calls.append(("config", config))

    def configure_peripherals(
        self,
        *,
        power: bool = False,
        pullups: bool = False,
        aux: bool = False,
        cs: bool = False,
    ) -> None:
        self.calls.append(("peripherals", (power, pullups, aux, cs)))

    def write_bytes(self, data: bytes) -> None:
        self.calls.append(("write", data))
        self.sent += data

    def receive(self, count: int) -> bytes:
        self.calls.append(("receive", count))
        return self.received[:count]

    def begin_input(self) -> None:
        self.calls.append(("begin", None))

    def end_input(self) -> None:
        self.calls.append(("end", None))

    def enter_bridge_mode(self) -> None:
        self.calls.append(("bridge", None))


def _factory(
    session: FakeUartSession,
    captured: list[ConnectionSettings] | None = None,
) -> UartSessionFactory:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[UartSession, None, None]:
        if captured is not None:
            captured.append(connection)
        yield session

    return factory


def test_configure_service_applies_speed_frame_and_power() -> None:
    session = FakeUartSession()
    config = UartConfig(data_bits=UartDataBits.EIGHT_EVEN, stop_bits=2, inverted=True, normal_output=False)
    uart_module.configure_uart(
        ConnectionSettings(port="COM9"),
        UartSpeed.BPS_115200,
        config,
        power=True,
        pullups=True,
        session_factory=_factory(session),
    )
    assert session.calls == [
        ("speed", UartSpeed.BPS_115200),
        ("config", config),
        ("peripherals", (True, True, False, False)),
    ]


def test_manual_speed_service_uses_brg() -> None:
    session = FakeUartSession()
    uart_module.manual_speed_uart(
        ConnectionSettings(port="COM9"),
        411,
        UartConfig(),
        power=False,
        pullups=False,
        session_factory=_factory(session),
    )
    assert session.calls == [
        ("manual_speed", 411),
        ("config", UartConfig()),
        ("peripherals", (False, False, False, False)),
    ]


def test_send_and_receive_services() -> None:
    session = FakeUartSession(b"abc")
    connection = ConnectionSettings(port="COM9")
    uart_module.send_uart(
        connection,
        b"\x01",
        UartSpeed.BPS_9600,
        UartConfig(),
        power=False,
        pullups=False,
        session_factory=_factory(session),
    )
    assert (
        uart_module.receive_uart(
            connection,
            3,
            UartSpeed.BPS_9600,
            UartConfig(),
            power=False,
            pullups=False,
            session_factory=_factory(session),
        )
        == b"abc"
    )
    assert ("write", b"\x01") in session.calls
    assert ("receive", 3) in session.calls


def test_bridge_service_enters_bridge() -> None:
    session = FakeUartSession()
    uart_module.bridge_uart(
        ConnectionSettings(port="COM9"),
        UartSpeed.BPS_9600,
        UartConfig(),
        power=False,
        pullups=False,
        session_factory=_factory(session),
    )
    assert ("bridge", None) in session.calls


def test_cli_help_lists_uart_commands() -> None:
    result = CliRunner().invoke(cli, ["uart", "--help"])
    assert result.exit_code == 0
    for command in ("config", "manual-speed", "send", "receive", "bridge"):
        assert command in result.output


def test_cli_config_prints_settings(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeUartSession()
    monkeypatch.setattr(uart_module, "open_uart_session", _factory(session))
    result = CliRunner().invoke(
        cli,
        [
            "uart",
            "config",
            "--baud",
            "115200",
            "--data-bits-parity",
            "8e",
            "--stop-bits",
            "2",
            "--polarity",
            "inverted",
            "--output",
            "open-drain",
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 0
    assert "115200 bps, 8e, 2 stop bit(s), inverted polarity, open-drain" in result.output


def test_cli_manual_speed(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeUartSession()
    monkeypatch.setattr(uart_module, "open_uart_session", _factory(session))
    result = CliRunner().invoke(cli, ["uart", "manual-speed", "--brg", "411", "--port", "COM9"])
    assert result.exit_code == 0
    assert "BRG 0x019B (411)" in result.output
    assert ("manual_speed", 411) in session.calls


def test_cli_send_and_receive_hex(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeUartSession(b"\x10\x20")
    monkeypatch.setattr(uart_module, "open_uart_session", _factory(session))
    send = CliRunner().invoke(cli, ["uart", "send", "--data", "AA BB", "--port", "COM9"])
    receive = CliRunner().invoke(cli, ["uart", "receive", "--length", "2", "--port", "COM9"])
    assert send.exit_code == 0
    assert "AA BB" in send.output
    assert receive.exit_code == 0
    assert "10 20" in receive.output


def test_cli_receive_to_file_and_timeout(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    session = FakeUartSession(b"hi")
    captured: list[ConnectionSettings] = []
    monkeypatch.setattr(uart_module, "open_uart_session", _factory(session, captured))
    output = tmp_path / "rx.txt"
    result = CliRunner().invoke(
        cli,
        ["uart", "receive", "--length", "2", "--timeout", "0.5", "--file", str(output), "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert output.read_text(encoding="ascii") == "68 69\n"
    assert captured[0].read_timeout == 0.5


def test_cli_bridge_requires_confirmation() -> None:
    result = CliRunner().invoke(cli, ["uart", "bridge", "--port", "COM9"])
    assert result.exit_code == 2
    assert "only exits on a Bus Pirate reset" in result.output


def test_cli_bridge_with_confirmation(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeUartSession()
    monkeypatch.setattr(uart_module, "open_uart_session", _factory(session))
    result = CliRunner().invoke(cli, ["uart", "bridge", "--yes", "--port", "COM9"])
    assert result.exit_code == 0
    assert "UART bridge active" in result.output
    assert ("bridge", None) in session.calls


def test_cli_rejects_invalid_baud() -> None:
    result = CliRunner().invoke(cli, ["uart", "config", "--baud", "12345", "--port", "COM9"])
    assert result.exit_code == 2
    assert "Invalid value for '--baud'" in result.output


def test_cli_rejects_missing_send_data() -> None:
    result = CliRunner().invoke(cli, ["uart", "send", "--port", "COM9"])
    assert result.exit_code == 1
    assert "Supply --data or --file" in result.output
