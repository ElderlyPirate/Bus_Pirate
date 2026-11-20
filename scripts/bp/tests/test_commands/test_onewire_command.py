# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the ``bp onewire`` command services and Click wiring."""

from __future__ import annotations

from collections.abc import Generator
from contextlib import contextmanager
from pathlib import Path

import pytest
from click.testing import CliRunner

from bp.cli import cli
from bp.commands import onewire as onewire_module
from bp.errors import CommandError
from bp.protocol.onewire import OneWireRom, OneWireSession, OneWireSessionFactory, OneWireSpeed, onewire_crc8
from bp.types import ConnectionSettings


def make_rom(family: int, serial: bytes) -> OneWireRom:
    body = bytes([family]) + serial
    return OneWireRom(body + bytes([onewire_crc8(body)]))


class FakeOneWireSession:
    """Record 1-Wire operations without a serial port."""

    def __init__(self) -> None:
        self.calls: list[tuple[str, object]] = []
        self.devices: tuple[OneWireRom, ...] = (make_rom(0x28, b"\x01\x02\x03\x04\x05\x06"),)
        self.memory = b"\xaa\xbb"
        self.status = 0xA5

    def configure_speed(self, speed: OneWireSpeed) -> None:
        self.calls.append(("speed", speed))

    def configure_peripherals(
        self,
        *,
        power: bool = False,
        pullups: bool = True,
        aux: bool = False,
        cs: bool = False,
    ) -> None:
        self.calls.append(("peripherals", (power, pullups, aux, cs)))

    def reset(self) -> None:
        self.calls.append(("reset", None))

    def read_byte(self) -> int:
        self.calls.append(("read_byte", None))
        return 0x5A

    def read_bytes(self, count: int) -> bytes:
        self.calls.append(("read_bytes", count))
        return bytes(range(count))

    def write_bytes(self, data: bytes) -> None:
        self.calls.append(("write_bytes", data))

    def search_rom(self) -> tuple[OneWireRom, ...]:
        self.calls.append(("search", None))
        return self.devices

    def alarm_search(self) -> tuple[OneWireRom, ...]:
        self.calls.append(("alarm", None))
        return self.devices

    def read_rom(self) -> OneWireRom:
        self.calls.append(("read_rom", None))
        return self.devices[0]

    def match_rom(self, rom: OneWireRom) -> None:
        self.calls.append(("match", rom))

    def skip_rom(self) -> None:
        self.calls.append(("skip", None))

    def read_scratchpad(self, rom: OneWireRom | None, *, length: int = 9) -> bytes:
        self.calls.append(("read_scratchpad", (rom, length)))
        return bytes(range(length))

    def write_scratchpad(self, rom: OneWireRom | None, data: bytes) -> None:
        self.calls.append(("write_scratchpad", (rom, data)))

    def copy_scratchpad(self, rom: OneWireRom | None) -> None:
        self.calls.append(("copy_scratchpad", rom))

    def read_memory(self, rom: OneWireRom | None, *, address: int, length: int) -> bytes:
        self.calls.append(("read_memory", (rom, address, length)))
        return self.memory[:length]

    def read_status(self, rom: OneWireRom | None, *, address: int) -> int:
        self.calls.append(("read_status", (rom, address)))
        return self.status


def _factory(
    session: FakeOneWireSession,
    captured: list[ConnectionSettings] | None = None,
) -> OneWireSessionFactory:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[OneWireSession, None, None]:
        if captured is not None:
            captured.append(connection)
        yield session

    return factory


def test_parse_rom_accepts_plain_and_separated_hex() -> None:
    rom = make_rom(0x28, b"\x01\x02\x03\x04\x05\x06")
    assert onewire_module.parse_rom(rom.hex) == rom
    assert onewire_module.parse_rom(":".join(f"{value:02X}" for value in rom.data)) == rom
    assert onewire_module.parse_rom(None) is None


@pytest.mark.parametrize("value", ["", "28", "28:FF", "GGGGGGGGGGGGGGGG"])
def test_parse_rom_rejects_invalid_values(value: str) -> None:
    with pytest.raises(CommandError):
        onewire_module.parse_rom(value)


def test_scan_service_returns_devices() -> None:
    session = FakeOneWireSession()
    devices = onewire_module.scan_onewire(
        ConnectionSettings(port="COM9"),
        alarm=False,
        speed=OneWireSpeed.OVERDRIVE,
        power=True,
        pullups=True,
        session_factory=_factory(session),
    )
    assert devices == session.devices
    assert session.calls == [
        ("speed", OneWireSpeed.OVERDRIVE),
        ("peripherals", (True, True, False, False)),
        ("search", None),
    ]


def test_scratchpad_memory_and_status_services() -> None:
    session = FakeOneWireSession()
    connection = ConnectionSettings(port="COM9")
    rom = session.devices[0]
    factory = _factory(session)
    assert (
        onewire_module.read_onewire_scratchpad(
            connection, rom, length=3, speed=OneWireSpeed.STANDARD, power=False, pullups=True, session_factory=factory
        )
        == b"\x00\x01\x02"
    )
    onewire_module.write_onewire_scratchpad(
        connection, rom, b"\x01", speed=OneWireSpeed.STANDARD, power=False, pullups=True, session_factory=factory
    )
    onewire_module.copy_onewire_scratchpad(
        connection, rom, speed=OneWireSpeed.STANDARD, power=False, pullups=True, session_factory=factory
    )
    assert (
        onewire_module.read_onewire_memory(
            connection,
            rom,
            address=0x10,
            length=2,
            speed=OneWireSpeed.STANDARD,
            power=False,
            pullups=True,
            session_factory=factory,
        )
        == b"\xaa\xbb"
    )
    assert (
        onewire_module.read_onewire_status(
            connection,
            rom,
            address=0x10,
            speed=OneWireSpeed.STANDARD,
            power=False,
            pullups=True,
            session_factory=factory,
        )
        == 0xA5
    )
    assert ("read_scratchpad", (rom, 3)) in session.calls
    assert ("write_scratchpad", (rom, b"\x01")) in session.calls
    assert ("copy_scratchpad", rom) in session.calls
    assert ("read_memory", (rom, 0x10, 2)) in session.calls
    assert ("read_status", (rom, 0x10)) in session.calls


def test_send_service_resets_and_selects_device() -> None:
    session = FakeOneWireSession()
    rom = session.devices[0]
    onewire_module.send_onewire(
        ConnectionSettings(port="COM9"),
        b"\xbe",
        rom,
        skip_rom=False,
        speed=OneWireSpeed.STANDARD,
        power=False,
        pullups=True,
        session_factory=_factory(session),
    )
    assert ("reset", None) in session.calls
    assert ("match", rom) in session.calls
    assert ("write_bytes", b"\xbe") in session.calls


def test_send_service_rejects_rom_and_skip_together() -> None:
    session = FakeOneWireSession()
    with pytest.raises(CommandError, match="either --rom or --skip-rom"):
        onewire_module.send_onewire(
            ConnectionSettings(port="COM9"),
            b"\xbe",
            session.devices[0],
            skip_rom=True,
            speed=OneWireSpeed.STANDARD,
            power=False,
            pullups=True,
            session_factory=_factory(session),
        )


def test_cli_help_lists_onewire_commands() -> None:
    result = CliRunner().invoke(cli, ["onewire", "--help"])
    assert result.exit_code == 0
    for command in (
        "reset",
        "scan",
        "alarm-scan",
        "read-rom",
        "read-scratchpad",
        "write-scratchpad",
        "copy-scratchpad",
        "read-memory",
        "read-status",
        "send",
    ):
        assert command in result.output


def test_cli_scan_and_alarm_scan_print_devices(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeOneWireSession()
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(session))
    scan = CliRunner().invoke(cli, ["onewire", "scan", "--port", "COM9"])
    alarm = CliRunner().invoke(cli, ["onewire", "alarm-scan", "--port", "COM9"])
    assert scan.exit_code == 0
    assert session.devices[0].hex in scan.output
    assert "CRC OK" in scan.output
    assert alarm.exit_code == 0
    assert ("alarm", None) in session.calls


def test_cli_scan_reports_no_devices(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeOneWireSession()
    session.devices = ()
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(session))
    result = CliRunner().invoke(cli, ["onewire", "scan", "--port", "COM9"])
    assert result.exit_code == 0
    assert "No 1-Wire devices found." in result.output


def test_cli_read_rom_prints_crc_status(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeOneWireSession()
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(session))
    result = CliRunner().invoke(cli, ["onewire", "read-rom", "--port", "COM9"])
    assert result.exit_code == 0
    assert session.devices[0].hex in result.output
    assert "CRC OK" in result.output


def test_cli_read_and_write_scratchpad(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    session = FakeOneWireSession()
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(session))
    output = tmp_path / "scratch.txt"
    read = CliRunner().invoke(
        cli,
        ["onewire", "read-scratchpad", "--length", "3", "--file", str(output), "--port", "COM9"],
    )
    write = CliRunner().invoke(
        cli,
        ["onewire", "write-scratchpad", "--data", "01 02", "--port", "COM9"],
    )
    assert read.exit_code == 0
    assert output.read_text(encoding="ascii") == "00 01 02\n"
    assert write.exit_code == 0
    assert ("write_scratchpad", (None, b"\x01\x02")) in session.calls


def test_cli_scratchpad_read_without_file_prints_hex(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(FakeOneWireSession()))
    result = CliRunner().invoke(cli, ["onewire", "read-scratchpad", "--length", "3", "--port", "COM9"])
    assert result.exit_code == 0
    assert "00 01 02" in result.output


def test_cli_copy_scratchpad(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeOneWireSession()
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(session))
    result = CliRunner().invoke(cli, ["onewire", "copy-scratchpad", "--port", "COM9"])
    assert result.exit_code == 0
    assert ("copy_scratchpad", None) in session.calls


def test_cli_read_memory_and_status(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    session = FakeOneWireSession()
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(session))
    output = tmp_path / "memory.txt"
    memory = CliRunner().invoke(
        cli,
        ["onewire", "read-memory", "--address", "16", "--length", "2", "--file", str(output), "--port", "COM9"],
    )
    status = CliRunner().invoke(
        cli,
        ["onewire", "read-status", "--address", "16", "--port", "COM9"],
    )
    assert memory.exit_code == 0
    assert output.read_text(encoding="ascii") == "AA BB\n"
    assert status.exit_code == 0
    assert "Status: 0xA5" in status.output


def test_cli_send_with_match_and_skip(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeOneWireSession()
    rom = session.devices[0]
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(session))
    match = CliRunner().invoke(
        cli,
        ["onewire", "send", "--rom", rom.hex, "--data", "BE", "--port", "COM9"],
    )
    skip = CliRunner().invoke(
        cli,
        ["onewire", "send", "--skip-rom", "--data", "CC", "--port", "COM9"],
    )
    assert match.exit_code == 0
    assert ("match", rom) in session.calls
    assert skip.exit_code == 0
    assert ("skip", None) in session.calls


def test_cli_send_rejects_rom_and_skip(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeOneWireSession()
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(session))
    result = CliRunner().invoke(
        cli,
        ["onewire", "send", "--rom", session.devices[0].hex, "--skip-rom", "--data", "CC", "--port", "COM9"],
    )
    assert result.exit_code == 1
    assert "either --rom or --skip-rom" in result.output


def test_cli_reset(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeOneWireSession()
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(session))
    result = CliRunner().invoke(cli, ["onewire", "reset", "--port", "COM9"])
    assert result.exit_code == 0
    assert "1-Wire bus reset complete." in result.output
    assert ("reset", None) in session.calls


def test_cli_rejects_invalid_rom(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(onewire_module, "open_onewire_session", _factory(FakeOneWireSession()))
    result = CliRunner().invoke(
        cli,
        ["onewire", "read-scratchpad", "--rom", "not-a-rom", "--port", "COM9"],
    )
    assert result.exit_code == 1
    assert "16 hex digits" in result.output


def test_cli_write_scratchpad_rejects_missing_data() -> None:
    result = CliRunner().invoke(cli, ["onewire", "write-scratchpad", "--port", "COM9"])
    assert result.exit_code == 1
    assert "Supply --data or --file" in result.output
