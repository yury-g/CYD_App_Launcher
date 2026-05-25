#!/usr/bin/env python3
"""Guard the cross-machine resume and flash entrypoint."""

from pathlib import Path

from project_metadata import read_firmware_metadata


root = Path(__file__).resolve().parents[1]
source = (root / "PulseSensor_CYD.ino").read_text()
flash_helper = (root / "tools" / "flash_current_favorite.py").read_text()
start_here = (root / "START_HERE_NEXT_CHAT.md").read_text()
handoff = (root / "CODEX_HANDOFF.md").read_text()
signal_log = (root / "docs" / "signal-behavior-log.md").read_text()
metadata = read_firmware_metadata()

required_flash_tokens = {
    'INTERNAL_REPO_URL = "https://github.com/yury-g/CYD_App_Launcher"': "flash helper must name the internal repo",
    "read_firmware_metadata()": "flash helper must read the favorite firmware version from the sketch",
    'FAVORITE_REF = "main"': "flash helper must know the favorite ref",
    "read_app_version()": "flash helper must inspect APP_VERSION",
    "--allow-version-mismatch": "flash helper must require an explicit override for experiments",
    '"upload"': "flash helper must run the PlatformIO upload target",
    "--upload-port": "flash helper must pass the selected serial port",
}

required_doc_tokens = {
    "https://github.com/yury-g/CYD_App_Launcher": "handoff docs must point at the internal repo",
    metadata.version: "handoff docs must name the favorite firmware",
    "docs/handoff-2026-05-24-0.4.41-snappy-lock.md": "handoff docs must point at the latest continuation note",
    "working like a champ": "signal log must preserve the placement clarification",
    "earlobe placement": "signal log must call out placement as part of the finding",
}

for token, message in required_flash_tokens.items():
    if token not in flash_helper:
        raise SystemExit(message)

combined_docs = "\n".join([start_here, handoff, signal_log])
for token, message in required_doc_tokens.items():
    if token not in combined_docs:
        raise SystemExit(message)

if f'#define APP_VERSION "{metadata.version}"' in source and "read_firmware_metadata" not in flash_helper:
    raise SystemExit("favorite source builds need the metadata-backed flash helper preserved")

print("Resume flash entrypoint checks passed")
