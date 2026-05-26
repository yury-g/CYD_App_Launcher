from pathlib import Path

from project_metadata import read_firmware_metadata

source = Path("PulseSensor_CYD.ino").read_text()
metadata = read_firmware_metadata()

required = {
    "bool signalIsRecentlyClipped()": "firmware should name recent rail clipping as an explicit signal state",
    "bool signalRangeIsMotionArtifact()": "firmware should name rail-to-rail motion/artifact range explicitly",
    "bool signalLooksCleanForAcquisition()": "acquisition/re-arm logic should share one clean-signal guard",
    "liveRange >= REARM_SIGNAL_RANGE && !signalIsRecentlyClipped()": "detector re-arm should not run on clipped rail noise",
    "COACH_CLIPPED": "dashboard coach should have a clipped/adjust-sensor state",
    'return "ADJUST SENSOR";': "dashboard should tell the user to adjust the sensor when the ADC is railed",
    f'#define APP_VERSION "{metadata.version}"': "firmware version should identify the current build",
}

missing = [message for token, message in required.items() if token not in source]

platformio = Path("platformio.ini").read_text()
if f'-D APP_VERSION=\\"{metadata.diagnostic_version}\\"' not in platformio:
    missing.append("diagnostic PlatformIO env should report the beat-thinking marker logger version")

def function_body(name):
    start = source.index(f"\n{name} {{")
    end = source.index("\n}", start)
    return source[start:end]

if "if (!signalLooksCleanForAcquisition()) return 0;" not in function_body("int acquisitionScoreForCurrentSignal()"):
    missing.append("signal quality should not rise during clipped or motion-artifact input")

if "if (!signalLooksCleanForAcquisition()) return 0;" not in function_body("int peakToPeakScoreForCurrentSignal()"):
    missing.append("peak-to-peak score should not rise during clipped or motion-artifact input")

if missing:
    raise SystemExit("Clipping quality guard failed: " + "; ".join(missing))

print("Clipping quality guard checks passed")
