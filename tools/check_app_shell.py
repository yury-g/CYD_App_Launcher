from pathlib import Path

source = Path("PulseSensor_CYD.ino").read_text()

required_tokens = [
    "APP_VERSION",
    "APP_FIRMWARE_DATE",
    "About",
    "Version",
    "Firmware",
    "WiFi",
    "Bluetooth",
    "LED Control",
    "your app here",
    "APP_PLACEHOLDER_1",
    "APP_PLACEHOLDER_2",
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
    "#define APP3_CRAWL_TEXT_SIZE 2": "Origin Story crawl text is not enlarged",
    "TFT_eSprite app3CrawlSprite": "Origin Story crawl is missing an offscreen sprite",
    "ensureApp3CrawlSprite": "Origin Story crawl sprite is not managed for rotation changes",
    "app3CrawlSprite.pushSprite": "Origin Story crawl frames are not pushed from an offscreen sprite",
    "app3CrawlSprite.setTextSize(APP3_CRAWL_TEXT_SIZE)": "Origin Story crawl does not use the enlarged text size",
    "app3CrawlSprite.setColorDepth(8)": "Origin Story crawl sprite is not using lower-memory 8-bit color",
    "drawApp3OriginCrawlDirectFallback": "Origin Story crawl has no visible fallback if sprite allocation fails",
    "int baseY = crawlBottom - lineHeight - offset;": "Origin Story crawl starts below the visible area",
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

print("App shell checks passed")
