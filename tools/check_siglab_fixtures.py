#!/usr/bin/env python3
"""Guard CYD firmware trust logic against known SignalLab fixture lessons."""

from __future__ import annotations

import csv
import json
import os
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SIGNALLAB = ROOT.parent / "CYD-GPIO35-SignalLab"
SIGNALLAB_ROOT = Path(os.environ.get("SIGNALLAB_REPO", DEFAULT_SIGNALLAB))
INDEX_PATH = SIGNALLAB_ROOT / "analysis" / "capture-index.json"

EXPECTED_RATINGS = {
    "best": {
        "20260526-004010_ear_connected",
        "20260526-004038_ear_connected",
        "20260526-003830_ear_connected",
    },
    "maybe": {
        "20260526-003311_finger_connected",
        "20260526-003844_ear_connected",
        "20260526-004055_ear_connected",
    },
    "control": {
        "20260526-003409_none_connected",
        "20260526-003429_none_connected",
        "20260526-003522_none_unplugged",
        "20260526-003538_none_unplugged",
        "20260526-003658_none_connected",
        "20260526-003725_none_connected",
    },
    "ignore": {
        "20260526-003149_finger_connected",
    },
}


def capture_name(summary: dict[str, Any]) -> str:
    return Path(summary["source"]).parent.name


def siglab_row_count(raw_path: Path) -> int:
    with raw_path.open(newline="") as handle:
        return sum(1 for row in csv.DictReader(handle) if row.get("kind") == "siglab")


def warning_starts(summary: dict[str, Any], prefix: str) -> bool:
    return any(str(warning).startswith(prefix) for warning in summary.get("warnings", []))


def lock_review_eligible(summary: dict[str, Any]) -> bool:
    """A conservative offline proxy for "worth reviewing for contact/lock"."""
    if summary.get("is_control") or summary.get("rating_suggestion") in {"control", "ignore"}:
        return False
    if summary.get("clipping_percent", 100.0) > 5.0:
        return False

    segments = summary.get("candidate_segments") or []
    if not segments:
        return False

    best_segment = segments[0]
    if best_segment.get("clipping_percent", 100.0) > 1.0:
        return False
    if best_segment.get("score", 0.0) < 75.0:
        return False
    if best_segment.get("step_p95", 999.0) > 80.0:
        return False
    return True


def main() -> int:
    if not INDEX_PATH.exists():
        raise SystemExit(f"Missing SignalLab index: {INDEX_PATH}")

    index = json.loads(INDEX_PATH.read_text())
    summaries = {capture_name(summary): summary for summary in index.get("captures", [])}
    missing = sorted(set().union(*EXPECTED_RATINGS.values()) - summaries.keys())
    if missing:
        raise SystemExit(f"SignalLab fixture index is missing captures: {', '.join(missing)}")

    failures: list[str] = []

    for rating, expected_names in EXPECTED_RATINGS.items():
        actual_names = {
            name
            for name, summary in summaries.items()
            if summary.get("rating_suggestion") == rating
        }
        if actual_names != expected_names:
            failures.append(
                f"{rating} set mismatch: expected {sorted(expected_names)}, got {sorted(actual_names)}"
            )

    for name, summary in summaries.items():
        raw_path = SIGNALLAB_ROOT / summary["source"]
        if siglab_row_count(raw_path) != summary.get("samples"):
            failures.append(f"{name} summary sample count is not limited to kind=siglab rows")

        good_rows = int(summary.get("status_counts", {}).get("GOOD WAVEFORM", 0))
        if summary.get("is_control"):
            if lock_review_eligible(summary):
                failures.append(f"{name} control capture became lock-review eligible")
            if good_rows and not warning_starts(summary, "CONTROL_FALSE_POSITIVE"):
                failures.append(f"{name} has GOOD WAVEFORM control rows without false-positive warning")

        if summary.get("rating_suggestion") == "ignore" and lock_review_eligible(summary):
            failures.append(f"{name} ignored capture became lock-review eligible")

        if summary.get("rating_suggestion") == "best" and not lock_review_eligible(summary):
            failures.append(f"{name} best ear capture is no longer eligible for contact/lock review")

        if good_rows and (
            summary.get("is_control")
            or summary.get("clipping_percent", 0.0) > 5.0
            or warning_starts(summary, "MOVING_DOMINANT")
        ) and lock_review_eligible(summary):
            failures.append(f"{name} GOOD WAVEFORM overrode label, clipping, or movement warnings")

    if failures:
        raise SystemExit("SignalLab fixture guard failed: " + "; ".join(failures))

    print(f"SignalLab fixture checks passed ({len(summaries)} captures from {SIGNALLAB_ROOT})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
