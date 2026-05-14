# Publishing Notes

This project is headed toward a beginner-friendly PulseSensor CYD release that can eventually live alongside or inside `WorldFamousElectronics/PulseSensor_CYD`.

## What To Carry Forward

- Keep the mature firmware behavior from this repo: qualified-beat gating, change-driven dashboard redraws, touch volume, heartbeat tone, animated heart, re-arm count, and the `GPIO 35` PulseSensor input.
- Keep the Arduino CLI build path and compile-time CYD `TFT_eSPI` flags. Do not require users or maintainers to edit a global `User_Setup.h`.
- Keep the one-file Arduino sketch for approachability.

## What To Adopt From PulseSensor_CYD

- A WebSerial install path using ESP Web Tools.
- A simple wiring table for PulseSensor red, black, and purple wires.
- A strong 3.3V warning.
- Beginner troubleshooting for USB cables, flat waveforms, and erratic readings.
- A public-facing README shape: install first, wiring second, what-you-see third, developer build details later.

## Web Flasher Prototype

The prototype lives in `docs/flasher/`:

- `index.html` provides the browser install page.
- `manifest.json` lists the ESP32 binary parts and flash offsets.
- `firmware/` contains the binary parts used by ESP Web Tools.

Refresh the binary parts with:

```bash
bash scripts/build-web-flasher-firmware.sh
```

That script uses the same board family, upload speed, and display flags as `flash-cyd.sh`. It copies:

- app firmware to `docs/flasher/firmware/firmware.bin` at offset `0x10000`
- bootloader to `docs/flasher/firmware/bootloader.bin` at offset `0x1000`
- partitions to `docs/flasher/firmware/partitions.bin` at offset `0x8000`
- `boot_app0.bin` from the installed ESP32 Arduino core to offset `0xE000`

## Release Checklist

1. Run `bash tests/host/test_beat_tone_contract.sh`.
2. Run `bash scripts/build-web-flasher-firmware.sh`.
3. Run `xmllint --noout docs/flasher/index.html docs/flasher/manifest.json`.
4. Serve the repo locally and open `docs/flasher/index.html` in a WebSerial-capable browser.
5. Install on a CYD and verify display, PulseSensor input, sound, touch volume, and LED behavior.
6. Update README screenshots and the flasher version before publishing.
