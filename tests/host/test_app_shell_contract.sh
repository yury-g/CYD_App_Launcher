#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:-.}"
SOURCE_FILES=$(find "${ROOT}" -maxdepth 1 \( -name '*.ino' -o -name '*.h' -o -name '*.cpp' \))

require_literal() {
  local needle="$1"
  local description="$2"

  if ! rg -q --fixed-strings "${needle}" "${ROOT}"; then
    echo "FAIL: ${description}"
    echo "Missing literal: ${needle}"
    exit 1
  fi
}

require_regex() {
  local pattern="$1"
  local description="$2"

  if ! perl -0ne "exit(/${pattern}/s ? 0 : 1)" ${SOURCE_FILES}; then
    echo "FAIL: ${description}"
    echo "Missing pattern: ${pattern}"
    exit 1
  fi
}

require_source_literal() {
  local needle="$1"
  local description="$2"

  if ! rg -q --fixed-strings "${needle}" ${SOURCE_FILES}; then
    echo "FAIL: ${description}"
    echo "Missing literal: ${needle}"
    exit 1
  fi
}

require_literal "#define PACKAGE_BRAND \"PulseSensor.com\"" "firmware should expose the PulseSensor brand for splash/about screens"
require_literal "#define PACKAGE_APP_NAME \"CYD App Launcher\"" "firmware should expose the app launcher name"
require_literal "#define PACKAGE_VERSION \"1.3.0-dev\"" "firmware should carry an individual development version"
require_literal "#define PACKAGE_BUILD \"esp32-2432s028r-cyd\"" "firmware should identify the CYD build"
require_literal "struct LauncherApp" "apps should be registered through a small launcher app contract"
require_literal "static LauncherApp apps[]" "launcher should keep apps in a self-contained app table"
require_source_literal "static uint8_t activeApp = 0;" "launcher should start on the main Pulse app"
require_literal "{\"Pulse\", appPulseSetup, appPulseLoop, appPulseDraw}" "pulse dashboard should remain one launcher app"
require_literal "{\"PinScan\", appScannerSetup, appScannerLoop, appScannerDraw}" "analog pin scanner should be one launcher app"
require_literal "{\"Splash\", appSplashSetup, appSplashLoop, appSplashDraw}" "ported splash screen should be one launcher app"
require_literal "{\"About\", appAboutSetup, appAboutLoop, appAboutDraw}" "ported about screen should be one launcher app"
require_literal "void switchToNextApp();" "launcher should have a single app-switching action"
require_literal "bool handleAppSwitchTouch(int16_t x, int16_t y);" "touch should provide an app-switching target"
require_literal "touchAppJustSwitched" "touch handling should debounce app switching separately from volume"
require_source_literal "bool appSwitchTouchArmed = true;" "app switching should require a release before it can fire again"
require_source_literal "{\"IO35 P3\", 35, 0, 4095, 0, 0}" "embedded scanner should focus on the no-solder CYD P3 PulseSensor pin"
require_source_literal "BUILT-IN SCAN KEEPS TOUCH LIVE" "embedded scanner should document why touch-bus pins are excluded"
require_source_literal "NO-SOLDER SIGNAL PIN" "embedded scanner should explain that it scans the beginner-accessible signal pin"
require_source_literal "appSwitchTouchArmed = true;" "app switching should re-arm after touch release"
require_source_literal "if (!appSwitchTouchArmed) return false;" "app switching should ignore held touches"
require_source_literal "appSwitchTouchArmed = false;" "app switching should disarm after one switch"
require_literal "void appScannerSetup();" "analog pin scanner app should be declared"
require_literal "void appScannerLoop();" "analog pin scanner app should be declared"
require_literal "void appScannerDraw();" "analog pin scanner app should be declared"
require_literal "void scannerUpdateReadings();" "analog pin scanner should keep its reading update self-contained"
require_literal "int scannerHotIndex();" "analog pin scanner should identify the hottest moving pin"
require_literal "void scannerDrawRow(int index, int y, bool hot);" "analog pin scanner should draw its own rows"
require_source_literal "analogReadResolution(12);" "scanner should use 12-bit ADC values"
require_source_literal "analogSetAttenuation(ADC_11db);" "scanner should use full-range ESP32 ADC attenuation"
require_source_literal "total += analogRead(scannerPins[i].pin);" "scanner should read raw ADC values without PulseSensorPlayground"
require_source_literal "scannerPins[i].movement = scannerPins[i].maxValue - scannerPins[i].minValue;" "scanner should track recent movement"
require_source_literal "drawCenteredText(\"PIN SCAN\", 70, 3, COLOR_CYAN, COLOR_BG);" "scanner should show a clear app title"
require_source_literal "Serial.printf(\"35=%4d\\n\"," "embedded scanner should print live serial readings for the no-solder signal pin"
require_literal "drawTopBar(\"Splash\");" "splash app should draw a named top bar"
require_literal "drawTopBar(\"About\");" "about app should draw a named top bar"
require_source_literal "#define VOL_Y 17" "pulse dashboard should move volume controls below the top-right brand"
require_source_literal "tft.setCursor(224, 6);" "pulse dashboard header should place the brand in the top-right corner"
require_source_literal "tft.print(PACKAGE_BRAND);" "pulse dashboard header should show PulseSensor.com instead of a generic label"
require_literal "drawCenteredText(\"Pulse\", 38, 5, COLOR_RED, COLOR_BG);" "splash should carry over the large Pulse title"
require_literal "drawCenteredText(\"Sensor\", 94, 5, COLOR_CYAN, COLOR_BG);" "splash should carry over the large Sensor title"
require_literal "drawCenteredText(\"VERSION\", 46, 3, COLOR_MUTED, COLOR_BG);" "splash should include a large version page"
require_literal "drawCenteredText(\"BOARD\", 38, 3, COLOR_MUTED, COLOR_BG);" "about should include a board page"
require_literal "drawCenteredText(\"SIGNAL\", 34, 3, COLOR_MUTED, COLOR_BG);" "about should include a signal wiring page"
require_literal "drawCenteredText(\"GPIO35\", 128, 5, COLOR_AMBER, COLOR_BG);" "about should document the CYD PulseSensor signal pin"
require_literal "drawCenteredText(\"NEXT\", 34, 3, COLOR_MUTED, COLOR_BG);" "about should include the planned-apps page"
require_literal "drawBottomBar();" "ported screens should include the app-switching footer"
require_regex "void appSplashDraw\\(\\).*PACKAGE_VERSION" "splash draw should render the package version"
require_regex "void appAboutDraw\\(\\).*PACKAGE_VERSION" "about draw should render the package version"

if rg -q --fixed-strings "LIVE BEAT DETECTION" "${ROOT}" -g '*.ino' -g '*.h' -g '*.cpp'; then
  echo "FAIL: pulse dashboard header should not use the old generic live beat label"
  echo "Unexpected literal: LIVE BEAT DETECTION"
  exit 1
fi

echo "App shell contract checks passed"
