#!/usr/bin/env python3
"""Run the current app-shell p2p integration checks."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INO = ROOT / "PulseSensor_CYD.ino"
PLATFORMIO = ROOT / "platformio.ini"


REQUIRED_INO_TOKENS = {
    '#define APP_VERSION "0.5.17-p2papps"': "visible firmware version must identify this app-shell p2p build",
    '#define APP_FIRMWARE_DATE "2026-05-26"': "visible firmware date should match this integration pass",
    "#define LOCK_QUALIFIED_BEATS 3": "successful on-device p2p branch acquired after three qualified beats",
    "#define VOLUME_START 0": "sound should default off/conservative",
    "#define PEAK_TO_PEAK_MIN_BPM 60": "p2p path should use human-range lower BPM",
    "#define PEAK_TO_PEAK_MAX_BPM 200": "p2p path should use human-range upper BPM",
    "#define PEAK_TO_PEAK_MIN_IBI 300": "p2p path should reject intervals faster than 200 BPM",
    "#define PEAK_TO_PEAK_MAX_IBI 1000": "p2p path should reject intervals slower than 60 BPM",
    "#define PEAK_TO_PEAK_PRIMARY_MIN_SCORE 6": "p2p score should stay tight",
    "#define PEAK_TO_PEAK_FIRST_BEAT_SCORE 7": "first p2p beat should need stronger evidence",
    "#define SIGNAL_COACH_ARTIFACT_RANGE 900": "artifact range guard should be high enough for finger/ear p2p",
    "bool peakToPeakAccepted = isPeakToPeakCandidateBeat": "read path should try p2p before strict beat math",
    'lastBeatAcceptReason = peakToPeakAccepted ? "p2p" : "strict";': "serial should explain p2p vs strict acceptance",
    "noteGraphBeatMarker": "waveform should mark accepted/rejected beat events",
    "MARKER_P2P": "graph marker legend should include p2p events",
    "signal=%d amp=%d bpm=%d ibi=%d locked=%d quality=%d p2p=%d range=%d clip=%d jump=%d qStreak=%d accept=%s drop=%s": "serial summary should include p2p, movement, accept, and drop fields",
}

FORBIDDEN_INO_TOKENS = {
    "ACQUISITION_CADENCE_TOLERANCE_PERCENT": "successful branch removed acquisition cadence gates from p2p acquisition",
    "LOCK_GRACE_BAD_BEATS": "successful branch did not use lock grace to preserve BPM/IBI",
    "decision.peakToPeakAccepted": "old app-shell BeatDecision path should not drive acquisition",
    "PEAK_TO_PEAK_ACQUIRE_MIN_SCORE": "old loose score macro should not be used",
    "SIGNAL_MOTION_ARTIFACT_RANGE 420": "old artifact range was too restrictive for this p2p build",
}

REQUIRED_PLATFORMIO_TOKENS = {
    "upload_port = /dev/cu.usbserial-10": "default CYD upload port should match the connected tester",
    '-D APP_VERSION=\\"0.5.17-p2papps-log\\"': "diagnostic build should carry the same version family",
    "-D RAW_SIGNAL_DIAGNOSTICS=1": "diagnostic build should still enable raw CSV logging",
}


def require_tokens(text: str, tokens: dict[str, str], source: str, failures: list[str]) -> None:
    for token, message in tokens.items():
        if token not in text:
            failures.append(f"{source}: {message}")


def forbid_tokens(text: str, tokens: dict[str, str], source: str, failures: list[str]) -> None:
    for token, message in tokens.items():
        if token in text:
            failures.append(f"{source}: {message}")


def check_settings_version_width(text: str, failures: list[str]) -> None:
    match = re.search(r'#define APP_VERSION "([^"]+)"', text)
    if not match:
        failures.append("PulseSensor_CYD.ino: APP_VERSION macro missing")
        return
    version = match.group(1)
    if len(version) * 12 > 190:
        failures.append("PulseSensor_CYD.ino: APP_VERSION is too wide for Settings size-2 text")


def main() -> int:
    ino = INO.read_text()
    platformio = PLATFORMIO.read_text()
    failures: list[str] = []

    require_tokens(ino, REQUIRED_INO_TOKENS, "PulseSensor_CYD.ino", failures)
    forbid_tokens(ino, FORBIDDEN_INO_TOKENS, "PulseSensor_CYD.ino", failures)
    require_tokens(platformio, REQUIRED_PLATFORMIO_TOKENS, "platformio.ini", failures)
    check_settings_version_width(ino, failures)

    if failures:
        raise SystemExit("Project guard failed: " + "; ".join(failures))

    subprocess.run([sys.executable, "tools/check_siglab_fixtures.py"], cwd=ROOT, check=True)
    print("Project checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
