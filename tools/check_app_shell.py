from pathlib import Path

source = Path("PulseSensor_CYD.ino").read_text()

required_tokens = [
    "APP_VERSION",
    "APP_FIRMWARE_DATE",
    "About",
    "Version",
    "Firmware",
    "Memory",
    "WiFi",
    "Bluetooth",
    "LED Control",
    "your app here",
    "APP_PLACEHOLDER_1",
    "APP_PLACEHOLDER_2",
    "APP_PIN_SCANNER",
    "APP_SETTINGS",
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

read_touch_start = source.index("void readTouchControls() {")
read_touch_end = source.index("void mapTouchPoint", read_touch_start)
read_touch_body = source[read_touch_start:read_touch_end]
if "handleVolumeTouch" in read_touch_body:
    raise SystemExit("Pulse touch handling still uses top-bar volume controls")
if "handleRotateTouch(x, y)" in read_touch_body:
    raise SystemExit("Rotate touch is still exposed as a persistent top-bar control")

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
    "#define SETTINGS_TEXT_SIZE 2": "Settings screen text is not enlarged",
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
    "Pin Scanner": "App 4 pin scanner title is missing",
    "PIN_SCANNER_PIN_COUNT": "App 4 pin scanner pin table is missing",
    "ScannerPin scannerPins[]": "App 4 pin scanner data model is missing",
    "#define PIN_SCANNER_PIN_COUNT 4": "App 4 should list GPIO35, GPIO22, GPIO21, and GPIO27",
    "scannerActiveIndex = -1": "App 4 scanner should start with no active pin selected",
    "handlePinScannerTouch": "App 4 scanner should let one pin be selected at a time",
    "isPinScannerAdcCapable": "App 4 scanner should guard non-ADC pins before reading",
    "pinScannerStatusText": "App 4 scanner should explain inactive and non-ADC pin states",
    "HOT_MOVEMENT_MIN": "App 4 pin scanner hot movement threshold is missing",
    "SORT_INTERVAL_MS": "App 4 pin scanner sort interval is missing",
    "SORT_HYSTERESIS": "App 4 pin scanner sort hysteresis is missing",
    "setupPinScanner": "App 4 pin scanner setup is missing",
    "updateActivePinScannerReading": "App 4 pin scanner should update only the selected pin",
    "maybeSortScannerPins": "App 4 pin scanner auto-sort is missing",
    "drawApp4PinScanner": "App 4 pin scanner renderer is missing",
    "drawPinScannerRow": "App 4 pin scanner row renderer is missing",
    "pinScannerHotColor": "App 4 pin scanner hot color is not mode-aware",
    "pinScannerBarColor": "App 4 pin scanner bar color is not mode-aware",
    "pinScannerRailColor": "App 4 pin scanner rail color is not mode-aware",
    "analogReadResolution(12);": "App 4 pin scanner does not switch ADC reads to 12-bit",
    "analogReadResolution(10);": "Pulse app does not restore 10-bit ADC reads",
}
for token, message in required_large_controls.items():
    if token not in source:
        raise SystemExit(message)

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

expected_app_order = [
    "APP_PULSE",
    "APP_SETTINGS",
    "APP_PIN_SCANNER",
    "APP_PLACEHOLDER_1",
    "APP_PLACEHOLDER_2",
    "APP_COUNT",
]
enum_start = source.index("enum AppId {")
enum_end = source.index("};", enum_start)
enum_body = source[enum_start:enum_end]
enum_positions = [enum_body.index(token) for token in expected_app_order]
if enum_positions != sorted(enum_positions):
    raise SystemExit("App order should be Pulse, Settings, Pin Scanner, your-app-here, Origin Story")

if "if (next > APP_PLACEHOLDER_2) next = APP_PULSE;" not in source:
    raise SystemExit("App next navigation should wrap only after final Origin Story screen")

if "previous == APP_PULSE ? APP_PLACEHOLDER_2" not in source:
    raise SystemExit("App previous navigation should wrap back to final Origin Story screen")

app4_branch_start = source.index("} else if (currentApp == APP_PIN_SCANNER) {")
app4_branch_end = source.index("\n  }", app4_branch_start)
app4_branch_body = source[app4_branch_start:app4_branch_end]
if "drawApp4PinScanner();" not in app4_branch_body:
    raise SystemExit("App 4 does not render the pin scanner")

app4_fn_start = source.index("void drawApp4PinScanner() {")
app4_fn_end = source.index("void drawPinScannerRow", app4_fn_start)
app4_fn_body = source[app4_fn_start:app4_fn_end]
if "drawAppNavControls();" not in app4_fn_body:
    raise SystemExit("App 4 pin scanner does not draw persistent nav controls")
if "displayModeName()" not in app4_fn_body:
    raise SystemExit("App 4 pin scanner does not show the active display mode")

scanner_start = source.index("ScannerPin scannerPins[] = {")
scanner_end = source.index("};", scanner_start)
scanner_body = source[scanner_start:scanner_end]
for unsafe_pin in ['"LDR IO34"', '"IO32"', '"IO33"', ", 32,", ", 33,"]:
    if unsafe_pin in scanner_body:
        raise SystemExit(f"App 4 pin scanner includes unsafe integrated-app scan target: {unsafe_pin}")
for required_pin in ['"P3  IO35"', '"IO22"', '"BL  IO21"', '"CN1 IO27"']:
    if required_pin not in scanner_body:
        raise SystemExit(f"App 4 pin scanner is missing external connector target: {required_pin}")

active_update_start = source.index("void updateActivePinScannerReading() {")
active_update_end = source.index("void maybeSortScannerPins()", active_update_start)
active_update_body = source[active_update_start:active_update_end]
if "analogRead(21)" in active_update_body or "analogRead(22)" in active_update_body:
    raise SystemExit("App 4 scanner still directly reads known non-ADC pins")
if "isPinScannerAdcCapable(scannerPins[scannerActiveIndex].pin)" not in active_update_body:
    raise SystemExit("App 4 scanner does not guard analogRead with ADC capability")

print("App shell checks passed")
