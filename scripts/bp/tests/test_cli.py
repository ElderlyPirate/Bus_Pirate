"""CLI wiring and end-to-end command exit-code tests without hardware."""

from __future__ import annotations

from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from pathlib import Path

import pytest
from click.testing import CliRunner

from bp.cli import cli
from bp.commands import eeprom as eeprom_module, version as version_module, xsvf as xsvf_module
from bp.errors import EepromVerifyError, ProtocolError, TransportError, XsvfHostError, XsvfPlayerError
from bp.protocol.i2c import EepromSession, EepromWriteReport
from bp.protocol.xsvf import ChainScan, XsvfPlayReport, XsvfResult, XsvfSession
from bp.transport import Transport
from bp.types import ConnectionSettings, EepromLayout
from conftest import ScriptedTransport

BANNER = (
    b"\r\nRESET\r\nBus Pirate v3.6\r\nElderlyPirate Firmware v8.0\r\n"
    b"DEVID:0x044F REVID:0x0004\r\nhttp://github.com/ElderlyPirate\r\nHiZ>"
)


class FakeEepromSession:
    """Minimal EEPROM session for CLI wiring tests."""

    def __init__(
        self,
        *,
        read_data: bytes = b"\xa5",
        read_error: ProtocolError | None = None,
        verify_error: EepromVerifyError | None = None,
    ) -> None:
        self.read_data = read_data
        self.read_error = read_error
        self.verify_error = verify_error
        self.read_calls: list[tuple[EepromLayout, int]] = []
        self.verify_calls: list[tuple[EepromLayout, bytes]] = []

    def read_eeprom(self, layout: EepromLayout, length: int) -> bytes:
        self.read_calls.append((layout, length))
        if self.read_error is not None:
            raise self.read_error
        return self.read_data

    def write_eeprom(self, layout: EepromLayout, data: bytes) -> EepromWriteReport:
        return EepromWriteReport(bytes_written=len(data), retries=0)

    def verify_eeprom(self, layout: EepromLayout, expected: bytes) -> None:
        self.verify_calls.append((layout, expected))
        if self.verify_error is not None:
            raise self.verify_error


def _eeprom_factory(
    session: FakeEepromSession,
    captured: list[ConnectionSettings] | None = None,
) -> Callable[[ConnectionSettings], AbstractContextManager[EepromSession]]:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[EepromSession, None, None]:
        if captured is not None:
            captured.append(connection)
        yield session

    return factory


def _version_factory(transport: ScriptedTransport) -> Callable[[ConnectionSettings], AbstractContextManager[Transport]]:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[Transport, None, None]:
        del connection
        yield transport

    return factory


class FakeXsvfCliSession:
    """Minimal XSVF session for CLI wiring tests."""

    def __init__(
        self,
        *,
        scan: ChainScan | None = None,
        play_report: XsvfPlayReport | None = None,
        play_error: XsvfPlayerError | None = None,
        scan_error: XsvfHostError | None = None,
    ) -> None:
        self.scan = scan or ChainScan(count=0, payload=b"")
        self.play_report = play_report or XsvfPlayReport(bytes_sent=0, chunks=0, result=XsvfResult.NONE)
        self.play_error = play_error
        self.scan_error = scan_error
        self.calls: list[tuple[str, bytes | None]] = []

    def reset_chain(self) -> None:
        self.calls.append(("reset", None))

    def scan_chain(self) -> ChainScan:
        self.calls.append(("scan", None))
        if self.scan_error is not None:
            raise self.scan_error
        return self.scan

    def play(self, data: bytes) -> XsvfPlayReport:
        self.calls.append(("play", data))
        if self.play_error is not None:
            raise self.play_error
        return self.play_report


def _xsvf_factory(
    session: FakeXsvfCliSession,
    captured: list[ConnectionSettings] | None = None,
) -> Callable[[ConnectionSettings], AbstractContextManager[XsvfSession]]:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[XsvfSession, None, None]:
        if captured is not None:
            captured.append(connection)
        yield session

    return factory


def test_help_lists_frontend_commands() -> None:
    result = CliRunner().invoke(cli, ["--help"])
    assert result.exit_code == 0
    assert "version" in result.output
    assert "eeprom" in result.output
    assert "lcd-test" in result.output
    assert "xsvf" in result.output


def test_version_prints_banner_fields(monkeypatch: pytest.MonkeyPatch) -> None:
    transport = ScriptedTransport([BANNER])
    monkeypatch.setattr(version_module, "open_transport", _version_factory(transport))
    result = CliRunner().invoke(cli, ["version", "--port", "COM9"])
    assert result.exit_code == 0
    assert "Hardware type:    Bus Pirate v3.6" in result.output
    assert "Firmware version: ElderlyPirate Firmware v8.0" in result.output
    assert "Hardware version: DEVID:0x044F REVID:0x0004" in result.output
    assert "Update URL:       http://github.com/ElderlyPirate" in result.output


def test_version_protocol_failure_exits_one(monkeypatch: pytest.MonkeyPatch) -> None:
    transport = ScriptedTransport()
    monkeypatch.setattr(version_module, "open_transport", _version_factory(transport))
    result = CliRunner().invoke(cli, ["version", "--port", "COM9"])
    assert result.exit_code == 1
    assert "no version banner" in result.output


def test_eeprom_read_uses_config_connection_and_writes_file(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    config = tmp_path / "bp.yaml"
    config.write_text("port: COM7\nspeed: 57600\n", encoding="utf-8")
    output = tmp_path / "dump.txt"
    session = FakeEepromSession(read_data=b"\xaa\xbb")
    captured: list[ConnectionSettings] = []
    monkeypatch.setattr(eeprom_module, "open_eeprom_session", _eeprom_factory(session, captured))

    result = CliRunner().invoke(
        cli,
        [
            "--config",
            str(config),
            "eeprom",
            "read",
            "--chip",
            "0",
            "--length",
            "2",
            "--file",
            str(output),
        ],
    )

    assert result.exit_code == 0
    assert "Read 2 bytes from 0x0000" in result.output
    assert output.read_text(encoding="ascii") == "AA BB\n"
    assert captured == [ConnectionSettings(port="COM7", speed=57_600)]
    assert session.read_calls[0][1] == 2


def test_eeprom_write_prints_retry_summary(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    source = tmp_path / "image.txt"
    source.write_text("00 FF\n", encoding="ascii")
    session = FakeEepromSession()
    monkeypatch.setattr(eeprom_module, "open_eeprom_session", _eeprom_factory(session))
    result = CliRunner().invoke(
        cli,
        ["eeprom", "write", "--chip", "3", "--file", str(source), "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert "Wrote 2 bytes at 0x0000 with 0 write retries." in result.output


def test_eeprom_verify_success(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    source = tmp_path / "expected.txt"
    source.write_text("AA\n", encoding="ascii")
    session = FakeEepromSession()
    monkeypatch.setattr(eeprom_module, "open_eeprom_session", _eeprom_factory(session))
    result = CliRunner().invoke(
        cli,
        ["eeprom", "verify", "--chip", "0", "--file", str(source), "--port", "COM9"],
    )
    assert result.exit_code == 0
    assert "Verified 1 bytes at 0x0000" in result.output


def test_eeprom_verify_mismatch_exits_one(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    source = tmp_path / "expected.txt"
    source.write_text("AA\n", encoding="ascii")
    session = FakeEepromSession(verify_error=EepromVerifyError(0, 0xAA, 0x55))
    monkeypatch.setattr(eeprom_module, "open_eeprom_session", _eeprom_factory(session))
    result = CliRunner().invoke(
        cli,
        ["eeprom", "verify", "--chip", "0", "--file", str(source), "--port", "COM9"],
    )
    assert result.exit_code == 1
    assert "expected 0xAA, read 0x55" in result.output


def test_eeprom_read_protocol_failure_exits_one(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    session = FakeEepromSession(read_error=ProtocolError("EEPROM NACK"))
    monkeypatch.setattr(eeprom_module, "open_eeprom_session", _eeprom_factory(session))
    result = CliRunner().invoke(
        cli,
        [
            "eeprom",
            "read",
            "--chip",
            "0",
            "--length",
            "1",
            "--file",
            str(tmp_path / "dump.txt"),
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 1
    assert "EEPROM NACK" in result.output


def test_eeprom_bad_port_exits_one(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    def failing_factory(connection: ConnectionSettings) -> AbstractContextManager[EepromSession]:
        raise TransportError(f"Could not open {connection.port}")

    monkeypatch.setattr(eeprom_module, "open_eeprom_session", failing_factory)
    result = CliRunner().invoke(
        cli,
        [
            "eeprom",
            "read",
            "--chip",
            "0",
            "--length",
            "1",
            "--file",
            str(tmp_path / "dump.txt"),
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 1
    assert "Could not open COM9" in result.output


def test_eeprom_requires_port(tmp_path: Path) -> None:
    result = CliRunner().invoke(
        cli,
        [
            "eeprom",
            "read",
            "--chip",
            "0",
            "--length",
            "1",
            "--file",
            str(tmp_path / "dump.txt"),
        ],
    )
    assert result.exit_code == 1
    assert "No serial port specified" in result.output


def test_eeprom_rejects_invalid_chip_address(tmp_path: Path) -> None:
    result = CliRunner().invoke(
        cli,
        [
            "eeprom",
            "read",
            "--chip",
            "8",
            "--length",
            "1",
            "--file",
            str(tmp_path / "dump.txt"),
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 2
    assert "not in the range" in result.output


def test_eeprom_rejects_invalid_address_width(tmp_path: Path) -> None:
    result = CliRunner().invoke(
        cli,
        [
            "eeprom",
            "read",
            "--chip",
            "0",
            "--address-width",
            "3",
            "--length",
            "1",
            "--file",
            str(tmp_path / "dump.txt"),
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 2
    assert "Invalid value for '--address-width'" in result.output


def test_eeprom_rejects_one_byte_address_out_of_range(tmp_path: Path) -> None:
    result = CliRunner().invoke(
        cli,
        [
            "eeprom",
            "read",
            "--chip",
            "0",
            "--address-width",
            "1",
            "--memory-address",
            "2048",
            "--length",
            "1",
            "--file",
            str(tmp_path / "dump.txt"),
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 2
    assert "0x7FF" in result.output


def test_eeprom_read_requires_length(tmp_path: Path) -> None:
    result = CliRunner().invoke(
        cli,
        [
            "eeprom",
            "read",
            "--chip",
            "0",
            "--file",
            str(tmp_path / "dump.txt"),
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 2
    assert "Missing option '--length'" in result.output


def test_eeprom_read_rejects_span_before_session(tmp_path: Path) -> None:
    result = CliRunner().invoke(
        cli,
        [
            "eeprom",
            "read",
            "--chip",
            "0",
            "--address-width",
            "1",
            "--memory-address",
            "2047",
            "--length",
            "2",
            "--file",
            str(tmp_path / "dump.txt"),
            "--port",
            "COM9",
        ],
    )
    assert result.exit_code == 1
    assert "extends past" in result.output


def test_eeprom_write_rejects_truncated_hex(tmp_path: Path) -> None:
    source = tmp_path / "bad.txt"
    source.write_text("AA 0x\n", encoding="ascii")
    result = CliRunner().invoke(
        cli,
        ["eeprom", "write", "--chip", "0", "--file", str(source), "--port", "COM9"],
    )
    assert result.exit_code == 1
    assert "Invalid hex token '0x'" in result.output


def test_eeprom_write_rejects_missing_file() -> None:
    result = CliRunner().invoke(
        cli,
        ["eeprom", "write", "--chip", "0", "--file", "missing.txt", "--port", "COM9"],
    )
    assert result.exit_code == 2
    assert "does not exist" in result.output


def test_invalid_config_exits_one(tmp_path: Path) -> None:
    config = tmp_path / "bp.yaml"
    config.write_text("unknown_key: 1\n", encoding="utf-8")
    result = CliRunner().invoke(cli, ["--config", str(config), "version"])
    assert result.exit_code == 1
    assert "Invalid configuration" in result.output


def test_missing_config_file_is_usage_error(tmp_path: Path) -> None:
    result = CliRunner().invoke(cli, ["--config", str(tmp_path / "missing.yaml"), "version"])
    assert result.exit_code == 2
    assert "does not exist" in result.output


def test_xsvf_file_play_prints_success(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    source = tmp_path / "design.xsvf"
    source.write_bytes(b"\x00\x01")
    session = FakeXsvfCliSession(play_report=XsvfPlayReport(bytes_sent=2, chunks=1, result=XsvfResult.NONE))
    monkeypatch.setattr(xsvf_module, "open_xsvf_session", _xsvf_factory(session))

    result = CliRunner().invoke(cli, ["xsvf", "--file", str(source), "--port", "COM9"])

    assert result.exit_code == 0
    assert "File is 2 bytes." in result.output
    assert "Sent 2 bytes in 1 chunk(s)." in result.output
    assert "End of operation reply: XSVF_ERROR_NONE" in result.output
    assert "Success!" in result.output
    assert session.calls == [("play", b"\x00\x01")]


def test_xsvf_scan_prints_count_and_idcode_bytes(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeXsvfCliSession(scan=ChainScan(count=4, payload=b"\x93\x40\x60\x59"))
    monkeypatch.setattr(xsvf_module, "open_xsvf_session", _xsvf_factory(session))

    result = CliRunner().invoke(cli, ["xsvf", "--scan", "--port", "COM9"])

    assert result.exit_code == 0
    assert "Chain Scan Result: 04 93 40 60 59" in result.output
    assert "Thank you for playing! :-)" in result.output
    assert session.calls == [("scan", None)]


def test_xsvf_reset_uses_short_native_option(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeXsvfCliSession()
    monkeypatch.setattr(xsvf_module, "open_xsvf_session", _xsvf_factory(session))

    result = CliRunner().invoke(cli, ["xsvf", "-r", "-p", "COM9"])

    assert result.exit_code == 0
    assert "Performing Reset.. Done" in result.output
    assert session.calls == [("reset", None)]


def test_xsvf_reset_scan_file_runs_in_native_order(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    source = tmp_path / "design.xsvf"
    source.write_bytes(b"\xaa")
    session = FakeXsvfCliSession(
        scan=ChainScan(count=0, payload=b""),
        play_report=XsvfPlayReport(bytes_sent=1, chunks=1, result=XsvfResult.NONE),
    )
    monkeypatch.setattr(xsvf_module, "open_xsvf_session", _xsvf_factory(session))

    result = CliRunner().invoke(
        cli,
        ["xsvf", "-p", "COM9", "-f", str(source), "-r", "-x"],
    )

    assert result.exit_code == 0
    assert session.calls == [("reset", None), ("scan", None), ("play", b"\xaa")]
    assert result.output.index("Performing Reset.. Done") < result.output.index("Chain Scan Result:")


def test_xsvf_player_error_exits_one(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    source = tmp_path / "design.xsvf"
    source.write_bytes(b"\x00")
    session = FakeXsvfCliSession(
        play_error=XsvfPlayerError(0x02, "XSVF_ERROR_TDOMISMATCH", "captured TDO did not match expected TDO")
    )
    monkeypatch.setattr(xsvf_module, "open_xsvf_session", _xsvf_factory(session))

    result = CliRunner().invoke(cli, ["xsvf", "--file", str(source), "--port", "COM9"])

    assert result.exit_code == 1
    assert "XSVF_ERROR_TDOMISMATCH" in result.output
    assert "Success!" not in result.output


def test_xsvf_scan_host_error_exits_two(monkeypatch: pytest.MonkeyPatch) -> None:
    session = FakeXsvfCliSession(scan_error=XsvfHostError("Chain scan: incomplete reply (bounded timeout)"))
    monkeypatch.setattr(xsvf_module, "open_xsvf_session", _xsvf_factory(session))

    result = CliRunner().invoke(cli, ["xsvf", "--scan", "--port", "COM9"])

    assert result.exit_code == 2
    assert "Chain scan: incomplete reply" in result.output


def test_xsvf_transport_failure_exits_two(monkeypatch: pytest.MonkeyPatch) -> None:
    def failing_factory(connection: ConnectionSettings) -> AbstractContextManager[XsvfSession]:
        raise TransportError(f"Could not open serial port {connection.port}")

    monkeypatch.setattr(xsvf_module, "open_xsvf_session", failing_factory)
    result = CliRunner().invoke(cli, ["xsvf", "--scan", "--port", "COM9"])

    assert result.exit_code == 2
    assert "Could not open serial port COM9" in result.output


def test_xsvf_requires_an_operation() -> None:
    result = CliRunner().invoke(cli, ["xsvf", "--port", "COM9"])
    assert result.exit_code == 2
    assert "No file specified; pass --file, --scan, or --reset" in result.output


def test_xsvf_requires_port() -> None:
    result = CliRunner().invoke(cli, ["xsvf", "--scan"])
    assert result.exit_code == 2
    assert "No serial port specified" in result.output


def test_xsvf_uses_configured_port(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    config = tmp_path / "bp.yaml"
    config.write_text("port: COM7\nspeed: 57600\n", encoding="utf-8")
    session = FakeXsvfCliSession()
    captured: list[ConnectionSettings] = []
    monkeypatch.setattr(xsvf_module, "open_xsvf_session", _xsvf_factory(session, captured))

    result = CliRunner().invoke(cli, ["--config", str(config), "xsvf", "--scan"])

    assert result.exit_code == 0
    assert captured == [ConnectionSettings(port="COM7", speed=57_600)]


def test_xsvf_rejects_missing_file() -> None:
    result = CliRunner().invoke(cli, ["xsvf", "--file", "missing.xsvf", "--port", "COM9"])
    assert result.exit_code == 2
    assert "does not exist" in result.output


def test_xsvf_rejects_empty_file_before_opening_session(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    source = tmp_path / "empty.xsvf"
    source.write_bytes(b"")
    session = FakeXsvfCliSession()
    monkeypatch.setattr(xsvf_module, "open_xsvf_session", _xsvf_factory(session))

    result = CliRunner().invoke(cli, ["xsvf", "--file", str(source), "--port", "COM9"])

    assert result.exit_code == 2
    assert "is empty" in result.output
    assert session.calls == []


def test_xsvf_rejects_invalid_speed() -> None:
    result = CliRunner().invoke(cli, ["xsvf", "--scan", "--port", "COM9", "--speed", "0"])
    assert result.exit_code == 2
    assert "not in the range" in result.output


def test_xsvf_rejects_unknown_option() -> None:
    result = CliRunner().invoke(cli, ["xsvf", "--scan", "--port", "COM9", "--chunk-size", "16"])
    assert result.exit_code == 2
    assert "No such option '--chunk-size'" in result.output


def test_xsvf_invalid_config_exits_two(tmp_path: Path) -> None:
    config = tmp_path / "bp.yaml"
    config.write_text("unknown_key: 1\n", encoding="utf-8")
    result = CliRunner().invoke(cli, ["--config", str(config), "xsvf", "--scan", "--port", "COM9"])
    assert result.exit_code == 2
    assert "Invalid configuration" in result.output
