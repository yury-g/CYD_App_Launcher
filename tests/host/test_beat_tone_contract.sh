#!/usr/bin/env bash
set -euo pipefail

SKETCH="${1:-CYD_App_Launcher.ino}"
README="${README:-README.md}"

require_literal() {
  local needle="$1"
  local description="$2"

  if ! rg -q --fixed-strings "${needle}" "${SKETCH}"; then
    echo "FAIL: ${description}"
    echo "Missing literal: ${needle}"
    exit 1
  fi
}

require_regex() {
  local pattern="$1"
  local description="$2"

  if ! perl -0ne "exit(/${pattern}/s ? 0 : 1)" "${SKETCH}"; then
    echo "FAIL: ${description}"
    echo "Missing pattern: ${pattern}"
    exit 1
  fi
}

reject_literal() {
  local needle="$1"
  local description="$2"

  if rg -q --fixed-strings "${needle}" "${SKETCH}"; then
    echo "FAIL: ${description}"
    echo "Unexpected literal: ${needle}"
    exit 1
  fi
}

require_file_literal() {
  local file="$1"
  local needle="$2"
  local description="$3"

  if ! rg -q --fixed-strings -- "${needle}" "${file}"; then
    echo "FAIL: ${description}"
    echo "Missing literal: ${needle}"
    echo "In file: ${file}"
    exit 1
  fi
}

require_literal "#define SPEAKER_PIN 26" "speaker output should use the CYD speaker pin"
require_literal "#define HEART_MIN_SIZE" "beat heart should define a calm resting size"
require_literal "#define HEART_MAX_SIZE" "beat heart should define a larger beat size"
require_literal "#define HEART_CENTER_X 160" "beat heart should be centered in the header"
require_literal "#define HEART_CENTER_Y 22" "beat heart should live in the top status band"
require_literal "#define BEAT_CHIME_STEP_COUNT 4" "beat chime should use a short rolling chord"
require_literal "#define VOLUME_START 1" "speaker should start at a very low volume"
require_literal "#define VOL_MINUS_X" "header volume control should define a minus hit target"
require_literal "#define VOL_PLUS_X" "header volume control should define a plus hit target"
require_literal "#include <XPT2046_Touchscreen.h>" "volume controls should use the CYD touch controller"
require_literal "const uint16_t BEAT_CHIME_FREQUENCIES[BEAT_CHIME_STEP_COUNT]" "beat chime should define multiple notes"
require_literal "{262, 392, 523, 659}" "beat chime should use a lower, warmer note range"
require_literal "const uint8_t BEAT_CHIME_DUTIES[BEAT_CHIME_STEP_COUNT]" "beat chime should define a decay envelope"
require_literal "const uint16_t BEAT_CHIME_DURATIONS_MS[BEAT_CHIME_STEP_COUNT]" "beat chime should define non-blocking note durations"
require_literal "uint8_t speakerVolume = VOLUME_START;" "speaker volume should initialize low"
require_literal "void setupSpeaker();" "speaker setup should be declared"
require_literal "void setupTouch();" "touch setup should be declared"
require_literal "void readTouchControls();" "touch controls should be declared"
require_literal "void drawVolumeControl();" "volume control drawing should be declared"
require_literal "bool handleVolumeTouch(int16_t x, int16_t y);" "volume touch handler should be declared"
require_literal "void startBeatChime();" "beat chime trigger should be declared"
require_literal "void updateBeatChime();" "beat chime update loop should be declared"
require_literal "void triggerBeatEffects();" "beat effects should have one shared trigger"
require_literal "void drawBeatHeart();" "beat heart drawing should be declared"
require_literal "void fillHeartShape(int centerX, int centerY, int size, uint16_t color);" "heart shape helper should support outline and fill passes"
require_literal "uint16_t liveTraceColor();" "heart outline and waveform should share the live trace color"
require_literal "#define SIGNAL_COACH_FLAT_AMPLITUDE 12" "signal coach should define a too-flat amplitude threshold"
require_literal "#define AMPLITUDE_METER_MAX 120" "signal coach should cap the display amplitude scale"
require_literal "enum SignalCoachState" "signal coach should use named states"
require_literal "bool insideBeatWindow = false;" "signal coach should track the library's isInsideBeat state"
require_literal "int previousPulseAmplitude = -1;" "dashboard redraw cache should include amplitude"
require_literal "int previousSignalCoachState = -1;" "dashboard redraw cache should include coach state"
require_literal "int signalCoachState();" "signal coach state helper should be declared"
require_literal "const char* signalCoachText();" "signal coach text helper should be declared"
require_literal "int amplitudeMeterSegments(int amplitude);" "amplitude meter helper should be declared"
require_literal "void drawAmplitudeMeter(int x, int y, int amplitude);" "signal panel should draw visible amplitude"
require_literal "void drawThresholdMarker(int localX);" "graph should draw the threshold marker"
require_literal "void drawDashboardIfChanged();" "dashboard should redraw only when displayed values change"
require_literal "bool dashboardDrawn = false;" "dashboard redraw cache should track initial draw"
require_literal "int previousDisplayBPM = -1;" "BPM redraw cache should avoid repeated panel clears"
require_literal "int previousSignalQuality = -1;" "quality redraw cache should avoid repeated panel clears"
require_literal "setupSpeaker();" "speaker should be initialized during setup"
require_literal "setupTouch();" "touch should be initialized during setup"
require_literal "readTouchControls();" "touch controls should be read in the main loop"
require_literal "drawVolumeControl();" "volume controls should be drawn in the header"
require_literal "updateBeatChime();" "chime should advance from the main loop without blocking"
require_literal "drawBeatHeart();" "heart should animate from the main loop"
require_literal "drawDashboardIfChanged();" "main loop should use change-driven dashboard drawing"
require_literal "const int centerX = HEART_CENTER_X;" "heart drawing should use the header center X constant"
require_literal "const int centerY = HEART_CENTER_Y;" "heart drawing should use the header center Y constant"
require_literal "tft.fillRect(clearX, clearY, clearW, clearH, COLOR_BG);" "heart should clear against the header background"
require_literal "uint16_t outlineColor = liveTraceColor();" "heart outline should use the waveform color"
require_literal "fillHeartShape(centerX, centerY, size + 2, outlineColor);" "heart should draw a thin trace-colored outline"
require_literal "fillHeartShape(centerX, centerY, size, heartColor);" "heart should keep its red fill"
require_literal "return lockedSignal ? COLOR_TEXT : COLOR_CYAN;" "live trace should be white when qualified and mint while searching"
require_literal "uint16_t waveColor = liveTraceColor();" "waveform should use the same color helper as the heart outline"
require_literal "insideBeatWindow = pulseSensor.isInsideBeat();" "firmware should expose the Playground isInsideBeat state"
require_literal "tft.print(signalCoachText());" "header should show coach text instead of a generic search label"
require_literal "drawAmplitudeMeter(x + 9, PANEL_Y + 49, pulseAmplitude);" "quality panel should include amplitude feedback"
require_literal "int segments = amplitudeMeterSegments(amplitude);" "amplitude meter should be scaled into stable segments"
require_literal "int y = map(PULSE_THRESHOLD, minSignal, maxSignal, GRAPH_Y + GRAPH_H - 8, GRAPH_Y + 8);" "threshold marker should map the Playground threshold into graph space"
require_literal "ledcAttach(SPEAKER_PIN, BEAT_CHIME_FREQUENCIES[0], SPEAKER_BITS);" "speaker PWM should attach to the chime channel"
require_literal "ledcWriteTone(SPEAKER_PIN, BEAT_CHIME_FREQUENCIES[beatChimeStep]);" "beat chime should play each arpeggio note"
require_literal "ledcWrite(SPEAKER_PIN, scaledChimeDuty(beatChimeStep));" "beat chime should use the volume-scaled envelope"
require_literal "ledcWriteTone(SPEAKER_PIN, 0);" "beat tone should stop after its short duration"
require_regex "void triggerBeatEffects\\(\\) \\{[[:space:]]*ledBrightness = 255;[[:space:]]*beatHeartNeedsRedraw = true;[[:space:]]*startBeatChime\\(\\);" "beat effects should sync LED fade, heart animation, and chime"
require_regex "if \\(lockedSignal && qualified\\) \\{[[:space:]]*triggerBeatEffects\\(\\);" "effects should only play on locked qualified beats"
require_file_literal "${README}" "- Signal Coach messages for too-flat, steady, good-wave, locking, and qualified states" "feature list should include the new Signal Coach behavior"
require_file_literal "${README}" '- `isInsideBeat()` to show when the signal is inside the detected beat window' "tie-ins should include isInsideBeat"
require_file_literal "${README}" '- `setThreshold(550)` as the starter sensitivity line shown on the graph' "tie-ins should include threshold display"
require_file_literal "${README}" "## Feature Wishlist" "README should record future feature ideas"
require_file_literal "${README}" "- Method Overlay: tap the quality panel to cycle through the live Playground method names behind each reading." "wishlist should include the method overlay idea"
require_file_literal "${README}" "- USB Serial Lab Mode: optional Playground-style serial output for Arduino Serial Plotter or a WebSerial monitor." "wishlist should include serial lab mode"
require_file_literal "${README}" "## Avoid For Now" "README should record deferred ideas"
require_file_literal "${README}" "https://github.com/yury-g/CYD_Analog_Pin_Scanner" "README should link to the standalone analog pin scanner"
require_file_literal "${README}" "- Multi-sensor and Pulse Transit Time experiments need extra wiring and should wait until the one-sensor CYD lesson is rock solid." "avoid list should include multi-sensor/PTT"
require_file_literal "${README}" "- WiFi server mode adds credentials, dependencies, and classroom setup friction that do not improve the default CYD experience yet." "avoid list should include WiFi server mode"
require_file_literal "${README}" "- Servo or motor outputs are fun Playground branches, but they add hardware without improving PulseSensor accuracy on this device." "avoid list should include servo/motor outputs"
reject_literal "tft.print(lockedSignal ? \"QUALIFIED BEAT\" : \"SIGNAL SEARCH\");" "header should use signal coach text now"
reject_literal "millis() - lastPanelDraw >= 180" "dashboard should not repaint all panels on a timer"
reject_literal "lastPanelDraw = millis();" "timer-driven panel repaint state should be removed"
reject_literal "beatHeartNeedsRedraw = true;\n\n  tft.setTextSize(1);\n  tft.setTextColor(COLOR_MUTED, COLOR_PANEL);" "quality panel redraw should not force the header heart to redraw"
reject_literal "tft.fillRoundRect(184, 6, 126, 24" "old status pill should be removed"
reject_literal "tft.print(lockedSignal ? \"LOCKED\" : \"SEARCHING\");" "duplicated status words should be removed"
reject_literal "drawLedIndicator" "quality-panel red dot should be removed"

echo "Beat tone contract checks passed"
