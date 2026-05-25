#!/usr/bin/env python3
"""Run the default project checks for the current CYD firmware baseline."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

CHECKS = [
    "tools/project_metadata.py",
    "tools/check_release_hygiene.py",
]


def main() -> int:
    for check in CHECKS:
        print(f"==> python3 {check}")
        subprocess.run([sys.executable, check], cwd=ROOT, check=True)
    print("Project checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
