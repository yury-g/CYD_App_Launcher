#!/usr/bin/env python3
"""Check metadata and web-installer release hygiene."""

from __future__ import annotations

import json
from pathlib import Path

from project_metadata import read_firmware_metadata


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    metadata = read_firmware_metadata()
    manifest = json.loads((ROOT / "manifest.json").read_text())
    readme = (ROOT / "README.md").read_text()

    if metadata.version not in readme:
        raise SystemExit("README should name the current firmware version")
    if metadata.diagnostic_version not in (ROOT / "platformio.ini").read_text():
        raise SystemExit("platformio.ini should expose the diagnostic firmware version")
    if "web-installer artifacts" not in readme:
        raise SystemExit("README should mark firmware/ binaries as web-installer artifacts")
    if manifest.get("version") == metadata.version:
        raise SystemExit("manifest.json unexpectedly matches source firmware; regenerate intentionally for web publish")
    for build in manifest.get("builds", []):
        for part in build.get("parts", []):
            path = ROOT / part["path"]
            if not path.exists():
                raise SystemExit(f"manifest references missing binary: {part['path']}")

    print(
        "Release hygiene checks passed "
        f"(source={metadata.version}, manifest={manifest.get('version')})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
