"""Tests for binary I2C, register helpers, scanning, and EEPROM transfers."""

from __future__ import annotations

import pytest

from bp.errors import EepromVerifyError, EepromWriteError, I2cNackError, ProtocolError
from bp.protocol.i2c import EepromWriteReport, I2CController, I2cScanHit, I2cSpeed
from bp.types import AddressWidth, ChipAddress, EepromLayout
from conftest import ScriptedTransport

ENTER = b"\x00" * 21
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


def test_enter_can_leave_power_off_and_pullups_on() -> None:
    transport = ScriptedTransport([b"BBIO1", b"I2C1", b"\x01", b"BBIO1", b"\x01"])
    with I2CController(transport, power=False, pullups=True):
        pass
    assert transport.writes == [ENTER, b"\x02", b"\x44", *CLEANUP]


def test_configure_speed_sets_low_two_bits() -> None:
    transport = ScriptedTransport([b"\x01"])
    I2CController(transport).configure_speed(I2cSpeed.KHZ_400)
    assert transport.writes == [b"\x63"]


def test_bus_control_commands_accept_zero_and_one_replies() -> None:
    transport = ScriptedTransport([b"\x00", b"\x00", b"\x00", b"\x00"])
    controller = I2CController(transport)
    controller.start()
    controller.stop()
    controller.acknowledge()
    controller.not_acknowledge()
    assert transport.writes == [b"\x02", b"\x03", b"\x06", b"\x07"]


@pytest.mark.parametrize(
    ("method", "command", "operation"),
    [
        ("start", b"\x02", "start"),
        ("stop", b"\x03", "stop"),
        ("acknowledge", b"\x06", "ACK"),
        ("not_acknowledge", b"\x07", "NACK"),
    ],
)
def test_bus_control_commands_reject_other_replies(method: str, command: bytes, operation: str) -> None:
    transport = ScriptedTransport([b"\x02"])
    controller = I2CController(transport)
    with pytest.raises(ProtocolError, match=f"I2C {operation} command returned 0x02"):
        getattr(controller, method)()
    assert transport.writes == [command]


def test_device_read_byte_sequence_and_repeated_start() -> None:
    transport = ScriptedTransport(
        [
            b"BBIO1",
            b"I2C1",
            b"\x01",
            b"\x01",
            b"\x01\x00\x00",
            b"\x01",
            b"\x01\x00",
            b"\x5a",
            b"\x01",
            b"\x01",
            b"BBIO1",
            b"\x01",
        ]
    )
    with I2CController(transport) as controller:
        assert controller.device(0x50).read_byte(0x10) == 0x5A
    assert transport.writes == [
        ENTER,
        b"\x02",
        b"\x4c",
        b"\x02",
        b"\x11\xa0\x10",
        b"\x02",
        b"\x10\xa1",
        b"\x04",
        b"\x07",
        b"\x03",
        *CLEANUP,
    ]


def test_device_write_byte_checks_all_acknowledgements() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01\x00\x00\x00", b"\x01"])
    controller = I2CController(transport)
    controller.device(0x50).write_byte(0x10, 0xA5)
    assert transport.writes == [b"\x02", b"\x12\xa0\x10\xa5", b"\x03"]


def test_device_read_word_acknowledges_first_byte_and_nacks_second() -> None:
    transport = ScriptedTransport(
        [
            b"\x01",
            b"\x01\x00\x00",
            b"\x01",
            b"\x01\x00",
            b"\x12",
            b"\x01",
            b"\x34",
            b"\x01",
            b"\x01",
        ]
    )
    controller = I2CController(transport)
    assert controller.device(0x48).read_word(0xAA) == 0x1234
    assert transport.writes == [
        b"\x02",
        b"\x11\x90\xaa",
        b"\x02",
        b"\x10\x91",
        b"\x04",
        b"\x06",
        b"\x04",
        b"\x07",
        b"\x03",
    ]


def test_device_write_word_is_big_endian() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01\x00\x00\x00\x00", b"\x01"])
    I2CController(transport).device(0x48).write_word(0xAA, 0x1234)
    assert transport.writes == [b"\x02", b"\x13\x90\xaa\x12\x34", b"\x03"]


def test_device_command_writes_one_payload_byte() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01\x00\x00", b"\x01"])
    I2CController(transport).device(0x48).command(0x51)
    assert transport.writes == [b"\x02", b"\x11\x90\x51", b"\x03"]


def test_device_read_block_reads_sequential_bytes() -> None:
    transport = ScriptedTransport(
        [
            b"\x01",
            b"\x01\x00\x00",
            b"\x01",
            b"\x01\x00",
            b"\x11",
            b"\x01",
            b"\x22",
            b"\x01",
            b"\x01",
        ]
    )
    controller = I2CController(transport)
    assert controller.device(0x50).read_block(0x00, 2) == b"\x11\x22"
    assert transport.writes[-3:] == [b"\x04", b"\x07", b"\x03"]
    assert transport.writes.count(b"\x06") == 1


def test_device_read_block_without_register_starts_at_read_address() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01\x00", b"\xaa", b"\x01", b"\x01"])
    controller = I2CController(transport)
    assert controller.device(0x50).read_block(None, 1) == b"\xaa"
    assert transport.writes == [b"\x02", b"\x10\xa1", b"\x04", b"\x07", b"\x03"]


def test_device_write_block_splits_at_sixteen_bytes() -> None:
    responses = [b"\x01", b"\x01" + b"\x00" * 16, b"\x01" + b"\x00" * 3, b"\x01"]
    transport = ScriptedTransport(responses)
    payload = bytes(range(17))
    I2CController(transport).device(0x50).write_block(0x00, payload)
    assert transport.writes == [
        b"\x02",
        b"\x1f\xa0\x00" + payload[:14],
        b"\x12" + payload[14:],
        b"\x03",
    ]


def test_device_read_byte_rejects_nack_on_register() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01\x01\x00", b"\x01"])
    with pytest.raises(I2cNackError) as raised:
        I2CController(transport).device(0x50).read_byte(0x10)
    assert raised.value.address == 0x50
    assert raised.value.operation == "register address"
    assert "0x50" in str(raised.value)
    assert transport.writes[-1] == b"\x03"


def test_device_read_byte_rejects_nack_on_read_address() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01\x00\x00", b"\x01", b"\x01\x01", b"\x01"])
    with pytest.raises(I2cNackError, match="read address"):
        I2CController(transport).device(0x50).read_byte(0x10)


def test_device_read_block_rejects_nack_on_register_or_read_address() -> None:
    register_nack = ScriptedTransport([b"\x01", b"\x01\x01\x00", b"\x01"])
    with pytest.raises(I2cNackError, match="register address"):
        I2CController(register_nack).device(0x50).read_block(0x10, 1)

    read_nack = ScriptedTransport([b"\x01", b"\x01\x01", b"\x01"])
    with pytest.raises(I2cNackError, match="read address"):
        I2CController(read_nack).device(0x50).read_block(None, 1)


def test_device_write_block_rejects_any_nack() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01\x00\x01\x00", b"\x01"])
    with pytest.raises(I2cNackError, match="block write"):
        I2CController(transport).device(0x50).write_block(0x00, b"\xaa")


@pytest.mark.parametrize(
    ("operation", "command", "ack_bytes"),
    [
        ("register write", b"\x12\xa0\x10\xa5", b"\x00\x01\x00"),
        ("register word write", b"\x13\xa0\x10\x12\x34", b"\x00\x01\x00\x00"),
        ("command", b"\x11\xa0\x51", b"\x00\x01"),
    ],
)
def test_device_writes_reject_data_phase_nacks(operation: str, command: bytes, ack_bytes: bytes) -> None:
    transport = ScriptedTransport([b"\x01", b"\x01" + ack_bytes, b"\x01"])
    device = I2CController(transport).device(0x50)
    with pytest.raises(I2cNackError, match=operation):
        if operation == "register write":
            device.write_byte(0x10, 0xA5)
        elif operation == "register word write":
            device.write_word(0x10, 0x1234)
        else:
            device.command(0x51)
    assert transport.writes[1] == command


def test_scan_reports_write_and_read_acknowledgements() -> None:
    responses = [
        b"\x01",
        b"\x01\x00",
        b"\x01",
        b"\x01",
        b"\x01\x00",
        b"\x55",
        b"\x01",
        b"\x01",
        b"\x01",
        b"\x01\x01",
        b"\x01",
        b"\x01",
        b"\x01\x00",
        b"\x66",
        b"\x01",
        b"\x01",
    ]
    transport = ScriptedTransport(responses)
    hits = I2CController(transport).scan(start=0x50, end=0x51)
    assert hits == (
        I2cScanHit(address=0x50, write_ack=True, read_ack=True),
        I2cScanHit(address=0x51, write_ack=False, read_ack=True),
    )
    assert transport.writes[0:3] == [b"\x02", b"\x10\xa0", b"\x03"]
    assert transport.writes[3:8] == [b"\x02", b"\x10\xa1", b"\x04", b"\x07", b"\x03"]


def test_scan_with_no_devices_returns_empty_tuple() -> None:
    responses = [b"\x01", b"\x01\x01", b"\x01"] * 2
    assert I2CController(ScriptedTransport(responses)).scan(start=0x30, end=0x30) == ()


@pytest.mark.parametrize(
    ("start", "end"),
    [(-1, 0x10), (0x10, 0x0F), (0, 0x80)],
)
def test_scan_rejects_invalid_ranges(start: int, end: int) -> None:
    with pytest.raises(ValueError, match="scan range"):
        I2CController(ScriptedTransport()).scan(start=start, end=end)


@pytest.mark.parametrize("address", [-1, 0x80])
def test_device_rejects_invalid_address(address: int) -> None:
    with pytest.raises(ValueError, match="0 through 0x7F"):
        I2CController(ScriptedTransport()).device(address)


def test_scan_hit_requires_at_least_one_acknowledgement() -> None:
    with pytest.raises(ValueError, match="at least one acknowledgement"):
        I2cScanHit(address=0x50, write_ack=False, read_ack=False)


@pytest.mark.parametrize("address", [-1, 0x80])
def test_scan_hit_rejects_addresses_outside_seven_bits(address: int) -> None:
    with pytest.raises(ValueError, match="0 through 0x7F"):
        I2cScanHit(address=address, write_ack=True, read_ack=False)


def test_device_rejects_out_of_range_register_and_value() -> None:
    device = I2CController(ScriptedTransport()).device(0x50)
    with pytest.raises(ValueError, match="I2C register"):
        device.read_byte(0x100)
    with pytest.raises(ValueError, match="I2C value"):
        device.write_byte(0x00, 0x100)
    with pytest.raises(ValueError, match="word value"):
        device.write_word(0x00, 0x10000)


def test_device_read_block_rejects_non_positive_length() -> None:
    with pytest.raises(ValueError, match="length must be positive"):
        I2CController(ScriptedTransport()).device(0x50).read_block(0x00, 0)


def test_device_write_block_rejects_empty_data() -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        I2CController(ScriptedTransport()).device(0x50).write_block(0x00, b"")
