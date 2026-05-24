from pathlib import Path

source = Path("PulseSensor_CYD.ino").read_text()

required_tokens = [
    "APP_VERSION",
    "About",
    "Version",
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

read_touch_start = source.index("void readTouchControls() {")
read_touch_end = source.index("void mapTouchPoint", read_touch_start)
read_touch_body = source[read_touch_start:read_touch_end]
if "handleVolumeTouch" in read_touch_body:
    raise SystemExit("Pulse touch handling still uses top-bar volume controls")

configure_start = source.index("void configureLayout() {")
configure_end = source.index("void resetDashboardState()", configure_start)
configure_body = source[configure_start:configure_end]
required_nav_layout = [
    "appSettingsButtonX = rotateButtonX - APP_BUTTON_SIZE - 2;",
    "appNextButtonX = appSettingsButtonX - APP_BUTTON_SIZE - 2;",
    "appPrevButtonX = appNextButtonX - APP_BUTTON_SIZE - 2;",
]
missing_nav_layout = [line for line in required_nav_layout if line not in configure_body]
if missing_nav_layout:
    raise SystemExit("App nav is not attached to rotate button")

print("App shell checks passed")
