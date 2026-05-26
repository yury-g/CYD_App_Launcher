#!/usr/bin/env python3
"""Run the default project checks for the CYD firmware."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

CHECKS = [
    "tools/check_app_shell.py",
    "tools/check_core_polish.py",
    "tools/check_clipping_quality_guard.py",
    "tools/check_signal_diagnostics.py",
    "tools/check_peak_to_peak_recovery.py",
    "tools/check_resume_flash_entrypoint.py",
    "tools/check_release_hygiene.py",
    "tools/sync_origin_story_crawl.py",
]


def main() -> int:
    for check in CHECKS:
        print(f"==> python3 {check}")
        subprocess.run([sys.executable, check], cwd=ROOT, check=True)
    print("Project checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
