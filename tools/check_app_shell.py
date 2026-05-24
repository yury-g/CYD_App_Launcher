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
    "your app here too",
    "APP_PLACEHOLDER_1",
    "APP_PLACEHOLDER_2",
    "APP_SETTINGS",
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
if "handleRotateTouch(x, y)" not in read_touch_body:
    raise SystemExit("Rotate touch is not persistent across apps")
if "currentApp == APP_PULSE && handleRotateTouch" in read_touch_body:
    raise SystemExit("Rotate touch is still limited to the pulse app")

app_nav_start = source.index("bool handleAppNavTouch(int16_t x, int16_t y) {")
app_nav_end = source.index("bool handleRotateTouch(int16_t x, int16_t y) {", app_nav_start)
app_nav_body = source[app_nav_start:app_nav_end]
required_touch_boundaries = [
    "appPrevNextBoundary",
    "appNextSettingsBoundary",
    "appSettingsRotateBoundary",
]
missing_touch_boundaries = [
    token for token in required_touch_boundaries if token not in app_nav_body
]
if missing_touch_boundaries:
    raise SystemExit(
        "App nav touch targets can overlap instead of using midpoint boundaries: "
        + ", ".join(missing_touch_boundaries)
    )

rotate_start = source.index("bool handleRotateTouch(int16_t x, int16_t y) {")
rotate_end = source.index("bool handleVolumeTouch(int16_t x, int16_t y) {", rotate_start)
rotate_body = source[rotate_start:rotate_end]
if "appSettingsRotateBoundary" not in rotate_body:
    raise SystemExit("Rotate touch can overlap the settings touch target")

configure_start = source.index("void configureLayout() {")
configure_end = source.index("void resetDashboardState()", configure_start)
configure_body = source[configure_start:configure_end]
required_nav_layout = [
    "appSettingsButtonX = rotateButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;",
    "appNextButtonX = appSettingsButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;",
    "appPrevButtonX = appNextButtonX - APP_BUTTON_WIDTH - APP_BUTTON_GAP;",
]
missing_nav_layout = [line for line in required_nav_layout if line not in configure_body]
if missing_nav_layout:
    raise SystemExit("App nav is not attached to rotate button")

required_large_controls = {
    "#define TOOLBAR_BUTTON_WIDTH 44": "toolbar buttons are not double-width",
    "#define TOOLBAR_BUTTON_HEIGHT 28": "toolbar buttons are not one-quarter taller",
    "#define APP_BUTTON_WIDTH TOOLBAR_BUTTON_WIDTH": "app nav width does not match toolbar width",
    "#define APP_BUTTON_HEIGHT TOOLBAR_BUTTON_HEIGHT": "app nav height does not match toolbar height",
    "#define SETTINGS_TEXT_SIZE 2": "Settings screen text is not enlarged",
    "settingsScrollY": "Settings screen is missing scroll state",
    "handleSettingsScrollTouch": "Settings screen is missing scroll touch handling",
    "drawSettingsScrollControls": "Settings screen is missing large scroll controls",
    "settingsRowScreenY": "Settings controls are not using scrolled row-local hit tests",
    "settingsRowBackground": "Settings rows are missing staggered yellow/green backgrounds",
    "COLOR_SIGNAL_YELLOW": "Settings row striping is missing first-app yellow",
    "COLOR_LOCK_GREEN": "Settings row striping is missing first-app green",
    "metricPanelBackground": "Pulse dashboard metrics are missing yellow/green tile backgrounds",
    "tft.setTextColor(COLOR_BG, panelBg)": "Pulse dashboard tile text is not black on yellow/green",
    "settingsScrollButtonW = (screenWidth - APP_BUTTON_GAP) / 2": "Settings scroll buttons do not fill the bottom bar width",
    "settingsScrollUpX = 0": "Settings scroll up button is not anchored to the left edge",
}
for token, message in required_large_controls.items():
    if token not in source:
        raise SystemExit(message)

for fn_name in [
    "void drawHeader() {",
    "void drawSettingsScreen() {",
    "void drawPlaceholderApp(const char* title, const char* message) {",
]:
    fn_start = source.index(fn_name)
    fn_end = source.index("\n}", fn_start)
    fn_body = source[fn_start:fn_end]
    if "drawAppNavControls();" not in fn_body or "drawRotateControl();" not in fn_body:
        raise SystemExit(f"{fn_name} does not draw persistent nav plus rotate controls")

print("App shell checks passed")
