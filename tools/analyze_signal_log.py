#!/usr/bin/env python3
import argparse
import csv
import statistics
from pathlib import Path


NUMERIC_FIELDS = {
    "ms", "signal", "amp", "bpm", "ibi", "locked", "quality", "p2p",
    "range", "clip", "inside", "beat", "qStreak", "badStreak",
}


def load_rows(path):
    rows = []
    with Path(path).open(newline="") as f:
        for row in csv.DictReader(f):
            if row.get("rawDiag") != "rawDiag":
                continue
            parsed = dict(row)
            for field in NUMERIC_FIELDS:
                parsed[field] = int(parsed[field])
            rows.append(parsed)
    return rows


def rolling_mean(values, index, radius):
    start = max(0, index - radius)
    end = min(len(values), index + radius + 1)
    return sum(values[start:end]) / max(1, end - start)


def median_abs_diff(values):
    if len(values) < 2:
        return 0
    diffs = [abs(values[i] - values[i - 1]) for i in range(1, len(values))]
    return statistics.median(diffs)


def independent_peaks(rows):
    if len(rows) < 5:
        return []

    signals = [row["signal"] for row in rows]
    detrended = [signals[i] - rolling_mean(signals, i, 12) for i in range(len(signals))]
    smoothed = [rolling_mean(detrended, i, 2) for i in range(len(detrended))]
    noise = median_abs_diff(smoothed)
    threshold = max(12, noise * 3)
    min_spacing_ms = 360

    peaks = []
    last_peak_ms = -10_000
    for i in range(2, len(smoothed) - 2):
        value = smoothed[i]
        if value < threshold:
            continue
        if not (value >= smoothed[i - 1] and value > smoothed[i + 1]):
            continue
        if value < max(smoothed[i - 2], smoothed[i + 2]):
            continue
        ms = rows[i]["ms"]
        if ms - last_peak_ms < min_spacing_ms:
            if peaks and value > peaks[-1]["prominence"]:
                peaks[-1] = {"ms": ms, "signal": rows[i]["signal"], "prominence": value}
                last_peak_ms = ms
            continue
        peaks.append({"ms": ms, "signal": rows[i]["signal"], "prominence": value})
        last_peak_ms = ms
    return peaks


def ibis_from_peaks(peaks):
    return [peaks[i]["ms"] - peaks[i - 1]["ms"] for i in range(1, len(peaks))]


def summarize_ibis(label, ibis):
    plausible = [ibi for ibi in ibis if 333 <= ibi <= 1500]
    if not plausible:
        print(f"{label}: no plausible IBIs")
        return
    bpm = [60000 / ibi for ibi in plausible]
    print(
        f"{label}: count={len(plausible)} "
        f"IBI median={statistics.median(plausible):.0f} ms "
        f"BPM median={statistics.median(bpm):.1f} "
        f"IBI range={min(plausible)}-{max(plausible)} ms"
    )


def motion_noise_windows(rows, window_ms=3000):
    windows = []
    if not rows:
        return windows
    start = rows[0]["ms"]
    current = []
    for row in rows:
        if row["ms"] - start <= window_ms:
            current.append(row)
            continue
        windows.append(current)
        start = row["ms"]
        current = [row]
    if current:
        windows.append(current)

    summaries = []
    for window in windows:
        signals = [row["signal"] for row in window]
        clipped = sum(1 for row in window if row["clip"] > 18)
        beats = sum(row["beat"] for row in window)
        accepts = [row["accept"] for row in window if row["beat"]]
        signal_span = max(signals) - min(signals)
        step_noise = median_abs_diff(signals)
        is_noisy = clipped > 0 or step_noise > 45 or signal_span > 420
        summaries.append({
            "start": window[0]["ms"],
            "end": window[-1]["ms"],
            "span": signal_span,
            "step_noise": step_noise,
            "clipped_rows": clipped,
            "beats": beats,
            "accepts": accepts,
            "is_noisy": is_noisy,
        })
    return summaries


def main():
    parser = argparse.ArgumentParser(description="Analyze a rawDiag signal-log CSV and compare firmware beats with independent peaks.")
    parser.add_argument("csv_path", type=Path)
    args = parser.parse_args()

    rows = load_rows(args.csv_path)
    if not rows:
        raise SystemExit("no rawDiag rows found")

    duration_s = (rows[-1]["ms"] - rows[0]["ms"]) / 1000
    firmware_beats = [row for row in rows if row["beat"]]
    accepted = [row for row in firmware_beats if row["accept"] not in ("none", "reject")]
    rejected = [row for row in firmware_beats if row["accept"] == "reject"]
    peaks = independent_peaks(rows)

    print(f"file: {args.csv_path}")
    print(f"duration: {duration_s:.1f}s rows={len(rows)} firmware_beats={len(firmware_beats)} accepted={len(accepted)} rejected={len(rejected)} independent_peaks={len(peaks)}")
    print(f"signal range overall: {min(row['signal'] for row in rows)}-{max(row['signal'] for row in rows)}")
    print(f"clip rows: {sum(1 for row in rows if row['clip'] > 18)}")
    print(f"accept reasons: {dict((reason, [row['accept'] for row in firmware_beats].count(reason)) for reason in sorted(set(row['accept'] for row in firmware_beats)))}")

    summarize_ibis("firmware accepted", [accepted[i]["ms"] - accepted[i - 1]["ms"] for i in range(1, len(accepted))])
    summarize_ibis("independent peaks", ibis_from_peaks(peaks))

    noisy = [w for w in motion_noise_windows(rows) if w["is_noisy"]]
    print(f"motion/noise windows: {len(noisy)}")
    for window in noisy[:8]:
        print(
            f"  {window['start']}-{window['end']}ms "
            f"span={window['span']} stepNoise={window['step_noise']:.1f} "
            f"clipRows={window['clipped_rows']} beats={window['beats']} accepts={window['accepts']}"
        )


if __name__ == "__main__":
    main()
