#!/usr/bin/env python3
"""Read canonical firmware metadata from the Arduino sketch."""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_SOURCE = ROOT / "PulseSensor_CYD.ino"
PLATFORMIO_CONFIG = ROOT / "platformio.ini"


@dataclass(frozen=True)
class FirmwareMetadata:
    version: str
    diagnostic_version: str
    firmware_date: str
    build_ram_usage: str
    build_flash_usage: str


def _read_define(source: str, name: str) -> str:
    match = re.search(rf'#define\s+{re.escape(name)}\s+"([^"]+)"', source)
    if not match:
        raise SystemExit(f"Could not find {name} in {FIRMWARE_SOURCE.name}")
    return match.group(1)


def _read_diag_version(platformio: str) -> str:
    match = re.search(r'-D\s+APP_VERSION=(?:\\"([^"]+)\\"|\'"([^"]+)"\')', platformio)
    if not match:
        raise SystemExit("Could not find diagnostic APP_VERSION override in platformio.ini")
    return next(value for value in match.groups() if value is not None)


def read_firmware_metadata() -> FirmwareMetadata:
    source = FIRMWARE_SOURCE.read_text()
    platformio = PLATFORMIO_CONFIG.read_text()
    return FirmwareMetadata(
        version=_read_define(source, "APP_VERSION"),
        diagnostic_version=_read_diag_version(platformio),
        firmware_date=_read_define(source, "APP_FIRMWARE_DATE"),
        build_ram_usage=_read_define(source, "APP_BUILD_RAM_USAGE"),
        build_flash_usage=_read_define(source, "APP_BUILD_FLASH_USAGE"),
    )


def main() -> int:
    metadata = read_firmware_metadata()
    print(f"version={metadata.version}")
    print(f"diagnostic_version={metadata.diagnostic_version}")
    print(f"firmware_date={metadata.firmware_date}")
    print(f"build_ram_usage={metadata.build_ram_usage}")
    print(f"build_flash_usage={metadata.build_flash_usage}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
