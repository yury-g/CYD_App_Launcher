from pathlib import Path

from project_metadata import read_firmware_metadata

source = Path("PulseSensor_CYD.ino").read_text()
platformio = Path("platformio.ini").read_text()
capture = Path("tools/capture_signal_log.py").read_text() if Path("tools/capture_signal_log.py").exists() else ""
analyzer = Path("tools/analyze_signal_log.py").read_text() if Path("tools/analyze_signal_log.py").exists() else ""
metadata = read_firmware_metadata()

required_source = {
    "#ifndef RAW_SIGNAL_DIAGNOSTICS": "raw diagnostics must be a build-mode switch",
    "#define RAW_SIGNAL_DIAGNOSTICS 0": "release build must keep raw diagnostics off by default",
    "#define RAW_SIGNAL_DIAGNOSTICS_MS 20": "raw diagnostics should stream at 50 Hz",
    "#ifndef APP_VERSION": "release firmware version must be overrideable by diagnostic build flags",
    f'#define APP_VERSION "{metadata.version}"': "release firmware version must identify the current build",
    "void printRawSignalDiagnostics": "firmware must have a raw diagnostic printer",
    "rawDiag,ms,signal,amp,bpm,ibi,locked,quality,p2p,range,clip,inside,beat,accept,drop,qStreak,badStreak": "CSV header is missing required fields",
    "rawDiagnosticsBeatPending": "diagnostics must mark firmware beat events",
    "if (!RAW_SIGNAL_DIAGNOSTICS && millis() - lastSerialPrint >= 500)": "normal slow serial summary must be disabled during raw diagnostics",
    "#define CLIPPING_SCORE_DECAY_MS 20": "clipping score must decay by time, not foreground loop speed",
    "now - lastClipDecayMs >= CLIPPING_SCORE_DECAY_MS": "clipping score decay should be time-based",
    "#define SIGNAL_MOTION_ARTIFACT_RANGE 980": "rail-to-rail artifact range guard is missing",
    "maxSignal - minSignal > SIGNAL_MOTION_ARTIFACT_RANGE": "beat acceptance should reject implausibly large motion range",
    "#define ACQUISITION_CADENCE_TOLERANCE_PERCENT 35": "acquisition cadence guard is missing",
    "bool isAcquisitionCadenceMatch": "pre-lock acquisition should require cadence consistency",
    "decision.strictAccepted = decision.qualified;": "strict beat acceptance should use the old simple qualified-beat path",
}

required_platformio = {
    "[env:cyd_diag]": "diagnostic PlatformIO environment is missing",
    f'-D APP_VERSION=\\"{metadata.diagnostic_version}\\"': "diagnostic build must set the logging firmware version",
    "-D RAW_SIGNAL_DIAGNOSTICS=1": "diagnostic build must enable raw CSV streaming",
}

required_capture = {
    "serial.Serial": "capture tool must read the serial port",
    "rawDiag,": "capture tool must keep raw diagnostic CSV lines",
    "signal-log": "capture tool should default to a signal-log filename",
}

required_analyzer = {
    "def independent_peaks": "analyzer must include an independent peak detector",
    "firmware_beats": "analyzer must compare firmware beat events",
    "motion/noise": "analyzer should report motion/noise windows",
}

missing = []
for token, message in required_source.items():
    if token not in source:
        missing.append(message)
for token, message in required_platformio.items():
    if token not in platformio:
        missing.append(message)
for token, message in required_capture.items():
    if token not in capture:
        missing.append(message)
for token, message in required_analyzer.items():
    if token not in analyzer:
        missing.append(message)

if missing:
    raise SystemExit("Signal diagnostics guard failed: " + "; ".join(missing))

print("Signal diagnostics checks passed")
