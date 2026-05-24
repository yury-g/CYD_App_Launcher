from pathlib import Path

source = Path("PulseSensor_CYD.ino").read_text()
settings_mock = Path("tools/render_settings_mock.py").read_text()

required_tokens = [
    "APP_VERSION",
    "APP_FIRMWARE_DATE",
    "About",
    "Version",
    "Firmware",
    "Build",
    "WiFi",
    "Bluetooth",
    "LED Control",
    "your app here",
    "APP_PLACEHOLDER_1",
    "APP_PLACEHOLDER_2",
    "APP_PIN_SCANNER",
    "APP_SETTINGS",
    "Pin Scanner",
    "Memory",
    "DISPLAY_MONO_DARK",
    "DISPLAY_MONO_LIGHT",
    "DISPLAY_COLOR_DARK",
    "DISPLAY_COLOR_LIGHT",
    "cycleDisplayMode",
    "displayModeName",
]

missing = [token for token in required_tokens if token not in source]
if missing:
    raise SystemExit("Missing app shell tokens: " + ", ".join(missing))

version_marker = '#define APP_VERSION "'
version_start = source.index(version_marker) + len(version_marker)
version_end = source.index('"', version_start)
app_version = source[version_start:version_end]
if len(app_version) > 25:
    raise SystemExit("APP_VERSION is too wide for landscape Settings size-2 text")

header_start = source.index("void drawHeader() {")
header_end = source.index("void drawAppNavControls()", header_start)
header_body = source[header_start:header_end]
if "drawVolumeControl();" in header_body:
    raise SystemExit("Pulse header still draws top-bar volume controls")

settings_start = source.index("void drawSettingsScreen() {")
settings_end = source.index("void drawSettingsRow", settings_start)
settings_body = source[settings_start:settings_end]
if '"Volume"' not in settings_body:
    raise SystemExit("Settings screen is missing Volume")
if "APP_FIRMWARE_DATE" not in settings_body:
    raise SystemExit("Settings screen is missing firmware date")
if '"Settings "' not in settings_body:
    raise SystemExit("Settings title is missing firmware date")
if '"Memory"' not in settings_body:
    raise SystemExit("Settings screen is missing the memory row")
if "ESP.getFreeHeap()" not in settings_body or "ESP.getHeapSize()" not in settings_body:
    raise SystemExit("Settings memory row must show heap size and percentage")
if "usedHeap" not in settings_body or '"used %luK free %luK %u%%"' not in settings_body:
    raise SystemExit("Settings memory row must show used memory and free memory")
if "APP_BUILD_RAM_USAGE" not in settings_body or "APP_BUILD_FLASH_USAGE" not in settings_body:
    raise SystemExit("Settings screen is missing build RAM/Flash usage")
if '"rot %u"' in settings_body:
    raise SystemExit("Settings rotation value should not use ROT text")
if 'drawSettingsButton(settingsRotateX, rowY + 6, 86, "", false);' not in settings_body:
    raise SystemExit("Settings rotation control should be icon-only")
if '"ROT"' in settings_mock:
    raise SystemExit("Settings mock still renders ROT text instead of the rotation icon")
if "bool settingsValueNeedsCompactText" not in source:
    raise SystemExit("Settings value fit check should be shared before choosing tiny text")
if "if (!portraitLayout && settingsValueNeedsCompactText(label, value))" not in source:
    raise SystemExit("Landscape Settings rows should avoid tiny value text by using two-line text")
if "return portraitLayout ? 1 : SETTINGS_TEXT_SIZE;" not in source:
    raise SystemExit("The smallest Settings value text should be limited to vertical display rotation")
if "value_w > width - 28 - label_w and width >= 300" not in settings_mock:
    raise SystemExit("Settings mock should keep landscape fallback text larger")

read_touch_start = source.index("void readTouchControls() {")
read_touch_end = source.index("void mapTouchPoint", read_touch_start)
read_touch_body = source[read_touch_start:read_touch_end]
if "handleVolumeTouch" in read_touch_body:
    raise SystemExit("Pulse touch handling still uses top-bar volume controls")
if "handleRotateTouch(x, y)" in read_touch_body:
    raise SystemExit("Rotate touch is still exposed as a persistent top-bar control")
if "handleAppNavTouch(x, y)" not in read_touch_body:
    raise SystemExit("App navigation should be checked before tap-to-reacquire")
if "handlePulseReacquireTouch" not in read_touch_body:
    raise SystemExit("Pulse dashboard needs tap-to-reacquire outside app navigation")
if read_touch_body.find("handlePulseReacquireTouch") < read_touch_body.find("handleAppNavTouch"):
    raise SystemExit("Tap-to-reacquire must not claim app navigation touches")

loop_start = source.index("void loop() {")
loop_end = source.index("// ===== HARDWARE SETUP =====", loop_start)
loop_body = source[loop_start:loop_end]
first_read = loop_body.find("readPulseSensor();")
if first_read < 0:
    raise SystemExit("Pulse loop no longer samples PulseSensor")
if "if (currentApp != APP_PIN_SCANNER) readPulseSensor();" in loop_body:
    raise SystemExit("PulseSensor sampling must not pause while App 4 is open")
for token in [
    "readTouchControls();",
    "updateLED();",
    "updateBeatChime();",
    "updateSignalHarmony();",
    "updateApp3CrawlFanfare();",
    "updateActivePinScannerReading();",
]:
    token_pos = loop_body.find(token)
    if token_pos >= 0 and token_pos < first_read:
        raise SystemExit(f"PulseSensor sampling is blocked by {token}")

pulse_reacquire_start = source.index("bool handlePulseReacquireTouch(int16_t x, int16_t y) {")
pulse_reacquire_end = source.index("bool handleSettingsTouch", pulse_reacquire_start)
pulse_reacquire_body = source[pulse_reacquire_start:pulse_reacquire_end]
for token in [
    'rearmPulseDetector("manual touch reacquire");',
    "resetSignalAcquisitionWindow();",
    "resetDashboardState();",
    "drawStaticScreen();",
]:
    if token not in pulse_reacquire_body:
        raise SystemExit("Tap-to-reacquire is missing signal reset behavior")
if "y < headerHeight + CONTROL_TOUCH_PAD" not in pulse_reacquire_body:
    raise SystemExit("Tap-to-reacquire should stay out of the app-navigation/header area")

live_trace_start = source.index("uint16_t liveTraceColorForMode() {")
live_trace_end = source.index("uint16_t pinScannerHotColor()", live_trace_start)
live_trace_body = source[live_trace_start:live_trace_end]
if "return lockedSignal ? signalLockColor() : signalSearchColor();" not in live_trace_body:
    raise SystemExit("Waveform color must match SIG panel acquisition/lock colors")
for token in [
    "COLOR_ACQUIRE_BLUE",
    "COLOR_LIGHT_TRACE_BLUE",
]:
    if token in live_trace_body:
        raise SystemExit("Waveform color should not use a separate acquisition palette")

required_lock_hold_tokens = {
    "#define LOCK_QUALIFIED_BEATS 4": "Strict acquisition should still require four qualified beats",
    "#define LOCK_GRACE_BAD_BEATS 2": "Balanced lock hold should tolerate two unqualified beats after lock",
    "#define LOCK_HOLD_GRACE_MS 2200": "Balanced lock hold should have a 2200 ms grace window",
    "int unqualifiedBeatStreak = 0;": "Lock hold should track unqualified beats after lock",
    'const char* lastLockDropReason = "none";': "Lock drops should record a lightweight serial reason",
    "wasLocked && unqualifiedBeatStreak <= LOCK_GRACE_BAD_BEATS": "Locked signal should survive brief bad-beat movement",
    "now - lastQualifiedBeatTime <= LOCK_HOLD_GRACE_MS": "Locked signal should survive a brief post-lock timing gap",
    'dropSignalLock("grace expired");': "Grace-expired lock drops should be tracked",
    'dropSignalLock("no beat timeout");': "Timeout lock drops should be tracked",
}
for token, message in required_lock_hold_tokens.items():
    if token not in source:
        raise SystemExit(message)

if "drop=%s" not in source or "unqualifiedBeatStreak" not in source[source.index("Serial.printf(\"signal="):source.index("// ===== HARDWARE SETUP =====")]:
    raise SystemExit("Serial signal telemetry should include lock-hold drop and streak fields")

app_nav_start = source.index("bool handleAppNavTouch(int16_t x, int16_t y) {")
app_nav_end = source.index("bool handleRotateTouch(int16_t x, int16_t y) {", app_nav_start)
app_nav_body = source[app_nav_start:app_nav_end]
required_touch_boundaries = [
    "appPrevNextBoundary",
    "appNextSettingsBoundary",
]
missing_touch_boundaries = [
    token for token in required_touch_boundaries if token not in app_nav_body
]
if missing_touch_boundaries:
    raise SystemExit(
        "App nav touch targets can overlap instead of using midpoint boundaries: "
        + ", ".join(missing_touch_boundaries)
    )

configure_start = source.index("void configureLayout() {")
configure_end = source.index("void resetDashboardState()", configure_start)
configure_body = source[configure_start:configure_end]
required_nav_layout = [
    "appSettingsButtonX = screenWidth - APP_BUTTON_WIDTH - 4;",
    "appNextButtonX = appSettingsButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;",
    "appPrevButtonX = appNextButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;",
]
missing_nav_layout = [line for line in required_nav_layout if line not in configure_body]
if missing_nav_layout:
    raise SystemExit("App nav is not attached to rotate button")

required_large_controls = {
    "DisplayMode displayMode = DISPLAY_COLOR_DARK;": "Default startup display mode is not color dark on black",
    "#define TOOLBAR_BUTTON_WIDTH 44": "toolbar buttons are not double-width",
    "#define TOOLBAR_BUTTON_HEIGHT 28": "toolbar buttons are not one-quarter taller",
    "#define APP_BUTTON_WIDTH TOOLBAR_BUTTON_WIDTH": "app nav width does not match toolbar width",
    "#define APP_BUTTON_HEIGHT TOOLBAR_BUTTON_HEIGHT": "app nav height does not match toolbar height",
    "#define SETTINGS_TEXT_SIZE 2": "Settings screen text is not bumped up one size",
    "#define SETTINGS_ROW_H 40": "Settings rows should support the larger font and touch controls",
    "#define SETTINGS_ROW_COUNT 12": "Settings screen is missing the added Memory/Build rows",
    "settingsScrollY": "Settings screen is missing scroll state",
    "handleSettingsScrollTouch": "Settings screen is missing scroll touch handling",
    "drawSettingsScrollControls": "Settings screen is missing large scroll controls",
    "settingsRowScreenY": "Settings controls are not using scrolled row-local hit tests",
    "settingsRowBackground": "Settings rows are missing mode-aware backgrounds",
    "metricPanelBackground": "Pulse dashboard metrics are missing mode-aware tile backgrounds",
    "tft.setTextColor(textColor(), panelBg)": "Pulse dashboard tile text is not mode-aware",
    "settingsScrollButtonW = (screenWidth - APP_BUTTON_GAP) / 2": "Settings scroll buttons do not fill the bottom bar width",
    "settingsScrollUpX = 0": "Settings scroll up button is not anchored to the left edge",
    "drawSettingsDisplayModeControl": "Settings screen is missing display mode control",
    "handleSettingsDisplayModeTouch": "Settings screen is missing display mode touch handling",
    "displayValueTextColor": "Settings value text is not separately colorable",
    "COLOR_LIGHT_NAV_FILL": "Color light app nav buttons are missing a high-contrast inactive fill",
    "APP3_CRAWL_FANFARE_STEP_COUNT": "App 3 origin crawl fanfare is missing",
    "startApp3CrawlFanfare": "App 3 origin crawl fanfare does not start on app entry",
    "updateApp3CrawlFanfare": "App 3 origin crawl fanfare does not update from the main loop",
    "stopApp3CrawlFanfare": "App 3 origin crawl fanfare does not stop when leaving App 3",
    "APP3_CRAWL_FANFARE_LOOP_START_STEP": "App 3 fanfare does not have a separate loop start",
    "APP3_CRAWL_FANFARE_LOOP_MS 15000": "App 3 fanfare loop is not tracked as a 15-second loop",
    "drawApp3OriginCrawl": "App 3 origin crawl renderer is missing",
    "APP3_ORIGIN_CRAWL_LINE_COUNT": "App 3 origin crawl copy is missing",
    "Origin Story": "App 3 is not labeled Origin Story",
    "Send feature requests,": "App 3 origin crawl is missing the feature-request ask",
    "UID US000075": "Origin Story crawl is missing the OSHWA registration number",
    "August 30, 2017": "Origin Story crawl is missing the OSHWA certification date",
    "PulseSensor_Amped_Arduino": "Origin Story crawl is missing the GitHub repo",
    "249 stars, 207 forks": "Origin Story crawl is missing the GitHub stats note",
    "#define APP3_CRAWL_TEXT_SIZE 2": "Origin Story crawl text is not enlarged",
    "TFT_eSprite app3CrawlSprite": "Origin Story crawl is missing an offscreen sprite",
    "ensureApp3CrawlSprite": "Origin Story crawl sprite is not managed for rotation changes",
    "app3CrawlSprite.pushSprite": "Origin Story crawl frames are not pushed from an offscreen sprite",
    "APP3_CRAWL_TEXT_SIZE": "Origin Story crawl does not keep the enlarged text size",
    "app3CrawlSprite.setTextSize(textSize)": "Origin Story crawl does not apply perspective text scaling",
    "app3CrawlSprite.setColorDepth(8)": "Origin Story crawl sprite is not using lower-memory 8-bit color",
    "drawApp3OriginCrawlDirectFallback": "Origin Story crawl has no visible fallback if sprite allocation fails",
    "int baseY = crawlBottom - lineHeight - offset;": "Origin Story crawl starts below the visible area",
    "#define APP3_CRAWL_SPEED_MS 52": "Origin Story crawl speed is not approximately 2x faster",
    "drawApp3CrawlLinePerspective": "Origin Story crawl is missing perspective fade/shrink rendering",
    "APP3_CRAWL_HORIZON_Y": "Origin Story crawl is missing a horizontal vanishing point",
    "APP3_CRAWL_MIN_TEXT_SIZE": "Origin Story crawl cannot shrink toward the horizon",
    "#define PIN_SCANNER_PIN_COUNT 4": "App 4 should list GPIO35, GPIO22, GPIO21, and GPIO27",
    "#define PIN_SCANNER_ADC_MAX_VALUE 1023": "App 4 scanner should keep the PulseSensor 10-bit ADC scale",
    "#define PIN_SCANNER_READ_MS": "App 4 scanner read rate limit is missing",
    "ScannerPin scannerPins[]": "App 4 pin scanner data model is missing",
    "scannerActiveIndex = -1": "App 4 scanner should start with no active pin selected",
    "handlePinScannerTouch": "App 4 scanner should let one pin be selected at a time",
    "setupPinScanner": "App 4 pin scanner setup is missing",
    "updateActivePinScannerReading": "App 4 pin scanner should update only the selected pin",
    "isPinScannerAdcCapable": "App 4 scanner should guard non-ADC pins before reading",
    "pinScannerStatusText": "App 4 scanner should explain inactive and non-ADC pin states",
    "drawApp4PinScanner": "App 4 pin scanner renderer is missing",
    "drawPinScannerRow": "App 4 pin scanner row renderer is missing",
    "PIN_SCANNER_DRAW_MS": "App 4 scanner draw rate limit is missing",
}
for token, message in required_large_controls.items():
    if token not in source:
        raise SystemExit(message)

app_order_start = source.index("enum AppId {")
app_order_end = source.index("};", app_order_start)
app_order_body = source[app_order_start:app_order_end]
app_order = [
    "APP_PULSE",
    "APP_SETTINGS",
    "APP_PIN_SCANNER",
    "APP_PLACEHOLDER_1",
    "APP_PLACEHOLDER_2",
]
last_pos = -1
for app_name in app_order:
    app_pos = app_order_body.find(app_name)
    if app_pos < 0 or app_pos < last_pos:
        raise SystemExit("App order should be Pulse, Settings, Pin Scanner, your-app-here, Origin Story")
    last_pos = app_pos

next_fn_start = source.index("void nextApp() {")
next_fn_end = source.index("void previousApp()", next_fn_start)
next_fn_body = source[next_fn_start:next_fn_end]
previous_fn_start = source.index("void previousApp() {")
previous_fn_end = source.index("void rotateScreen()", previous_fn_start)
previous_fn_body = source[previous_fn_start:previous_fn_end]
if "currentApp == APP_SETTINGS ? APP_PULSE" in next_fn_body + previous_fn_body:
    raise SystemExit("Settings should be in the app sequence, not skipped by next/previous")

for fn_name in [
    "void drawHeader() {",
    "void drawPlaceholderApp(const char* title, const char* message) {",
]:
    fn_start = source.index(fn_name)
    fn_end = source.index("\n}", fn_start)
    fn_body = source[fn_start:fn_end]
    if "drawAppNavControls();" not in fn_body:
        raise SystemExit(f"{fn_name} does not draw persistent nav controls")
    if "drawRotateControl();" in fn_body:
        raise SystemExit(f"{fn_name} still draws the removed top-bar rotate control")

settings_fn_start = source.index("void drawSettingsScreen() {")
settings_fn_end = source.index("\n}", settings_fn_start)
settings_fn_body = source[settings_fn_start:settings_fn_end]
if "drawAppNavControls();" not in settings_fn_body:
    raise SystemExit("Settings screen does not draw persistent nav controls")
if "drawRotateControl();" in settings_fn_body:
    raise SystemExit("Settings screen still draws the removed top-bar rotate control")

settings_row_start = source.index("void drawSettingsRow(int rowIndex, int y, const char* label, const char* value) {")
settings_row_end = source.index("void drawSettingsControlRow", settings_row_start)
settings_row_body = source[settings_row_start:settings_row_end]
if 'tft.print(": ");' in settings_row_body:
    raise SystemExit("Plain Settings rows should not print inline Label: value text")
if "y + 18" in settings_row_body:
    raise SystemExit("Plain Settings rows should not force a second line")
if "settingsRightAlignedValueX(value, valueTextSize)" not in settings_row_body:
    raise SystemExit("Plain Settings values should be right-aligned")
if "settingsValueTextSize(label, value)" not in settings_row_body:
    raise SystemExit("Plain Settings rows should dynamically fit long values")
if "int settingsRightAlignedValueX" not in source or "int settingsValueTextSize" not in source:
    raise SystemExit("Settings right-aligned value helpers are missing")
if "void drawSettingsControlRow" not in source:
    raise SystemExit("Settings control rows need a separate two-line fallback renderer")
for row_token in [
    'drawSettingsControlRow(0, rowY, "Volume", volumeText);',
    'drawSettingsControlRow(1, rowY, "Rotation", rotationText);',
    'drawSettingsControlRow(2, rowY, "Display", displayModeName());',
    'drawSettingsControlRow(5, rowY, "LED Control", beatLedEnabled ? "beat pulse" : "off");',
    'drawSettingsControlRow(6, rowY, "Color", "tap");',
]:
    if row_token not in settings_body:
        raise SystemExit("Settings rows with right-side controls should use the control-row fallback")
if "rowY + 6" not in settings_body:
    raise SystemExit("Settings controls should be vertically centered in larger rows")

dashboard_fn_start = source.index("void drawDashboardIfChanged() {")
dashboard_fn_end = source.index("void drawHeader()", dashboard_fn_start)
dashboard_fn_body = source[dashboard_fn_start:dashboard_fn_end]
if "drawGraphFrame();" in dashboard_fn_body:
    raise SystemExit("Live Pulse dashboard updates still redraw the whole graph frame")
if "drawSignalCoachStatus();" not in dashboard_fn_body:
    raise SystemExit("Live Pulse dashboard status changes should use a small status redraw")

if "bool shouldDrawInactiveQualitySegments() {" not in source:
    raise SystemExit("SIG GPIO quality bars need a mode guard for inactive segments")

if "int acquisitionScoreForCurrentSignal()" not in source:
    raise SystemExit("SIG GPIO quality bars need a granular acquisition score helper")
if "void updateSignalAcquisitionScore()" not in source:
    raise SystemExit("Pulse signal acquisition should update continuously before lock")
if "signalQuality = qualifiedBeatStreak * 3" in source:
    raise SystemExit("SIG GPIO quality should not jump only in 3-step beat-streak chunks")
if "lastSignalHarmonyQuality" not in source:
    raise SystemExit("Acquisition harmony should track score thresholds, not just beat streaks")
if "#define SIGNAL_HARMONY_NOTE_COUNT 8" not in source:
    raise SystemExit("Signal acquisition harmony should expose 8 notes")
if "#define SIGNAL_HARMONY_STEP_COUNT 4" not in source:
    raise SystemExit("Signal acquisition harmony phrase should expose 4 steps")

inactive_quality_start = source.index("bool shouldDrawInactiveQualitySegments() {")
inactive_quality_end = source.index("uint16_t beatColor()", inactive_quality_start)
inactive_quality_body = source[inactive_quality_start:inactive_quality_end]
if "return displayMode == DISPLAY_COLOR_DARK || displayMode == DISPLAY_COLOR_LIGHT;" not in inactive_quality_body:
    raise SystemExit("Monochrome modes should not draw inactive SIG GPIO quality-bar backgrounds")

quality_start = source.index("void drawQualitySegments(int x, int y) {")
quality_end = source.index("void drawAmplitudeMeter", quality_start)
quality_body = source[quality_start:quality_end]
if "bool drawInactiveSegments = shouldDrawInactiveQualitySegments();" not in quality_body:
    raise SystemExit("SIG GPIO quality bars are not checking whether inactive segments should draw")
if "else if (drawInactiveSegments)" not in quality_body:
    raise SystemExit("SIG GPIO quality bars should skip inactive segments when monochrome modes are active")

scanner_start = source.index("ScannerPin scannerPins[] = {")
scanner_end = source.index("};", scanner_start)
scanner_body = source[scanner_start:scanner_end]
for required_pin in ['"P3  IO35", 35', '"IO22", 22', '"BL  IO21", 21', '"CN1 IO27", 27']:
    if required_pin not in scanner_body:
        raise SystemExit(f"App 4 pin scanner is missing external connector target: {required_pin}")
for unsafe_pin in ['"IO34"', '"IO36"', '"IO39"', '"TOUCH"', '"MISO"', '"MOSI"', '"SCLK"', '"CS"']:
    if unsafe_pin in scanner_body:
        raise SystemExit(f"App 4 pin scanner includes unsafe integrated-app scan target: {unsafe_pin}")

active_update_start = source.index("void updateActivePinScannerReading() {")
active_update_end = source.index("const char* pinScannerStatusText", active_update_start)
active_update_body = source[active_update_start:active_update_end]
if "if (currentApp != APP_PIN_SCANNER) return;" not in active_update_body:
    raise SystemExit("App 4 scanner reads must be idle unless App 4 is visible")
if "scannerActiveIndex < 0" not in active_update_body:
    raise SystemExit("App 4 scanner must start idle until a single pin is selected")
if "isPinScannerAdcCapable(scannerPins[scannerActiveIndex].pin)" not in active_update_body:
    raise SystemExit("App 4 scanner does not guard analogRead with ADC capability")
if "analogReadResolution(12)" in active_update_body or "analogReadResolution(12)" in source:
    raise SystemExit("App 4 scanner should not change away from the PulseSensor 10-bit ADC resolution")
if "delayMicroseconds" in active_update_body or "delay(" in active_update_body:
    raise SystemExit("App 4 scanner reads should not add blocking delays")

app4_branch_start = source.index("} else if (currentApp == APP_PIN_SCANNER) {")
app4_branch_end = source.index("\n  }", app4_branch_start)
app4_branch_body = source[app4_branch_start:app4_branch_end]
if "updateActivePinScannerReading();" not in app4_branch_body or "drawApp4PinScanner();" not in app4_branch_body:
    raise SystemExit("App 4 should update and render only in its own app branch")

app3_branch_start = source.index("} else if (currentApp == APP_PLACEHOLDER_2) {")
app3_branch_end = source.index("\n  }", app3_branch_start)
app3_branch_body = source[app3_branch_start:app3_branch_end]
if 'drawPlaceholderApp("App 3", "your app here too")' in app3_branch_body:
    raise SystemExit("App 3 still renders the bouncing placeholder")
if "drawApp3OriginCrawl();" not in app3_branch_body:
    raise SystemExit("App 3 does not render the origin crawl")

app3_fn_start = source.index("void drawApp3OriginCrawl() {")
app3_fn_end = source.index("bool ensureApp3CrawlSprite", app3_fn_start)
app3_fn_body = source[app3_fn_start:app3_fn_end]
if "tft.fillRect(0, headerHeight" in app3_fn_body:
    raise SystemExit("Origin Story crawl clears the live TFT area directly, causing flicker")

print("App shell checks passed")
