# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for optional YAML configuration and connection resolution."""

from __future__ import annotations

from pathlib import Path

import pytest

from bp.config import AppContext, FileConfig, default_serial_port, load_config
from bp.errors import CommandError, ConfigError
from bp.types import DEFAULT_SERIAL_SPEED, ConnectionSettings


def test_missing_config_path_returns_empty_model() -> None:
    config = load_config(None)
    assert config.port is None
    assert config.speed is None


def test_load_config_reads_port_and_speed(tmp_path: Path) -> None:
    path = tmp_path / "bp.yaml"
    path.write_text("port: COM7\nspeed: 57600\n", encoding="utf-8")
    config = load_config(path)
    assert config == FileConfig(port="COM7", speed=57_600)


def test_load_config_rejects_unknown_keys(tmp_path: Path) -> None:
    path = tmp_path / "bp.yaml"
    path.write_text("port: COM7\ni2c_speed: 400000\n", encoding="utf-8")
    with pytest.raises(ConfigError, match="Invalid configuration"):
        load_config(path)


def test_load_config_rejects_wrong_types(tmp_path: Path) -> None:
    path = tmp_path / "bp.yaml"
    path.write_text("speed: fast\n", encoding="utf-8")
    with pytest.raises(ConfigError, match="Invalid configuration"):
        load_config(path)


@pytest.mark.parametrize("body", ["port: ''\n", "speed: 0\n"])
def test_load_config_rejects_invalid_scalar_values(tmp_path: Path, body: str) -> None:
    path = tmp_path / "bp.yaml"
    path.write_text(body, encoding="utf-8")
    with pytest.raises(ConfigError, match="Invalid configuration"):
        load_config(path)


def test_load_config_rejects_non_mapping(tmp_path: Path) -> None:
    path = tmp_path / "bp.yaml"
    path.write_text("- COM7\n- 57600\n", encoding="utf-8")
    with pytest.raises(ConfigError, match="must contain a YAML mapping"):
        load_config(path)


def test_load_config_rejects_empty_document(tmp_path: Path) -> None:
    path = tmp_path / "bp.yaml"
    path.write_text("\n", encoding="utf-8")
    with pytest.raises(ConfigError, match="must contain a YAML mapping"):
        load_config(path)


def test_load_config_rejects_malformed_yaml(tmp_path: Path) -> None:
    path = tmp_path / "bp.yaml"
    path.write_text("port: [COM7\n", encoding="utf-8")
    with pytest.raises(ConfigError, match="Could not parse YAML"):
        load_config(path)


def test_load_config_wraps_missing_file(tmp_path: Path) -> None:
    with pytest.raises(ConfigError, match="Could not read configuration"):
        load_config(tmp_path / "missing.yaml")


def test_load_config_rejects_invalid_utf8(tmp_path: Path) -> None:
    path = tmp_path / "bp.yaml"
    path.write_bytes(b"port: \xff\n")
    with pytest.raises(ConfigError, match="Could not decode configuration"):
        load_config(path)


def test_connection_prefers_cli_over_config() -> None:
    app = AppContext(config=FileConfig(port="COM1", speed=9600))
    connection = app.resolve_connection(port="COM9", speed=57_600)
    assert connection == ConnectionSettings(port="COM9", speed=57_600)


def test_connection_uses_config_then_speed_default() -> None:
    app = AppContext(config=FileConfig(port="COM4"))
    assert app.resolve_connection(port=None, speed=None) == ConnectionSettings(port="COM4", speed=DEFAULT_SERIAL_SPEED)


def test_connection_uses_explicit_fallback_port() -> None:
    app = AppContext(config=FileConfig())
    assert app.resolve_connection(port=None, speed=None, fallback_port="COM1") == ConnectionSettings(port="COM1")


def test_connection_requires_port_without_fallback() -> None:
    app = AppContext(config=FileConfig())
    with pytest.raises(CommandError, match="No serial port specified"):
        app.resolve_connection(port=None, speed=None)


@pytest.mark.parametrize("port", ["", "   "])
def test_connection_rejects_empty_port(port: str) -> None:
    app = AppContext(config=FileConfig())
    with pytest.raises(CommandError, match="Serial port must not be empty"):
        app.resolve_connection(port=port, speed=None)


def test_default_serial_port_is_nonempty() -> None:
    assert default_serial_port()
