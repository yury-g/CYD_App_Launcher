#!/usr/bin/env python3
"""Flash the current favorite CYD firmware from a fresh repo checkout.

This helper is intentionally self-contained so another AI agent or developer can
clone yury-g/CYD_App_Launcher on another computer and flash the known favorite
device state without needing this chat history.
"""

from __future__ import annotations

import argparse
import glob
import os
import shutil
import subprocess
import sys
from pathlib import Path

from project_metadata import read_firmware_metadata


INTERNAL_REPO_URL = "https://github.com/yury-g/CYD_App_Launcher"
FAVORITE_REF = "main"
FAVORITE_BRANCH = "main"
DEFAULT_ENV = "cyd"
COMMON_SERIAL_GLOBS = [
    "/dev/cu.usbserial*",
    "/dev/cu.SLAB_USBtoUART*",
    "/dev/cu.wchusbserial*",
    "/dev/ttyUSB*",
    "/dev/ttyACM*",
]


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def read_app_version() -> str:
    return read_firmware_metadata().version


def detect_port() -> str | None:
    candidates: list[str] = []
    for pattern in COMMON_SERIAL_GLOBS:
        candidates.extend(glob.glob(pattern))
    candidates = sorted(set(candidates))
    if len(candidates) == 1:
        return candidates[0]
    if candidates:
        print("Multiple serial ports found; choose one with --port:")
        for candidate in candidates:
            print(f"  {candidate}")
    return None


def command_to_string(command: list[str]) -> str:
    return " ".join(command)


def run(command: list[str], *, env: dict[str, str], dry_run: bool) -> None:
    print(command_to_string(command))
    if dry_run:
        return
    subprocess.run(command, cwd=repo_root(), env=env, check=True)


def build_env(platformio_core_dir: str | None) -> dict[str, str]:
    env = os.environ.copy()
    if platformio_core_dir:
        env["PLATFORMIO_CORE_DIR"] = platformio_core_dir
    return env


def parse_args() -> argparse.Namespace:
    favorite_version = read_firmware_metadata().version
    parser = argparse.ArgumentParser(
        description=(
            "Build and flash the current favorite CYD firmware. "
            f"Favorite: {favorite_version} on {FAVORITE_BRANCH} / {FAVORITE_REF}."
        )
    )
    parser.add_argument("--port", help="Serial port, for example /dev/cu.usbserial-* or COM4")
    parser.add_argument("--env", default=DEFAULT_ENV, help=f"PlatformIO env, default {DEFAULT_ENV}")
    parser.add_argument("--pio", default="pio", help="PlatformIO executable, default: pio")
    parser.add_argument(
        "--platformio-core-dir",
        help="Optional PLATFORMIO_CORE_DIR override for shared dependency caches",
    )
    parser.add_argument(
        "--allow-version-mismatch",
        action="store_true",
        help="Flash the current checkout even if APP_VERSION is not the favorite version",
    )
    parser.add_argument("--dry-run", action="store_true", help="Print commands without running them")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    app_version = read_app_version()
    favorite_version = read_firmware_metadata().version

    print(f"Internal repo: {INTERNAL_REPO_URL}")
    print(f"Favorite firmware: {favorite_version}")
    print(f"Current checkout firmware: {app_version}")

    if app_version != favorite_version and not args.allow_version_mismatch:
        print(
            "\nThis checkout is not the current favorite firmware. "
            f"Check out {FAVORITE_BRANCH} or {FAVORITE_REF}, then run this helper again.\n"
            "Use --allow-version-mismatch only for deliberate branch tests.",
            file=sys.stderr,
        )
        return 2

    pio = shutil.which(args.pio)
    if pio is None and args.dry_run:
        pio = args.pio
    elif pio is None:
        print(
            "PlatformIO executable was not found. Install PlatformIO or pass --pio /path/to/pio.",
            file=sys.stderr,
        )
        return 2

    port = args.port or detect_port()
    if not port:
        print("No single CYD serial port was detected. Re-run with --port <serial-port>.", file=sys.stderr)
        return 2

    env = build_env(args.platformio_core_dir)
    build = [pio, "run", "-e", args.env]
    upload = [pio, "run", "-e", args.env, "-t", "upload", "--upload-port", port]

    run(build, env=env, dry_run=args.dry_run)
    run(upload, env=env, dry_run=args.dry_run)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
