from pathlib import Path

source = Path("PulseSensor_CYD.ino").read_text()
platformio = Path("platformio.ini").read_text()

missing = []

required_source = {
    '#define APP_VERSION "0.4.41-snappy-lock"': "release version should identify the snappy-lock build",
    "#ifndef APP_VERSION": "diagnostic build should override the release version by build flag",
    "#define RAW_SIGNAL_DIAGNOSTICS 0": "release build should default raw diagnostics off",
    "#ifndef PERF_DIAGNOSTICS": "perf diagnostics should remain build-flag overrideable",
    "struct BeatDecision": "beat acceptance should be grouped in a small decision struct",
    "BeatDecision decideBeat": "beat decision helper is missing",
    "decision.strictAccepted = decision.qualified;": "strict decision path should be the old simple qualified-beat path",
    "decision.peakToPeakAccepted = PEAK_TO_PEAK_EXPERIMENT &&": "peak-to-peak decision path should be explicit",
    "drawAppFrameHeader": "shared app header helper is missing",
    "bool settingsRowVisible": "Settings row visibility helper is missing",
    "void releaseApp3CrawlSprite": "Origin Story sprite release helper is missing",
    "releaseApp3CrawlSprite();": "sprite release helper is not called",
}

required_platformio = {
    "[env:cyd]": "release PlatformIO environment is missing",
    "[env:cyd_diag]": "diagnostic PlatformIO environment is missing",
    '-D APP_VERSION=\\"0.4.41-snappy-lock-log\\"': "diagnostic build must set logging version",
    "-D RAW_SIGNAL_DIAGNOSTICS=1": "diagnostic build must enable raw logging",
}

for token, message in required_source.items():
    if token not in source:
        missing.append(message)

for token, message in required_platformio.items():
    if token not in platformio:
        missing.append(message)

for dead_token in [
    "handleRotateTouch",
    "handleVolumeTouch",
    "drawRotateControl",
    "drawVolumeControl",
    "rotateButtonX",
    "volumeLabelX",
    "CONTROL_BUTTON_SIZE",
]:
    if dead_token in source:
        missing.append(f"dead top-bar control token still present: {dead_token}")

loop_start = source.index("void loop() {")
loop_end = source.index("// ===== HARDWARE SETUP =====", loop_start)
loop_body = source[loop_start:loop_end]
first_read = loop_body.find("readPulseSensor();")
if first_read < 0:
    missing.append("loop no longer calls readPulseSensor")
else:
    for token in [
        "readTouchControls();",
        "updateLED();",
        "updateBeatChime();",
        "updateSignalHarmony();",
        "updateApp3CrawlFanfare();",
        "updateActivePinScannerReading();",
    ]:
        token_pos = loop_body.find(token)
        if 0 <= token_pos < first_read:
            missing.append(f"readPulseSensor is blocked by {token}")

if missing:
    raise SystemExit("Core polish guard failed: " + "; ".join(missing))

print("Core polish checks passed")
