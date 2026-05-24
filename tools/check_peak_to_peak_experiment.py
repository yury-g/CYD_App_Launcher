from pathlib import Path

source = Path("PulseSensor_CYD.ino").read_text()

required = {
    "#define PEAK_TO_PEAK_EXPERIMENT 1": "experiment switch must be enabled",
    '#define APP_VERSION "0.4.25-stable-wave"': "firmware version must identify the stable waveform build",
    "bool isPeakToPeakCandidateBeat": "candidate helper is missing",
    "int peakToPeakScoreForCurrentSignal": "acquisition scoring hook is missing",
    "decision.peakToPeakAccepted = PEAK_TO_PEAK_EXPERIMENT &&": "read path must use peak-to-peak acceptance",
    '"peak2peak"': "serial accept reason must include peak2peak",
    "p2p=%d": "serial telemetry must include the peak-to-peak score",
    "peakToPeakScore": "runtime score state is missing",
    "#define PEAK_TO_PEAK_FIRST_BEAT_SCORE 8": "experiment should allow high-score first acquisition beats",
    "bool isPeakToPeakCadenceMatch": "experiment should use a broader peak-to-peak cadence match",
    "#define PEAK_TO_PEAK_LOCKED_IBI_TOLERANCE_PERCENT 35": "locked peak-to-peak cadence should be more liberal than strict recovery",
    "#define PEAK_TO_PEAK_LOCKED_MIN_IBI_PERCENT 70": "locked peak-to-peak should reject short movement-blip intervals",
}

missing = [message for token, message in required.items() if token not in source]
if missing:
    raise SystemExit("Peak-to-peak experiment guard failed: " + "; ".join(missing))

print("Peak-to-peak experiment checks passed")
