"""Tests for the ``bp spi`` command services and Click wiring."""

from __future__ import annotations

from collections.abc import Generator
from contextlib import contextmanager
from pathlib import Path

import pytest
from click.testing import CliRunner

from bp.cli import cli
from bp.commands import spi as spi_module
from bp.commands.spi import SpiSessionFactory
from bp.protocol.spi import SpiConfig, SpiSession, SpiSpeed
from bp.types import ConnectionSettings


class FakeSpiSession:
    """Record SPI operations without a serial port."""

    def __init__(self, received: bytes = b"\x00\xef\x40\x18") -> None:
        self.received = received
        self.calls: list[tuple[str, object]] = []
        self.sent = b""

    def configure_speed(self, speed: SpiSpeed) -> None:
        self.calls.append(("speed", speed))

    def configure_spi(
        self,
        *,
        output_3v3: bool,
        clock_idle_high: bool,
        active_to_idle: bool,
        sample_end: bool,
    ) -> None:
        self.calls.append(("config", (output_3v3, clock_idle_high, active_to_idle, sample_end)))

    def configure_peripherals(
        self,
        *,
        power: bool = False,
        pullups: bool = False,
        aux: bool = False,
        cs: bool = False,
    ) -> None:
        self.calls.append(("peripherals", (power, pullups, aux, cs)))

    def set_cs(self, *, low: bool) -> None:
        self.calls.append(("cs", low))

    def transfer(self, value: int) -> int:
        self.calls.append(("transfer", value))
        return 0

    def bulk_transfer(self, data: bytes) -> bytes:
        self.calls.append(("bulk", data))
        return self.received[: len(data)]

    def write_bytes(self, data: bytes) -> bytes:
        self.calls.append(("write", data))
        self.sent += data
        return self.received[: len(data)]

    def read_bytes(self, count: int) -> bytes:
        self.calls.append(("read", count))
        return bytes(range(count))


def _factory(session: FakeSpiSession, captured: list[ConnectionSettings] | None = None) -> SpiSessionFactory:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[SpiSession, None, None]:
        if captured is not None:
            captured.append(connection)
        yield session

    return factory


def test_configure_service_applies_speed_mode_and_power() -> None:
    session = FakeSpiSession()
    spi_module.configure_spi(
        ConnectionSettings(port="COM9"),
        SpiSpeed.MHZ_4,
        SpiConfig(output_3v3=False, clock_idle_high=True, active_to_idle=False, sample_end=True),
        power=True,
        pullups=True,
        session_factory=_factory(session),
    )
    assert session.calls == [
        ("speed", SpiSpeed.MHZ_4),
        ("config", (False, True, False, True)),
        ("peripherals", (True, True, False, False)),
    ]


def test_chip_id_service_asserts_cs_and_reads_four_bytes() -> None:
    session = FakeSpiSession(b"\x00\xef\x40\x18")
    response = spi_module.read_spi_chip_id(
        ConnectionSettings(port="COM9"),
        SpiSpeed.MHZ_1,
        SpiConfig(),
        power=False,
        pullups=False,
        session_factory=_factory(session),
    )
    assert response == b"\x00\xef\x40\x18"
    assert ("bulk", b"\x9f\xff\xff\xff") in session.calls
    assert ("cs", True) in session.calls
    assert ("cs", False) in session.calls
    assert session.calls.index(("cs", True)) < session.calls.index(("bulk", b"\x9f\xff\xff\xff"))


def test_transfer_service_uses_write_bytes_under_cs() -> None:
    session = FakeSpiSession(b"\x00\x00\x00\x00")
    response = spi_module.transfer_spi(
        ConnectionSettings(port="COM9"),
        b"\x9f\x00",
        SpiSpeed.MHZ_1,
        SpiConfig(),
        power=False,
        pullups=False,
        session_factory=_factory(session),
    )
    assert response == b"\x00\x00"
    assert ("write", b"\x9f\x00") in session.calls
    assert ("cs", True) in session.calls
    assert ("cs", False) in session.calls


def test_read_service_sends_prefix_then_reads_requested_length() -> None:
    session = FakeSpiSession()
    data = spi_module.read_spi(
        ConnectionSettings(port="COM9"),
        opcode=0x0B,
        address=0x123456,
        address_width=3,
        dummy_bytes=1,
        length=4,
        speed=SpiSpeed.MHZ_1,
        config=SpiConfig(),
        power=False,
        pullups=False,
        session_factory=_factory(session),
    )
    assert data == b"\x00\x01\x02\x03"
    assert ("write", b"\x0b\x12\x34\x56\x00") in session.calls
    assert ("read", 4) in session.calls


def test_write_service_sends_optional_prefix_and_data() -> None:
    session = FakeSpiSession()
    spi_module.write_spi(
        ConnectionSettings(port="COM9"),
        opcode=0x02,
        address=0x10,
        address_width=3,
        data=b"\xaa\xbb",
        speed=SpiSpeed.MHZ_1,
        config=SpiConfig(),
        power=False,
        pullups=False,
        session_factory=_factory(session),
    )
    assert ("write", b"\x02\x00\x00\x10") in session.calls
    assert ("write", b"\xaa\xbb") in session.calls


def test_write_service_without_opcode_sends_only_data() -> None:
    session = FakeSpiSession()
    spi_module.write_spi(
        ConnectionSettings(port="COM9"),
        opcode=None,
        address=0,
        address_width=3,
        data=b"\xaa",
        speed=SpiSpeed.MHZ_1,
        config=SpiConfig(),
        power=False,
        pullups=False,
        session_factory=_factory(session),
    )
    assert ("write", b"\xaa") in session.calls
    assert not any(call[0] == "write" and call[1] == b"\x00\x00\x00" for call in session.calls)


@pytest.mark.parametrize("address_width", [5, -1])
def test_read_service_rejects_invalid_address_width(address_width: int) -> None:
    with pytest.raises(ValueError, match="address width"):
        spi_module.read_spi(
            ConnectionSettings(port="COM9"),
            opcode=0,
            address=0,
            address_width=address_width,
            dummy_bytes=0,
            length=1,
            speed=SpiSpeed.MHZ_1,
            config=SpiConfig(),
            power=False,
            pullups=False,
            session_factory=_factory(FakeSpiSession()),
        )


def test_read_service_rejects_address_that_does_not_fit() -> None:
    with pytest.raises(ValueError, match="does not fit"):
        spi_module.read_spi(
            ConnectionSettings(port="COM9"),
            opcode=0,
            address=0x100,
            address_width=1,
            dummy_bytes=0,
            length=1,
            speed=SpiSpeed.MHZ_1,
            config=SpiConfig(),
            power=False,
            pullups=False,
            session_factory=_factory(FakeSpiSession()),
        )


def test_read_service_rejects_negative_dummy_and_zero_length() -> None:
    connection = ConnectionSettings(port="COM9")
    with pytest.raises(ValueError, match="dummy byte"):
        spi_module.read_spi(
            connection,
            opcode=0,
            address=0,
            address_width=0,
            dummy_bytes=-1,
            length=1,
            speed=SpiSpeed.MHZ_1,
            config=SpiConfig(),
            power=False,
            pullups=False,
            session_factory=_factory(FakeSpiSession()),
        )
    with pytest.raises(ValueError, match="length must be positive"):
        spi_module.read_spi(
            connection,
            opcode=0,
            address=0,
            address_width=0,
            dummy_bytes=0,
            length=0,
            speed=SpiSpeed.MHZ_1,
            config=SpiConfig(),
            power=False,
            pullups=False,
            session_factory=_factory(FakeSpiSession()),
        )


def test_cli_help_lists_spi_commands() -> None:
    result = CliRunner().invoke(cli, ["spi", "--help"])
    assert result.exit_code == 0
    for command in ("config", "chip-id", "transfer", "read", "write"):
        assert command in result.output


def test_cli_config_prints_settings(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeSpiSession()
    captured: list[ConnectionSettings] = []
    monkeypatch.setattr(spi_module, "open_spi_session", _factory(session, captured))
    result = CliRunner().invoke(
        cli,
        ["spi", "config", "--bus-speed", "2.6m", "--output", "open-drain", "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert "2.6m, open-drain" in result.output
    assert captured == [ConnectionSettings(port="COM9")]


def test_cli_chip_id_prints_response_and_jedec(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(spi_module, "open_spi_session", _factory(FakeSpiSession(b"\x00\xef\x40\x18")))
    result = CliRunner().invoke(cli, ["spi", "chip-id", "--port", "COM9"])
    assert result.exit_code == 0
    assert "Chip ID response: 00 EF 40 18" in result.output
    assert "JEDEC ID: EF 40 18" in result.output


def test_cli_transfer_prints_received(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(spi_module, "open_spi_session", _factory(FakeSpiSession(b"\x00\x00")))
    result = CliRunner().invoke(cli, ["spi", "transfer", "--data", "9F 00", "--port", "COM9"])
    assert result.exit_code == 0
    assert "Transferred 2 byte(s)." in result.output
    assert "Received: 00 00" in result.output


def test_cli_read_writes_binary_file(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    monkeypatch.setattr(spi_module, "open_spi_session", _factory(FakeSpiSession()))
    output = tmp_path / "dump.bin"
    result = CliRunner().invoke(
        cli,
        [
            "spi",
            "read",
            "--opcode",
            "11",
            "--address",
            "0",
            "--length",
            "4",
            "--file",
            str(output),
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 0
    assert output.read_bytes() == b"\x00\x01\x02\x03"
    assert "Read 4 bytes" in result.output
    assert "First bytes: 00 01 02 03" in result.output


def test_cli_write_reads_binary_file(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    session = FakeSpiSession()
    monkeypatch.setattr(spi_module, "open_spi_session", _factory(session))
    source = tmp_path / "image.bin"
    source.write_bytes(b"\xaa\xbb")
    result = CliRunner().invoke(
        cli,
        ["spi", "write", "--opcode", "2", "--address", "16", "--file", str(source), "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert ("write", b"\x02\x00\x00\x10") in session.calls
    assert ("write", b"\xaa\xbb") in session.calls


def test_cli_write_rejects_empty_file(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    monkeypatch.setattr(spi_module, "open_spi_session", _factory(FakeSpiSession()))
    source = tmp_path / "empty.bin"
    source.write_bytes(b"")
    result = CliRunner().invoke(cli, ["spi", "write", "--file", str(source), "--port", "COM9"])
    assert result.exit_code == 1
    assert "is empty" in result.output


def test_cli_read_rejects_address_that_does_not_fit(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    monkeypatch.setattr(spi_module, "open_spi_session", _factory(FakeSpiSession()))
    result = CliRunner().invoke(
        cli,
        [
            "spi",
            "read",
            "--opcode",
            "3",
            "--address",
            "256",
            "--address-width",
            "1",
            "--length",
            "1",
            "--file",
            str(tmp_path / "x.bin"),
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 1
    assert "does not fit" in result.output
