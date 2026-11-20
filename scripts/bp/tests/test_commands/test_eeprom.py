# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for text-hex files and EEPROM command services."""

from __future__ import annotations

from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from pathlib import Path

import pytest

from bp.commands.eeprom import (
    read_eeprom_to_file,
    verify_eeprom_from_file,
    write_eeprom_from_file,
)
from bp.errors import CommandError, EepromVerifyError
from bp.hextext import format_hex_text, parse_hex_text, read_hex_file, write_hex_file
from bp.protocol.i2c import EepromSession, EepromWriteReport
from bp.types import AddressWidth, ChipAddress, ConnectionSettings, EepromLayout


class FakeEepromSession:
    """Record EEPROM service calls without touching a serial port."""

    def __init__(
        self,
        *,
        read_data: bytes = b"",
        write_report: EepromWriteReport | None = None,
        verify_error: EepromVerifyError | None = None,
    ) -> None:
        self.read_data = read_data
        self.write_report = write_report or EepromWriteReport(bytes_written=0, retries=0)
        self.verify_error = verify_error
        self.read_calls: list[tuple[EepromLayout, int]] = []
        self.write_calls: list[tuple[EepromLayout, bytes]] = []
        self.verify_calls: list[tuple[EepromLayout, bytes]] = []

    def read_eeprom(self, layout: EepromLayout, length: int) -> bytes:
        self.read_calls.append((layout, length))
        return self.read_data

    def write_eeprom(self, layout: EepromLayout, data: bytes) -> EepromWriteReport:
        self.write_calls.append((layout, data))
        return self.write_report

    def verify_eeprom(self, layout: EepromLayout, expected: bytes) -> None:
        self.verify_calls.append((layout, expected))
        if self.verify_error is not None:
            raise self.verify_error


def _factory(session: FakeEepromSession) -> Callable[[ConnectionSettings], AbstractContextManager[EepromSession]]:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[EepromSession, None, None]:
        del connection
        yield session

    return factory


def _layout(memory_address: int = 0) -> EepromLayout:
    return EepromLayout(ChipAddress(0), AddressWidth.TWO_BYTE, memory_address)


def test_parse_hex_text_accepts_supported_token_forms() -> None:
    assert parse_hex_text("FF aa 0x0b\n0X0C\t1") == b"\xff\xaa\x0b\x0c\x01"


@pytest.mark.parametrize("token", ["0x", "GG", "123", "FF,", "0x100", "-1"])
def test_parse_hex_text_rejects_invalid_tokens(token: str) -> None:
    with pytest.raises(CommandError, match="Invalid hex token"):
        parse_hex_text(f"AA {token}")


def test_parse_hex_text_rejects_empty_input() -> None:
    with pytest.raises(CommandError, match="contains no bytes"):
        parse_hex_text(" \r\n\t")


def test_format_hex_text_uses_uppercase_two_digits() -> None:
    assert format_hex_text(b"\x00\xff\x0a") == "00 FF 0A\n"


def test_read_hex_file_wraps_missing_file(tmp_path: Path) -> None:
    with pytest.raises(CommandError, match="Could not read hex file"):
        read_hex_file(tmp_path / "missing.txt")


def test_read_hex_file_rejects_undecodable_content(tmp_path: Path) -> None:
    path = tmp_path / "bad.txt"
    path.write_bytes(b"\xff\xfe\x00")
    with pytest.raises(CommandError, match="Could not read hex file"):
        read_hex_file(path)


def test_write_hex_file_wraps_filesystem_errors(tmp_path: Path) -> None:
    with pytest.raises(CommandError, match="Could not write hex file"):
        write_hex_file(tmp_path, b"\x00")


def test_read_eeprom_to_file_uses_session_and_writes_hex(
    tmp_path: Path,
    connection: ConnectionSettings,
) -> None:
    session = FakeEepromSession(read_data=b"\x01\x02")
    output = tmp_path / "dump.txt"
    data = read_eeprom_to_file(
        connection,
        _layout(0x10),
        2,
        output,
        session_factory=_factory(session),
    )
    assert data == b"\x01\x02"
    assert output.read_text(encoding="ascii") == "01 02\n"
    assert session.read_calls == [(_layout(0x10), 2)]


def test_read_eeprom_to_file_rejects_span_before_opening_session(
    tmp_path: Path,
    connection: ConnectionSettings,
) -> None:
    session = FakeEepromSession()
    with pytest.raises(CommandError, match="extends past"):
        read_eeprom_to_file(
            connection,
            _layout(0xFFFF),
            2,
            tmp_path / "dump.txt",
            session_factory=_factory(session),
        )
    assert session.read_calls == []


def test_read_hex_file_accepts_utf8_bom(tmp_path: Path) -> None:
    path = tmp_path / "bom.txt"
    path.write_text("\ufeffAA\n", encoding="utf-8")
    assert read_hex_file(path) == b"\xaa"


def test_read_eeprom_to_file_wraps_output_failure(
    tmp_path: Path,
    connection: ConnectionSettings,
) -> None:
    session = FakeEepromSession(read_data=b"\x01")
    with pytest.raises(CommandError, match="Could not write hex file"):
        read_eeprom_to_file(
            connection,
            _layout(),
            1,
            tmp_path,
            session_factory=_factory(session),
        )
    assert session.read_calls == [(_layout(), 1)]


def test_write_eeprom_from_file_rejects_span_before_opening_session(
    tmp_path: Path,
    connection: ConnectionSettings,
) -> None:
    source = tmp_path / "image.txt"
    source.write_text("AA BB\n", encoding="ascii")
    session = FakeEepromSession()
    with pytest.raises(CommandError, match="extends past"):
        write_eeprom_from_file(
            connection,
            _layout(0xFFFF),
            source,
            session_factory=_factory(session),
        )
    assert session.write_calls == []


def test_write_eeprom_from_file_parses_then_writes(
    tmp_path: Path,
    connection: ConnectionSettings,
) -> None:
    source = tmp_path / "image.txt"
    source.write_text("0A 0B\n", encoding="ascii")
    session = FakeEepromSession(write_report=EepromWriteReport(bytes_written=2, retries=1))
    report = write_eeprom_from_file(
        connection,
        _layout(0x20),
        source,
        session_factory=_factory(session),
    )
    assert report == EepromWriteReport(bytes_written=2, retries=1)
    assert session.write_calls == [(_layout(0x20), b"\x0a\x0b")]


def test_write_eeprom_from_file_rejects_truncated_hex_before_opening_session(
    tmp_path: Path,
    connection: ConnectionSettings,
) -> None:
    source = tmp_path / "image.txt"
    source.write_text("0A 0x\n", encoding="ascii")
    session = FakeEepromSession()
    with pytest.raises(CommandError, match="Invalid hex token '0x'"):
        write_eeprom_from_file(connection, _layout(), source, session_factory=_factory(session))
    assert session.write_calls == []


def test_verify_eeprom_from_file_returns_length(
    tmp_path: Path,
    connection: ConnectionSettings,
) -> None:
    source = tmp_path / "expected.txt"
    source.write_text("AA BB\n", encoding="ascii")
    session = FakeEepromSession()
    length = verify_eeprom_from_file(
        connection,
        _layout(0x30),
        source,
        session_factory=_factory(session),
    )
    assert length == 2
    assert session.verify_calls == [(_layout(0x30), b"\xaa\xbb")]


def test_verify_eeprom_from_file_propagates_mismatch(
    tmp_path: Path,
    connection: ConnectionSettings,
) -> None:
    source = tmp_path / "expected.txt"
    source.write_text("AA\n", encoding="ascii")
    session = FakeEepromSession(verify_error=EepromVerifyError(0, 0xAA, 0x55))
    with pytest.raises(EepromVerifyError, match="expected 0xAA, read 0x55"):
        verify_eeprom_from_file(connection, _layout(), source, session_factory=_factory(session))


def test_verify_eeprom_from_file_rejects_empty_file(
    tmp_path: Path,
    connection: ConnectionSettings,
) -> None:
    source = tmp_path / "expected.txt"
    source.write_text("\n", encoding="ascii")
    session = FakeEepromSession()
    with pytest.raises(CommandError, match="contains no bytes"):
        verify_eeprom_from_file(connection, _layout(), source, session_factory=_factory(session))
    assert session.verify_calls == []
