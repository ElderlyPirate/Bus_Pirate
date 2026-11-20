# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for frozen EEPROM and connection value objects."""

from __future__ import annotations

import pytest

from bp.types import AddressWidth, ChipAddress, ConnectionSettings, EepromLayout


def test_chip_address_bounds_and_i2c_addresses() -> None:
    assert ChipAddress(0).write_address == 0xA0
    assert ChipAddress(7).write_address == 0xAE


@pytest.mark.parametrize("value", [-1, 8])
def test_chip_address_rejects_out_of_range(value: int) -> None:
    with pytest.raises(ValueError, match="0 through 7"):
        ChipAddress(value)


@pytest.mark.parametrize("address", [-1, 0x10000])
def test_layout_rejects_memory_address_outside_word_range(address: int) -> None:
    with pytest.raises(ValueError, match="0 through 0xFFFF"):
        EepromLayout(ChipAddress(0), AddressWidth.TWO_BYTE, address)


def test_one_byte_layout_accepts_boundary_address() -> None:
    layout = EepromLayout(ChipAddress(0), AddressWidth.ONE_BYTE, 0x7FF)
    layout.ensure_span(1)


def test_one_byte_layout_rejects_boundary_overflow() -> None:
    with pytest.raises(ValueError, match="0 through 0x7FF"):
        EepromLayout(ChipAddress(0), AddressWidth.ONE_BYTE, 0x800)


@pytest.mark.parametrize("length", [0, -1])
def test_layout_ensure_span_rejects_non_positive_length(length: int) -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        EepromLayout(ChipAddress(0), AddressWidth.TWO_BYTE, 0).ensure_span(length)


def test_layout_ensure_span_rejects_span_past_address_space() -> None:
    with pytest.raises(ValueError, match="extends past"):
        EepromLayout(ChipAddress(0), AddressWidth.TWO_BYTE, 0xFFFF).ensure_span(2)


def test_one_byte_page_bits_are_or_ed_into_device_address() -> None:
    layout = EepromLayout(ChipAddress(0), AddressWidth.ONE_BYTE, 0x400)
    assert layout.write_address_for(0x000) == 0xA0
    assert layout.write_address_for(0x100) == 0xA2
    assert layout.write_address_for(0x200) == 0xA4
    assert layout.write_address_for(0x400) == 0xA8
    assert layout.address_bytes_for(0x123) == b"\x23"


def test_two_byte_addressing_keeps_chip_address_and_sends_high_byte() -> None:
    layout = EepromLayout(ChipAddress(3), AddressWidth.TWO_BYTE, 0x1234)
    assert layout.write_address_for(0x1234) == 0xA6
    assert layout.address_bytes_for(0x1234) == b"\x12\x34"


def test_connection_settings_defaults() -> None:
    settings = ConnectionSettings(port="COM9")
    assert settings.speed == 115_200
    assert settings.read_timeout > 0
    assert settings.write_timeout > 0
