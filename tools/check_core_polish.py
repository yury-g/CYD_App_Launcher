from pathlib import Path

from project_metadata import read_firmware_metadata

source = Path("PulseSensor_CYD.ino").read_text()
platformio = Path("platformio.ini").read_text()
metadata = read_firmware_metadata()

missing = []

required_source = {
    f'#define APP_VERSION "{metadata.version}"': "release version should identify the current build",
    "#ifndef APP_VERSION": "diagnostic build should override the release version by build flag",
    "#define RAW_SIGNAL_DIAGNOSTICS 0": "release build should default raw diagnostics off",
    "#ifndef PERF_DIAGNOSTICS": "perf diagnostics should remain build-flag overrideable",
    "struct BeatDecision": "beat acceptance should be grouped in a small decision struct",
    "struct PulseSignalState": "pulse/signal state should be grouped",
    "struct DashboardState": "dashboard state should be grouped",
    "struct SoundState": "sound state should be grouped",
    "struct AppRuntimeState": "app runtime state should be grouped",
    "BeatDecision decideBeat": "beat decision helper is missing",
    "decision.strictAccepted = decision.qualified;": "strict decision path should be the old simple qualified-beat path",
    "decision.peakToPeakAccepted = PEAK_TO_PEAK_RECOVERY_ENABLED &&": "peak-to-peak recovery decision path should be explicit",
    "drawAppFrameHeader": "shared app header helper is missing",
    "bool settingsRowVisible": "Settings row visibility helper is missing",
    "void releaseApp3CrawlSprite": "Origin Story sprite release helper is missing",
    "releaseApp3CrawlSprite();": "sprite release helper is not called",
}

required_platformio = {
    "[env:cyd]": "release PlatformIO environment is missing",
    "[env:cyd_diag]": "diagnostic PlatformIO environment is missing",
    f'-D APP_VERSION=\\"{metadata.diagnostic_version}\\"': "diagnostic build must set logging version",
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
