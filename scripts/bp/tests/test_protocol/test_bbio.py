"""Tests for binary bitbang framing and version-banner parsing."""

from __future__ import annotations

import pytest

from bp.errors import ProtocolError
from bp.protocol.bbio import (
    BBIO_ENTER_SEQUENCE,
    VERSION_COMMAND,
    BbioSession,
    parse_version_banner,
    read_version_info,
)
from bp.types import BpBinaryMode
from conftest import ScriptedTransport


def test_enter_binary_uses_documented_nul_sequence() -> None:
    transport = ScriptedTransport([b"BBIO1"])
    BbioSession(transport).enter_binary()
    assert transport.writes == [BBIO_ENTER_SEQUENCE]
    assert transport.reset_count == 2
    transport.assert_consumed()


def test_enter_binary_rejects_wrong_banner() -> None:
    transport = ScriptedTransport([b"SPI1x"])
    with pytest.raises(ProtocolError, match="Expected BBIO1 banner"):
        BbioSession(transport).enter_binary()


def test_enter_binary_reports_timeout_as_protocol_error() -> None:
    transport = ScriptedTransport()
    with pytest.raises(ProtocolError, match="did not enter binary mode"):
        BbioSession(transport).enter_binary()


def test_enter_mode_uses_mode_identifier_and_banner() -> None:
    transport = ScriptedTransport([b"I2C1"])
    BbioSession(transport).enter_mode(BpBinaryMode.I2C)
    assert transport.writes == [b"\x02"]


def test_enter_mode_rejects_wrong_banner() -> None:
    transport = ScriptedTransport([b"SPI1"])
    with pytest.raises(ProtocolError, match="Expected I2C1 banner"):
        BbioSession(transport).enter_mode(BpBinaryMode.I2C)


def test_enter_xsvf_mode_uses_0x18_and_xsv1_banner() -> None:
    transport = ScriptedTransport([b"XSV1"])
    BbioSession(transport).enter_mode(BpBinaryMode.XSVF)
    assert transport.writes == [b"\x18"]


def test_configure_peripherals_sets_exact_flags() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    session = BbioSession(transport)
    session.configure_peripherals(power=True, pullups=True)
    session.configure_peripherals()
    assert transport.writes == [b"\x4c", b"\x40"]


@pytest.mark.parametrize(
    ("status", "message"),
    [
        (b"\x00", "reported a failed binary command"),
        (b"\x02", "Expected binary success status 0x01, received 0x02"),
    ],
)
def test_expect_success_rejects_failure_and_unknown_status(status: bytes, message: str) -> None:
    transport = ScriptedTransport()
    transport.feed(status)
    with pytest.raises(ProtocolError, match=message):
        BbioSession(transport).expect_success()


def test_exit_and_reset_sequence() -> None:
    transport = ScriptedTransport([b"BBIO1", b"\x01"])
    session = BbioSession(transport)
    session.exit_to_bitbang()
    session.reset()
    assert transport.writes == [b"\x00", b"\x0f"]


def test_exit_to_bitbang_rejects_wrong_banner() -> None:
    transport = ScriptedTransport([b"I2C1X"])
    with pytest.raises(ProtocolError, match="while leaving mode"):
        BbioSession(transport).exit_to_bitbang()


def test_read_version_info_parses_reset_banner() -> None:
    banner = (
        b"\r\nRESET\r\nBus Pirate v4\r\nElderlyPirate Firmware v9.0\r\n"
        b"[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]\r\n"
        b"DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)\r\n"
        b"http://github.com/ElderlyPirate\r\nHiZ>"
    )
    transport = ScriptedTransport([banner])
    info = read_version_info(transport)
    assert info.hardware_type == "Bus Pirate v4"
    assert info.firmware_version == "ElderlyPirate Firmware v9.0"
    assert info.hardware_version == "DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)"
    assert info.update_url == "http://github.com/ElderlyPirate"
    assert transport.writes == [VERSION_COMMAND]


def test_read_version_info_rejects_empty_response() -> None:
    with pytest.raises(ProtocolError, match="no version banner"):
        read_version_info(ScriptedTransport())


def test_read_version_info_rejects_non_ascii_response() -> None:
    transport = ScriptedTransport([b"RESET\nhardware\nfirmware\nversion\nurl\n\xff"])
    with pytest.raises(ProtocolError, match="non-ASCII"):
        read_version_info(transport)


def test_parse_version_banner_uses_last_reset_and_ignores_prompt() -> None:
    text = (
        "Earlier banner\nRESET\nOld hardware\nOld firmware\nOld hardware version\nOld URL\n"
        "RESET\nBus Pirate v4.0\nFirmware v8.0\n[HiZ SPI]\nDEVID:0x1234\n"
        "https://example.invalid\nHiZ>"
    )
    info = parse_version_banner(text)
    assert info.hardware_type == "Bus Pirate v4.0"
    assert info.firmware_version == "Firmware v8.0"
    assert info.hardware_version == "DEVID:0x1234"
    assert info.update_url == "https://example.invalid"


def test_parse_version_banner_accepts_v3_bootloader_line() -> None:
    text = (
        "RESET\nBus Pirate v3.6\nFirmware v9.0\n[HiZ 1WIRE UART I2C SPI]\n"
        "Bootloader v5.0\nDEVID:0x044F REVID:0x0004\n"
        "http://github.com/ElderlyPirate\nHiZ>"
    )
    info = parse_version_banner(text)
    assert info.hardware_type == "Bus Pirate v3.6"
    assert info.firmware_version == "Firmware v9.0"
    assert info.hardware_version == "DEVID:0x044F REVID:0x0004"
    assert info.update_url == "http://github.com/ElderlyPirate"


@pytest.mark.parametrize(
    "text",
    [
        "",
        "RESET\nonly one line",
        "No reset marker\nhardware\nfirmware\nversion\nurl",
        "RESET\nBus Pirate v4\nFirmware v9.0\nDEVID:0x1019",
        "RESET\nBus Pirate v4\nFirmware v9.0\nDEVID:0x1019\nno-url",
    ],
)
def test_parse_version_banner_rejects_incomplete_banner(text: str) -> None:
    with pytest.raises(ProtocolError, match="Incomplete version banner"):
        parse_version_banner(text)
