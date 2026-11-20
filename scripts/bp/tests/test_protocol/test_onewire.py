"""Tests for binary 1-Wire ROM, search, scratchpad, memory, and status operations."""

from __future__ import annotations

import pytest

from bp.errors import ProtocolError
from bp.protocol.onewire import OneWireController, OneWireRom, OneWireSpeed, onewire_crc8
from conftest import ScriptedTransport

ENTER = b"\x00" * 21
CLEANUP = [b"\x00", b"\x0f"]
TERMINATOR = b"\xff" * 8


def _rom(family: int, serial: bytes) -> OneWireRom:
    body = bytes([family]) + serial
    return OneWireRom(body + bytes([onewire_crc8(body)]))


def test_crc8_matches_dallas_reference_vector() -> None:
    assert onewire_crc8(b"\x02") == 0xBC


def test_rom_fields_and_crc_validation() -> None:
    rom = _rom(0x28, b"\xff\x64\x1e\x24\x16\x04")
    assert rom.family == 0x28
    assert rom.serial == b"\xff\x64\x1e\x24\x16\x04"
    assert rom.crc_valid is True
    assert rom.hex == rom.data.hex().upper()


def test_rom_crc_invalid_after_serial_change() -> None:
    rom = _rom(0x28, b"\xff\x64\x1e\x24\x16\x04")
    changed = OneWireRom(rom.data[:2] + bytes([rom.data[2] ^ 0x01]) + rom.data[3:])
    assert changed.crc_valid is False


def test_rom_rejects_wrong_length() -> None:
    with pytest.raises(ValueError, match="8 bytes"):
        OneWireRom(b"\x28")


def test_configure_speed_and_peripherals() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    controller = OneWireController(transport)
    controller.configure_speed(OneWireSpeed.OVERDRIVE)
    controller.configure_peripherals(power=True, pullups=True)
    assert transport.writes == [b"\x21", b"\x4c"]


def test_reset_read_byte_and_read_bytes() -> None:
    transport = ScriptedTransport([b"\x01", b"\x11", b"\x22"])
    controller = OneWireController(transport)
    controller.reset()
    assert controller.read_byte() == 0x11
    assert controller.read_bytes(1) == b"\x22"
    assert transport.writes == [b"\x02", b"\x04", b"\x04"]


def test_write_bytes_uses_bulk_and_per_byte_status() -> None:
    transport = ScriptedTransport([b"\x01"] * 5)
    OneWireController(transport).write_bytes(b"\xaa\xbb\xcc\xdd")
    assert transport.writes == [b"\x13", b"\xaa", b"\xbb", b"\xcc", b"\xdd"]


def test_write_bytes_splits_at_sixteen_bytes() -> None:
    payload = bytes(range(17))
    transport = ScriptedTransport([b"\x01"] * 19)
    OneWireController(transport).write_bytes(payload)
    assert transport.writes == [b"\x1f", *[bytes([value]) for value in payload[:16]], b"\x10", bytes([payload[16]])]


def test_search_rom_parses_devices_until_terminator() -> None:
    first = _rom(0x28, b"\x01\x02\x03\x04\x05\x06")
    second = _rom(0x22, b"\x0a\x0b\x0c\x0d\x0e\x0f")
    transport = ScriptedTransport()
    transport.queue_read(b"\x01")
    transport.queue_read(first.data)
    transport.queue_read(second.data)
    transport.queue_read(TERMINATOR)
    assert OneWireController(transport).search_rom() == (first, second)
    assert transport.writes == [b"\x08"]


def test_alarm_search_uses_alarm_command() -> None:
    transport = ScriptedTransport()
    transport.queue_read(b"\x01")
    transport.queue_read(TERMINATOR)
    assert OneWireController(transport).alarm_search() == ()
    assert transport.writes == [b"\x09"]


def test_search_requires_status_acknowledgement() -> None:
    transport = ScriptedTransport([b"\x00"])
    with pytest.raises(ProtocolError, match="failed binary command"):
        OneWireController(transport).search_rom()


def test_search_rejects_more_than_sixty_four_devices_without_terminator() -> None:
    rom = _rom(0x28, b"\x01\x02\x03\x04\x05\x06")
    transport = ScriptedTransport()
    transport.queue_read(b"\x01")
    for _ in range(65):
        transport.queue_read(rom.data)
    with pytest.raises(ProtocolError, match="without a terminator"):
        OneWireController(transport).search_rom()


def test_read_rom_resets_and_reads_eight_bytes() -> None:
    rom = _rom(0x28, b"\x01\x02\x03\x04\x05\x06")
    transport = ScriptedTransport([b"\x01", b"\x01", b"\x01", rom.data])
    assert OneWireController(transport).read_rom() == rom
    assert transport.writes == [b"\x02", b"\x10", b"\x33", *[b"\x04"] * 8]


def test_match_rom_sends_nine_byte_bulk_command() -> None:
    rom = _rom(0x28, b"\x01\x02\x03\x04\x05\x06")
    transport = ScriptedTransport([b"\x01"] * 10)
    OneWireController(transport).match_rom(rom)
    assert transport.writes == [b"\x18", b"\x55", *[bytes([value]) for value in rom.data]]


def test_skip_rom_sends_single_byte_bulk_command() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    OneWireController(transport).skip_rom()
    assert transport.writes == [b"\x10", b"\xcc"]


def test_read_scratchpad_resets_skips_and_reads() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01", b"\x01", b"\x01", b"\x01", b"scratch!!"])
    assert OneWireController(transport).read_scratchpad(None, length=9) == b"scratch!!"
    assert transport.writes == [b"\x02", b"\x10", b"\xcc", b"\x10", b"\xbe", *[b"\x04"] * 9]


def test_read_scratchpad_can_match_a_rom() -> None:
    rom = _rom(0x28, b"\x01\x02\x03\x04\x05\x06")
    transport = ScriptedTransport([b"\x01"] * 13 + [b"\x00" * 9])
    assert OneWireController(transport).read_scratchpad(rom, length=9) == b"\x00" * 9
    assert transport.writes[1] == b"\x18"
    assert transport.writes[2] == b"\x55"
    assert transport.writes[3:11] == [bytes([value]) for value in rom.data]


def test_write_scratchpad_sends_command_and_data() -> None:
    transport = ScriptedTransport([b"\x01"] * 8)
    OneWireController(transport).write_scratchpad(None, b"\x01\x02\x03")
    assert transport.writes == [b"\x02", b"\x10", b"\xcc", b"\x13", b"\x4e", b"\x01", b"\x02", b"\x03"]


def test_copy_scratchpad_sends_copy_command() -> None:
    transport = ScriptedTransport([b"\x01"] * 5)
    OneWireController(transport).copy_scratchpad(None)
    assert transport.writes == [b"\x02", b"\x10", b"\xcc", b"\x10", b"\x48"]


def test_read_memory_sends_address_and_reads_bytes() -> None:
    transport = ScriptedTransport([b"\x01"] * 7 + [b"\xaa\xbb"])
    assert OneWireController(transport).read_memory(None, address=0x0123, length=2) == b"\xaa\xbb"
    assert transport.writes == [
        b"\x02",
        b"\x10",
        b"\xcc",
        b"\x12",
        b"\xf0",
        b"\x01",
        b"\x23",
        b"\x04",
        b"\x04",
    ]


def test_read_status_returns_one_byte() -> None:
    transport = ScriptedTransport([b"\x01"] * 7 + [b"\xa5"])
    assert OneWireController(transport).read_status(None, address=0x0000) == 0xA5
    assert transport.writes == [b"\x02", b"\x10", b"\xcc", b"\x12", b"\xaa", b"\x00", b"\x00", b"\x04"]


def test_context_manager_enters_and_cleans_up() -> None:
    transport = ScriptedTransport([b"BBIO1", b"1W01", b"BBIO1", b"\x01"])
    with OneWireController(transport):
        pass
    assert transport.writes == [ENTER, b"\x04", *CLEANUP]


def test_write_bytes_rejects_empty_payload() -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        OneWireController(ScriptedTransport()).write_bytes(b"")


def test_read_bytes_rejects_non_positive_count() -> None:
    with pytest.raises(ValueError, match="count must be positive"):
        OneWireController(ScriptedTransport()).read_bytes(0)


def test_read_scratchpad_rejects_non_positive_length() -> None:
    with pytest.raises(ValueError, match="length must be positive"):
        OneWireController(ScriptedTransport()).read_scratchpad(None, length=0)


def test_write_scratchpad_rejects_empty_data() -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        OneWireController(ScriptedTransport()).write_scratchpad(None, b"")


@pytest.mark.parametrize("address", [-1, 0x10000])
def test_memory_operations_reject_invalid_addresses(address: int) -> None:
    controller = OneWireController(ScriptedTransport())
    with pytest.raises(ValueError, match="0 through 0xFFFF"):
        controller.read_memory(None, address=address, length=1)
    with pytest.raises(ValueError, match="0 through 0xFFFF"):
        controller.read_status(None, address=address)


def test_read_memory_rejects_non_positive_length() -> None:
    with pytest.raises(ValueError, match="length must be positive"):
        OneWireController(ScriptedTransport()).read_memory(None, address=0, length=0)
