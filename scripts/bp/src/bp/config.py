"""Optional YAML configuration and CLI connection resolution."""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path

from pydantic import BaseModel, ConfigDict, Field, ValidationError
from ruamel.yaml import YAML
from ruamel.yaml.error import YAMLError

from bp.errors import CommandError, ConfigError
from bp.types import DEFAULT_SERIAL_SPEED, ConnectionSettings


class FileConfig(BaseModel):
    """Validated contents of the optional YAML configuration file."""

    model_config = ConfigDict(extra="forbid", frozen=True, strict=True)

    port: str | None = Field(default=None, min_length=1)
    speed: int | None = Field(default=None, gt=0)


@dataclass(frozen=True, slots=True)
class AppContext:
    """Configuration shared by commands through the Click context."""

    config: FileConfig

    def resolve_connection(
        self,
        *,
        port: str | None,
        speed: int | None,
        fallback_port: str | None = None,
    ) -> ConnectionSettings:
        """Resolve CLI options, configuration values, and an optional port fallback."""
        resolved_port = port if port is not None else self.config.port
        if resolved_port is None:
            resolved_port = fallback_port
        if resolved_port is None:
            raise CommandError("No serial port specified; pass --port or set 'port' in the configuration file")
        resolved_port = resolved_port.strip()
        if not resolved_port:
            raise CommandError("Serial port must not be empty")

        resolved_speed = speed if speed is not None else self.config.speed
        if resolved_speed is None:
            resolved_speed = DEFAULT_SERIAL_SPEED
        try:
            return ConnectionSettings(port=resolved_port, speed=resolved_speed)
        except ValueError as exc:
            raise CommandError(str(exc)) from exc


def load_config(path: Path | None) -> FileConfig:
    """Load and validate an optional YAML configuration file.

    Raises:
        ConfigError: If the file cannot be read, is not a mapping, or fails
            strict schema validation.
    """
    if path is None:
        return FileConfig()

    yaml = YAML(typ="safe")
    try:
        with path.open("r", encoding="utf-8") as stream:
            document = yaml.load(stream)
    except OSError as exc:
        raise ConfigError(f"Could not read configuration file {path}: {exc}") from exc
    except UnicodeError as exc:
        raise ConfigError(f"Could not decode configuration file {path} as UTF-8: {exc}") from exc
    except YAMLError as exc:
        raise ConfigError(f"Could not parse YAML configuration file {path}: {exc}") from exc

    if not isinstance(document, dict):
        raise ConfigError(f"Configuration file {path} must contain a YAML mapping")

    try:
        return FileConfig.model_validate(document)
    except ValidationError as exc:
        raise ConfigError(f"Invalid configuration file {path}: {exc}") from exc


def default_serial_port() -> str:
    """Return the platform default serial port used by the legacy version tool."""
    if sys.platform.startswith("win"):
        return "COM1"
    if sys.platform.startswith("linux"):
        return "/dev/ttyACM0"
    if sys.platform == "darwin":
        return "/dev/cu.usbmodem00000001"
    return "/dev/ttyUSB0"
