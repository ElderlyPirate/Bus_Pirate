"""Tests for binary I2C and external EEPROM transfer sequencing."""

from __future__ import annotations

import pytest

from bp.errors import EepromVerifyError, EepromWriteError, ProtocolError
from bp.protocol.i2c import EepromWriteReport, I2CController
from bp.types import AddressWidth, ChipAddress, EepromLayout
from conftest import ScriptedTransport

ENTER = b"\x00" * 20
CLEANUP = [b"\x00", b"\x0f"]


def _two_byte_layout(memory_address: int = 0x0102) -> EepromLayout:
    return EepromLayout(ChipAddress(0), AddressWidth.TWO_BYTE, memory_address)


def _one_byte_layout(memory_address: int = 0x0123) -> EepromLayout:
    return EepromLayout(ChipAddress(2), AddressWidth.ONE_BYTE, memory_address)


def test_read_two_byte_address_sequence() -> None:
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00\x00",
            b"\x01",
            b"\x01",
            b"\x01\x00",
            b"\xaa",
            b"\x01",
            b"\xbb",
            b"\x01",
            b"\xcc",
            b"\x01",
            b"\x01",
            b"BBIO1",
            b"\x01",
        ]
    )
    with I2CController(transport) as controller:
        data = controller.read_eeprom(_two_byte_layout(), 3)

    assert data == b"\xaa\xbb\xcc"
    assert transport.writes == [
        ENTER,
        b"\x02",
        b"\x4c",
        b"\x02",
        b"\x12\xa0\x01\x02",
        b"\x03",
        b"\x02",
        b"\x10\xa1",
        b"\x04",
        b"\x06",
        b"\x04",
        b"\x06",
        b"\x04",
        b"\x07",
        b"\x03",
        *CLEANUP,
    ]
    transport.assert_consumed()


def test_read_one_byte_address_uses_page_bits() -> None:
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00",
            b"\x01",
            b"\x01",
            b"\x01\x00",
            b"\x11",
            b"\x01",
            b"\x22",
            b"\x01",
            b"\x01",
            b"BBIO1",
            b"\x01",
        ]
    )
    with I2CController(transport) as controller:
        data = controller.read_eeprom(_one_byte_layout(), 2)

    assert data == b"\x11\x22"
    assert transport.writes == [
        ENTER,
        b"\x02",
        b"\x4c",
        b"\x02",
        b"\x11\xa6\x23",
        b"\x03",
        b"\x02",
        b"\x10\xa7",
        b"\x04",
        b"\x06",
        b"\x04",
        b"\x07",
        b"\x03",
        *CLEANUP,
    ]


def test_read_rejects_nack_on_memory_address() -> None:
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            b"\x01\x00\x01\x00",
            b"\x01",
        ]
    )
    with pytest.raises(ProtocolError, match="did not acknowledge memory address 0x0102"):
        with I2CController(transport) as controller:
            controller.read_eeprom(_two_byte_layout(), 1)
    assert transport.writes[-1] == b"\x03"


def test_read_rejects_nack_on_read_address() -> None:
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00\x00",
            b"\x01",
            b"\x01",
            b"\x01\x01",
            b"\x01",
        ]
    )
    with pytest.raises(ProtocolError, match="did not acknowledge read address 0xA1"):
        with I2CController(transport) as controller:
            controller.read_eeprom(_two_byte_layout(), 1)
    assert transport.writes[-1] == b"\x03"


def test_write_retries_after_nack_without_page_writes() -> None:
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00\x00\x01",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00\x00\x00",
            b"\x01",
            b"BBIO1",
            b"\x01",
        ]
    )
    with I2CController(transport, retry_delay=0) as controller:
        report = controller.write_eeprom(_two_byte_layout(0), b"\x5a")

    assert report == EepromWriteReport(bytes_written=1, retries=1)
    frames = [write for write in transport.writes if write.startswith(b"\x13")]
    assert frames == [b"\x13\xa0\x00\x00\x5a", b"\x13\xa0\x00\x00\x5a"]
    assert transport.writes.count(b"\x03") == 2


def test_write_eeprom_increments_address_per_byte() -> None:
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00\x00\x00",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00\x00\x00",
            b"\x01",
            b"BBIO1",
            b"\x01",
        ]
    )
    with I2CController(transport) as controller:
        report = controller.write_eeprom(_two_byte_layout(0x00FE), b"\x01\x02")

    assert report == EepromWriteReport(bytes_written=2, retries=0)
    frames = [write for write in transport.writes if write.startswith(b"\x13")]
    assert frames == [b"\x13\xa0\x00\xfe\x01", b"\x13\xa0\x00\xff\x02"]
    assert transport.writes.count(b"\x03") == 2
    transport.assert_consumed()


def test_write_fails_after_attempt_limit() -> None:
    nack_reply = b"\x01\x00\x00\x00\x01"
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            nack_reply,
            b"\x01",
            b"\x01",
            nack_reply,
            b"\x01",
        ]
    )
    with pytest.raises(EepromWriteError) as raised:
        with I2CController(transport, write_attempts=2, retry_delay=0) as controller:
            controller.write_eeprom(_two_byte_layout(0), b"\x5a")
    assert raised.value.address == 0
    assert raised.value.value == 0x5A
    assert raised.value.attempts == 2
    assert len([write for write in transport.writes if write.startswith(b"\x13")]) == 2


def test_verify_detects_mismatch_offset() -> None:
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00\x00",
            b"\x01",
            b"\x01",
            b"\x01\x00",
            b"\xaa",
            b"\x01",
            b"\xbb",
            b"\x01",
            b"\x01",
        ]
    )
    with pytest.raises(EepromVerifyError) as raised:
        with I2CController(transport) as controller:
            controller.verify_eeprom(_two_byte_layout(0), b"\xaa\xcc")
    assert raised.value.offset == 1
    assert raised.value.expected == 0xCC
    assert raised.value.actual == 0xBB


def test_verify_success_returns_none() -> None:
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00\x00",
            b"\x01",
            b"\x01",
            b"\x01\x00",
            b"\xaa",
            b"\x01",
            b"\x01",
            b"BBIO1",
            b"\x01",
        ]
    )
    with I2CController(transport) as controller:
        controller.verify_eeprom(_two_byte_layout(), b"\xaa")
    transport.assert_consumed()


@pytest.mark.parametrize("length", [0, -1])
def test_read_rejects_non_positive_length(length: int) -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        I2CController(ScriptedTransport()).read_eeprom(_two_byte_layout(0), length)


def test_read_rejects_span_past_address_space() -> None:
    layout = EepromLayout(ChipAddress(0), AddressWidth.TWO_BYTE, 0xFFFF)
    with pytest.raises(ValueError, match="extends past"):
        I2CController(ScriptedTransport()).read_eeprom(layout, 2)


def test_write_rejects_empty_data() -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        I2CController(ScriptedTransport()).write_eeprom(_two_byte_layout(), b"")


def test_bulk_write_rejects_unsupported_lengths() -> None:
    controller = I2CController(ScriptedTransport())
    with pytest.raises(ValueError, match="1 through 16"):
        controller.write_bytes(b"")
    with pytest.raises(ValueError, match="1 through 16"):
        controller.write_bytes(b"\x00" * 17)


def test_constructor_rejects_invalid_retry_configuration() -> None:
    with pytest.raises(ValueError, match="attempts must be positive"):
        I2CController(ScriptedTransport(), write_attempts=0)
    with pytest.raises(ValueError, match="retry delay must not be negative"):
        I2CController(ScriptedTransport(), retry_delay=-1)


def test_enter_rejects_failed_peripheral_configuration() -> None:
    transport = ScriptedTransport([b"BBIO1", b"I2C1", b"\x00"])
    with pytest.raises(ProtocolError, match="failed binary command"):
        with I2CController(transport):
            pass
