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

print("App shell tokens present")
