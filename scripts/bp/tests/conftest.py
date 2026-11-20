# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Shared fake serial and transport fixtures for hardware-free tests."""

from __future__ import annotations

from collections import deque
from collections.abc import Iterable

import pytest

from bp.errors import TransportTimeout
from bp.types import ConnectionSettings


class ScriptedSerial:
    """Fake pyserial port that answers each write with a scripted byte chunk."""

    def __init__(self, responses: Iterable[bytes] = ()) -> None:
        self.responses: deque[bytes] = deque(responses)
        self.received: bytearray = bytearray()
        self.writes: list[bytes] = []
        self.reset_count = 0
        self.closed = False
        self.write_shortfall = 0
        self.write_raises = False
        self.report_no_count = False
        self.read_raises = False
        self.reset_raises = False
        self.close_raises = False

    def feed(self, data: bytes) -> None:
        self.received.extend(data)

    def write(self, data: bytes, /) -> int | None:
        if self.write_raises:
            raise OSError("scripted write failure")
        self.writes.append(bytes(data))
        if self.responses:
            self.received.extend(self.responses.popleft())
        if self.report_no_count:
            return None
        return len(data) - self.write_shortfall

    def read(self, size: int, /) -> bytes:
        if self.read_raises:
            raise OSError("scripted read failure")
        chunk = bytes(self.received[:size])
        del self.received[:size]
        return chunk

    def reset_input_buffer(self) -> None:
        if self.reset_raises:
            raise OSError("scripted reset failure")
        self.received.clear()
        self.reset_count += 1

    def close(self) -> None:
        if self.close_raises:
            raise OSError("scripted close failure")
        self.closed = True

    def assert_consumed(self) -> None:
        assert not self.responses


class ScriptedTransport:
    """Fake byte transport with one scripted response per write."""

    def __init__(self, responses: Iterable[bytes] = ()) -> None:
        self.responses: deque[bytes] = deque(responses)
        self.forced_reads: deque[bytes] = deque()
        self.received: bytearray = bytearray()
        self.writes: list[bytes] = []
        self.reset_count = 0

    def feed(self, data: bytes) -> None:
        self.received.extend(data)

    def queue_read(self, data: bytes) -> None:
        self.forced_reads.append(bytes(data))

    def write(self, data: bytes) -> None:
        self.writes.append(bytes(data))
        if self.responses:
            self.received.extend(self.responses.popleft())

    def read_exact(self, size: int) -> bytes:
        if self.forced_reads:
            chunk = self.forced_reads.popleft()
            if len(chunk) != size:
                raise TransportTimeout(f"forced read has {len(chunk)} of {size} bytes")
            return chunk
        if len(self.received) < size:
            raise TransportTimeout(f"fake transport has {len(self.received)} of {size} bytes")
        chunk = bytes(self.received[:size])
        del self.received[:size]
        return chunk

    def read_until_idle(self, *, max_bytes: int) -> bytes:
        chunk = bytes(self.received[:max_bytes])
        del self.received[: len(chunk)]
        return chunk

    def reset_input(self) -> None:
        self.received.clear()
        self.reset_count += 1

    def assert_consumed(self) -> None:
        assert not self.responses


@pytest.fixture
def connection() -> ConnectionSettings:
    """Return deterministic serial settings for tests."""
    return ConnectionSettings(port="COM9", speed=115_200)
