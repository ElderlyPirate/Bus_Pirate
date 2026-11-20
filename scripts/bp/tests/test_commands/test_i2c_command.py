# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the ``bp i2c`` command services and Click wiring."""

from __future__ import annotations

from collections.abc import Generator
from contextlib import contextmanager
from pathlib import Path

import pytest
from click.testing import CliRunner

from bp.cli import cli
from bp.commands import i2c as i2c_module
from bp.commands.i2c import I2cSessionFactory
from bp.errors import I2cNackError
from bp.protocol.i2c import I2cControlSession, I2cScanHit, I2cSpeed
from bp.types import ConnectionSettings


class FakeI2cDevice:
    """Record register-level I2C operations."""

    def __init__(self) -> None:
        self.calls: list[tuple[str, object]] = []
        self.fail: I2cNackError | None = None
        self.write_data = b""

    def read_byte(self, register: int) -> int:
        self.calls.append(("read_byte", register))
        return 0xA5

    def write_byte(self, register: int, value: int) -> None:
        self.calls.append(("write_byte", (register, value)))
        if self.fail is not None:
            raise self.fail

    def read_word(self, register: int) -> int:
        self.calls.append(("read_word", register))
        return 0x1234

    def write_word(self, register: int, value: int) -> None:
        self.calls.append(("write_word", (register, value)))

    def command(self, command: int) -> None:
        self.calls.append(("command", command))

    def read_block(self, register: int | None, length: int) -> bytes:
        self.calls.append(("read_block", (register, length)))
        return bytes(range(length))

    def write_block(self, register: int | None, data: bytes) -> None:
        self.calls.append(("write_block", (register, data)))
        self.write_data = data


class FakeI2cSession:
    """Record I2C control operations."""

    def __init__(self, hits: tuple[I2cScanHit, ...] = ()) -> None:
        self.hits = hits
        self.speeds: list[I2cSpeed] = []
        self.devices: dict[int, FakeI2cDevice] = {}
        self.scan_calls: list[tuple[int, int]] = []

    def configure_speed(self, speed: I2cSpeed) -> None:
        self.speeds.append(speed)

    def scan(self, *, start: int = 0, end: int = 0x7F) -> tuple[I2cScanHit, ...]:
        self.scan_calls.append((start, end))
        return self.hits

    def device(self, address: int) -> FakeI2cDevice:
        return self.devices.setdefault(address, FakeI2cDevice())


def _factory(
    session: FakeI2cSession,
    captured: list[tuple[ConnectionSettings, bool, bool]] | None = None,
) -> I2cSessionFactory:
    @contextmanager
    def factory(
        connection: ConnectionSettings,
        *,
        power: bool = False,
        pullups: bool = True,
    ) -> Generator[I2cControlSession, None, None]:
        if captured is not None:
            captured.append((connection, power, pullups))
        yield session

    return factory


def test_scan_service_configures_speed_and_returns_hits() -> None:
    hit = I2cScanHit(address=0x50, write_ack=True, read_ack=True)
    session = FakeI2cSession((hit,))
    result = i2c_module.scan_i2c(
        ConnectionSettings(port="COM9"),
        speed=I2cSpeed.KHZ_400,
        start=0x10,
        end=0x20,
        session_factory=_factory(session),
    )
    assert result == (hit,)
    assert session.speeds == [I2cSpeed.KHZ_400]
    assert session.scan_calls == [(0x10, 0x20)]


def test_register_services_use_the_selected_device() -> None:
    session = FakeI2cSession()
    connection = ConnectionSettings(port="COM9")
    factory = _factory(session)
    assert i2c_module.get_i2c_byte(connection, speed=I2cSpeed.KHZ_100, address=0x50, register=1, session_factory=factory) == 0xA5
    i2c_module.set_i2c_byte(connection, speed=I2cSpeed.KHZ_100, address=0x50, register=1, value=2, session_factory=factory)
    assert (
        i2c_module.get_i2c_word(connection, speed=I2cSpeed.KHZ_100, address=0x50, register=1, session_factory=factory) == 0x1234
    )
    i2c_module.set_i2c_word(connection, speed=I2cSpeed.KHZ_100, address=0x50, register=1, value=2, session_factory=factory)
    i2c_module.send_i2c_command(connection, speed=I2cSpeed.KHZ_100, address=0x50, command=0x51, session_factory=factory)
    assert session.devices[0x50].calls == [
        ("read_byte", 1),
        ("write_byte", (1, 2)),
        ("read_word", 1),
        ("write_word", (1, 2)),
        ("command", 0x51),
    ]


def test_block_services_use_register_and_length() -> None:
    session = FakeI2cSession()
    connection = ConnectionSettings(port="COM9")
    factory = _factory(session)
    assert (
        i2c_module.read_i2c_block(
            connection,
            speed=I2cSpeed.KHZ_100,
            address=0x50,
            register=0x10,
            length=3,
            session_factory=factory,
        )
        == b"\x00\x01\x02"
    )
    i2c_module.write_i2c_block(
        connection,
        speed=I2cSpeed.KHZ_100,
        address=0x50,
        register=None,
        data=b"\xaa\xbb",
        session_factory=factory,
    )
    assert session.devices[0x50].calls == [
        ("read_block", (0x10, 3)),
        ("write_block", (None, b"\xaa\xbb")),
    ]


def test_cli_help_lists_i2c_commands() -> None:
    result = CliRunner().invoke(cli, ["i2c", "--help"])
    assert result.exit_code == 0
    for command in ("scan", "get-byte", "set-byte", "get-word", "set-word", "command", "read", "write"):
        assert command in result.output


def test_cli_scan_prints_hits_and_passes_power_options(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeI2cSession((I2cScanHit(address=0x50, write_ack=True, read_ack=True),))
    captured: list[tuple[ConnectionSettings, bool, bool]] = []
    monkeypatch.setattr(i2c_module, "open_i2c_session", _factory(session, captured))

    result = CliRunner().invoke(
        cli,
        ["i2c", "scan", "--start", "80", "--end", "81", "--power", "--no-pullups", "--port", "COM9"],
    )

    assert result.exit_code == 0
    assert "0x50" in result.output
    assert "W" in result.output
    assert "R" in result.output
    assert captured == [(ConnectionSettings(port="COM9"), True, False)]
    assert session.scan_calls == [(80, 81)]


def test_cli_scan_reports_empty_range(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(i2c_module, "open_i2c_session", _factory(FakeI2cSession()))
    result = CliRunner().invoke(cli, ["i2c", "scan", "--start", "16", "--end", "17", "--port", "COM9"])
    assert result.exit_code == 0
    assert "No I2C devices acknowledged addresses 0x10 through 0x11." in result.output


def test_cli_scan_rejects_reversed_range() -> None:
    result = CliRunner().invoke(cli, ["i2c", "scan", "--start", "16", "--end", "15", "--port", "COM9"])
    assert result.exit_code == 2
    assert "--start must not be greater" in result.output


def test_cli_get_and_set_byte(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeI2cSession()
    monkeypatch.setattr(i2c_module, "open_i2c_session", _factory(session))
    get = CliRunner().invoke(
        cli,
        ["i2c", "get-byte", "--address", "80", "--register", "16", "--port", "COM9"],
    )
    set_result = CliRunner().invoke(
        cli,
        ["i2c", "set-byte", "--address", "80", "--register", "16", "--value", "165", "--port", "COM9"],
    )
    assert get.exit_code == 0
    assert "0xA5" in get.output
    assert set_result.exit_code == 0
    assert "Wrote 0xA5" in set_result.output


def test_cli_get_and_set_word(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeI2cSession()
    monkeypatch.setattr(i2c_module, "open_i2c_session", _factory(session))
    get = CliRunner().invoke(
        cli,
        ["i2c", "get-word", "--address", "80", "--register", "16", "--port", "COM9"],
    )
    set_result = CliRunner().invoke(
        cli,
        ["i2c", "set-word", "--address", "80", "--register", "16", "--value", "4660", "--port", "COM9"],
    )
    assert get.exit_code == 0
    assert "0x1234" in get.output
    assert set_result.exit_code == 0
    assert "0x1234" in set_result.output


def test_cli_command_read_and_write(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    session = FakeI2cSession()
    monkeypatch.setattr(i2c_module, "open_i2c_session", _factory(session))
    command = CliRunner().invoke(
        cli,
        ["i2c", "command", "--address", "80", "--command", "81", "--port", "COM9"],
    )
    output = tmp_path / "read.txt"
    read = CliRunner().invoke(
        cli,
        [
            "i2c",
            "read",
            "--address",
            "80",
            "--register",
            "16",
            "--length",
            "3",
            "--file",
            str(output),
            "--port",
            "COM9",
        ],
    )
    write = CliRunner().invoke(
        cli,
        ["i2c", "write", "--address", "80", "--register", "16", "--data", "AA BB", "--port", "COM9"],
    )
    assert command.exit_code == 0
    assert "0x51" in command.output
    assert read.exit_code == 0
    assert output.read_text(encoding="ascii") == "00 01 02\n"
    assert write.exit_code == 0
    assert session.devices[0x50].write_data == b"\xaa\xbb"


def test_cli_read_without_file_prints_hex(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(i2c_module, "open_i2c_session", _factory(FakeI2cSession()))
    result = CliRunner().invoke(
        cli,
        ["i2c", "read", "--address", "80", "--length", "3", "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert "00 01 02" in result.output


def test_cli_nack_exits_one(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeI2cSession()
    session.device(0x50).fail = I2cNackError(0x50, "register write")
    monkeypatch.setattr(i2c_module, "open_i2c_session", _factory(session))
    result = CliRunner().invoke(
        cli,
        ["i2c", "set-byte", "--address", "80", "--register", "16", "--value", "1", "--port", "COM9"],
    )
    assert result.exit_code == 1
    assert "did not acknowledge register write" in result.output


def test_cli_rejects_invalid_address_and_register() -> None:
    address = CliRunner().invoke(cli, ["i2c", "get-byte", "--address", "128", "--register", "0", "--port", "COM9"])
    register = CliRunner().invoke(cli, ["i2c", "get-byte", "--address", "80", "--register", "256", "--port", "COM9"])
    assert address.exit_code == 2
    assert register.exit_code == 2


def test_cli_write_rejects_missing_data() -> None:
    result = CliRunner().invoke(cli, ["i2c", "write", "--address", "80", "--port", "COM9"])
    assert result.exit_code == 1
    assert "Supply --data or --file" in result.output
