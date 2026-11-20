# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the ``bp raw-wire`` command services and Click wiring."""

from __future__ import annotations

from collections.abc import Generator
from contextlib import contextmanager

import pytest
from click.testing import CliRunner

from bp.cli import cli
from bp.commands import raw_wire as raw_wire_module
from bp.protocol.raw_wire import RawWireConfig, RawWireSession, RawWireSessionFactory, RawWireSpeed
from bp.types import ConnectionSettings


class FakeRawWireSession:
    """Record raw-wire operations without a serial port."""

    def __init__(self, *, byte_value: int = 0x5A, bit_value: bool = True) -> None:
        self.byte_value = byte_value
        self.bit_value = bit_value
        self.calls: list[tuple[str, object]] = []
        self.read_data = b"\x11\x22"

    def configure(self, config: RawWireConfig) -> None:
        self.calls.append(("configure", config))

    def configure_peripherals(
        self,
        *,
        power: bool = False,
        pullups: bool = False,
        aux: bool = False,
        cs: bool = False,
    ) -> None:
        self.calls.append(("peripherals", (power, pullups, aux, cs)))

    def set_speed(self, speed: RawWireSpeed) -> None:
        self.calls.append(("speed", speed))

    def set_cs(self, *, low: bool) -> None:
        self.calls.append(("cs", low))

    def start(self) -> None:
        self.calls.append(("start", None))

    def stop(self) -> None:
        self.calls.append(("stop", None))

    def clock_tick(self) -> None:
        self.calls.append(("clock_tick", None))

    def clock_high(self) -> None:
        self.calls.append(("clock_high", None))

    def clock_low(self) -> None:
        self.calls.append(("clock_low", None))

    def data_high(self) -> None:
        self.calls.append(("data_high", None))

    def data_low(self) -> None:
        self.calls.append(("data_low", None))

    def read_byte(self) -> int:
        return self.byte_value

    def read_bit(self) -> bool:
        return self.bit_value

    def peek(self) -> bool:
        return self.bit_value

    def bulk_clock(self, ticks: int) -> None:
        self.calls.append(("bulk_clock", ticks))

    def write_bit(self, high: bool) -> None:
        self.calls.append(("write_bit", high))

    def write_bits(self, value: int, count: int, *, lsb_first: bool = False) -> None:
        self.calls.append(("write_bits", (value, count, lsb_first)))

    def write_byte(self, value: int) -> None:
        self.calls.append(("write_byte", value))

    def write_bytes(self, data: bytes) -> None:
        self.calls.append(("write_bytes", data))

    def read_bytes(self, count: int) -> bytes:
        self.calls.append(("read_bytes", count))
        return self.read_data[:count]


def _factory(session: FakeRawWireSession) -> RawWireSessionFactory:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[RawWireSession, None, None]:
        del connection
        yield session

    return factory


def test_configure_service_applies_config_speed_and_peripherals() -> None:
    session = FakeRawWireSession()
    config = RawWireConfig(output_3v3=True, three_wire=True, lsb_first=True)
    raw_wire_module.configure_raw_wire(
        ConnectionSettings(port="COM9"),
        config,
        RawWireSpeed.KHZ_100,
        power=True,
        pullups=True,
        session_factory=_factory(session),
    )
    assert session.calls == [
        ("configure", config),
        ("speed", RawWireSpeed.KHZ_100),
        ("peripherals", (True, True, False, False)),
    ]


def test_write_service_frames_with_cs_start_and_stop() -> None:
    session = FakeRawWireSession()
    raw_wire_module.write_raw_wire(
        ConnectionSettings(port="COM9"),
        b"\xaa",
        RawWireConfig(),
        RawWireSpeed.KHZ_50,
        power=False,
        pullups=False,
        use_cs=True,
        start=True,
        stop=True,
        session_factory=_factory(session),
    )
    assert session.calls == [
        ("configure", RawWireConfig()),
        ("speed", RawWireSpeed.KHZ_50),
        ("cs", True),
        ("start", None),
        ("write_bytes", b"\xaa"),
        ("stop", None),
        ("cs", False),
    ]


def test_read_service_returns_bytes_and_frames_cs() -> None:
    session = FakeRawWireSession()
    data = raw_wire_module.read_raw_wire(
        ConnectionSettings(port="COM9"),
        2,
        RawWireConfig(),
        RawWireSpeed.KHZ_50,
        power=False,
        pullups=False,
        use_cs=True,
        start=False,
        stop=False,
        session_factory=_factory(session),
    )
    assert data == b"\x11\x22"
    assert session.calls == [
        ("configure", RawWireConfig()),
        ("speed", RawWireSpeed.KHZ_50),
        ("cs", True),
        ("read_bytes", 2),
        ("cs", False),
    ]


def test_bit_peek_and_clock_services() -> None:
    session = FakeRawWireSession()
    connection = ConnectionSettings(port="COM9")
    config = RawWireConfig()
    factory = _factory(session)
    assert raw_wire_module.read_raw_wire_bit(
        connection, config, RawWireSpeed.KHZ_50, power=False, pullups=False, session_factory=factory
    )
    assert raw_wire_module.peek_raw_wire(
        connection, config, RawWireSpeed.KHZ_50, power=False, pullups=False, session_factory=factory
    )
    raw_wire_module.clock_raw_wire(
        connection,
        3,
        config,
        RawWireSpeed.KHZ_50,
        power=False,
        pullups=False,
        session_factory=factory,
    )
    assert session.calls[-1] == ("bulk_clock", 3)


def test_cli_help_lists_raw_wire_commands() -> None:
    result = CliRunner().invoke(cli, ["raw-wire", "--help"])
    assert result.exit_code == 0
    for command in ("config", "write", "read", "bit", "peek", "clock"):
        assert command in result.output


def test_cli_config_prints_selected_mode(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeRawWireSession()
    monkeypatch.setattr(raw_wire_module, "open_raw_wire_session", _factory(session))
    result = CliRunner().invoke(
        cli,
        [
            "raw-wire",
            "config",
            "--wires",
            "2",
            "--output",
            "open-drain",
            "--lsb-first",
            "--bus-speed",
            "400k",
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 0
    assert "2 wires, open-drain, LSB first, 400k" in result.output


def test_cli_write_uses_cs_for_three_wire_and_prints_payload(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeRawWireSession()
    monkeypatch.setattr(raw_wire_module, "open_raw_wire_session", _factory(session))
    result = CliRunner().invoke(
        cli,
        ["raw-wire", "write", "--data", "AA BB", "--wires", "3", "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert "AA BB" in result.output
    assert ("cs", True) in session.calls
    assert ("write_bytes", b"\xaa\xbb") in session.calls
    assert ("cs", False) in session.calls


def test_cli_write_two_wire_defaults_to_no_cs(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeRawWireSession()
    monkeypatch.setattr(raw_wire_module, "open_raw_wire_session", _factory(session))
    result = CliRunner().invoke(
        cli,
        ["raw-wire", "write", "--data", "55", "--wires", "2", "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert ("cs", True) not in session.calls


def test_cli_write_can_force_cs_and_start_stop(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeRawWireSession()
    monkeypatch.setattr(raw_wire_module, "open_raw_wire_session", _factory(session))
    result = CliRunner().invoke(
        cli,
        [
            "raw-wire",
            "write",
            "--data",
            "55",
            "--wires",
            "2",
            "--cs",
            "--start",
            "--stop",
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 0
    assert ("start", None) in session.calls
    assert ("stop", None) in session.calls
    assert ("cs", True) in session.calls


def test_cli_read_bit_and_clock(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeRawWireSession(bit_value=False)
    monkeypatch.setattr(raw_wire_module, "open_raw_wire_session", _factory(session))
    read = CliRunner().invoke(cli, ["raw-wire", "read", "--count", "2", "--port", "COM9"])
    bit = CliRunner().invoke(cli, ["raw-wire", "bit", "--port", "COM9"])
    peek = CliRunner().invoke(cli, ["raw-wire", "peek", "--port", "COM9"])
    clock = CliRunner().invoke(cli, ["raw-wire", "clock", "--ticks", "3", "--port", "COM9"])
    assert read.exit_code == 0
    assert "11 22" in read.output
    assert bit.exit_code == 0
    assert "Bit: 0" in bit.output
    assert peek.exit_code == 0
    assert "Pin: 0" in peek.output
    assert clock.exit_code == 0
    assert "3 clock tick" in clock.output


def test_cli_rejects_bad_hex_and_missing_data() -> None:
    bad = CliRunner().invoke(cli, ["raw-wire", "write", "--data", "GG", "--port", "COM9"])
    missing = CliRunner().invoke(cli, ["raw-wire", "write", "--port", "COM9"])
    assert bad.exit_code == 1
    assert "Invalid hex token" in bad.output
    assert missing.exit_code == 1
    assert "Supply --data or --file" in missing.output
