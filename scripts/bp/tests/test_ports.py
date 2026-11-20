# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for serial-port enumeration and its external boundary validation."""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass

import pytest

from bp.errors import CommandError
from bp.ports import BusPiratePortKind, SerialPortInfo, list_serial_ports


@dataclass(frozen=True, slots=True)
class FakePort:
    """External port object with deliberately untyped fields."""

    device: object
    vid: object
    pid: object
    description: object


def _port(
    *,
    device: object = "COM6",
    vid: object = 0x04D8,
    pid: object = 0xFB00,
    description: object = "USB Serial Device",
) -> FakePort:
    return FakePort(device=device, vid=vid, pid=pid, description=description)


class DeviceOnly:
    """External port object without a description."""

    device = "COM1"
    vid = None
    pid = None


class DeviceAndDescription:
    """External port object without USB IDs."""

    device = "COM1"
    description = "Communications Port"


def test_list_serial_ports_narrows_and_classifies_known_ids() -> None:
    ports = list_serial_ports(
        lister=lambda: (
            _port(device="COM6", vid=0x04D8, pid=0xFB00, description="Bus Pirate v4"),
            _port(device="COM7", vid=0x04D8, pid=0xFAFF, description="Bus Pirate v4 bootloader"),
            _port(device="COM3", vid=0x0403, pid=0x6001, description="USB Serial Port"),
            _port(device="COM2", vid=0x1234, pid=0x5678, description="Other USB serial"),
            _port(device="COM1", vid=None, pid=None, description="Communications Port"),
        )
    )

    assert ports == (
        SerialPortInfo(device="COM6", vid=0x04D8, pid=0xFB00, description="Bus Pirate v4"),
        SerialPortInfo(device="COM7", vid=0x04D8, pid=0xFAFF, description="Bus Pirate v4 bootloader"),
        SerialPortInfo(device="COM3", vid=0x0403, pid=0x6001, description="USB Serial Port"),
        SerialPortInfo(device="COM2", vid=0x1234, pid=0x5678, description="Other USB serial"),
        SerialPortInfo(device="COM1", vid=None, pid=None, description="Communications Port"),
    )
    assert [port.vid_pid for port in ports] == ["04D8:FB00", "04D8:FAFF", "0403:6001", "1234:5678", None]
    assert [port.bus_pirate_kind for port in ports] == [
        BusPiratePortKind.V4_APPLICATION,
        BusPiratePortKind.V4_BOOTLOADER,
        BusPiratePortKind.V3_CANDIDATE,
        None,
        None,
    ]


def test_list_serial_ports_returns_empty_tuple_when_no_ports_exist() -> None:
    assert list_serial_ports(lister=lambda: ()) == ()


def test_list_serial_ports_wraps_enumeration_failure() -> None:
    def failing_lister() -> Iterable[object]:
        raise OSError("registry unavailable")

    with pytest.raises(CommandError, match="Could not enumerate serial ports: registry unavailable"):
        list_serial_ports(lister=failing_lister)


@pytest.mark.parametrize(
    ("entry", "message"),
    [
        (object(), "missing the required 'device' attribute"),
        (DeviceOnly(), "missing the required 'description' attribute"),
        (DeviceAndDescription(), "missing the required 'vid' attribute"),
        (_port(device=123), "non-string 'device' value"),
        (_port(device=""), "device must not be empty"),
        (_port(description=123), "non-string 'description' value"),
        (_port(vid="04D8"), r"invalid 'vid' value: '04D8'"),
        (_port(vid=True), r"invalid 'vid' value: True"),
        (_port(vid=0x10000), "VID must be 0 through 0xFFFF, got 65536"),
        (_port(pid=0x10000), "PID must be 0 through 0xFFFF, got 65536"),
        (_port(vid=0x04D8, pid=None), "VID and PID must be provided together"),
        (_port(vid=None, pid=0xFB00), "VID and PID must be provided together"),
    ],
)
def test_list_serial_ports_rejects_invalid_external_entries(entry: object, message: str) -> None:
    with pytest.raises(CommandError, match=message):
        list_serial_ports(lister=lambda: (entry,))


def test_serial_port_info_rejects_invalid_usb_id_ranges() -> None:
    with pytest.raises(ValueError, match="VID must be 0 through 0xFFFF"):
        SerialPortInfo(device="COM1", vid=-1, pid=0x6001, description="USB Serial Port")
    with pytest.raises(ValueError, match="PID must be 0 through 0xFFFF"):
        SerialPortInfo(device="COM1", vid=0x0403, pid=0x10000, description="USB Serial Port")
