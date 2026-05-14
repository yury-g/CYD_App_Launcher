# Firmware Files

Run this from the repo root to refresh the ESP Web Tools binaries:

```bash
bash scripts/build-web-flasher-firmware.sh
```

The script compiles with the same CYD `TFT_eSPI` flags used by `flash-cyd.sh`, then writes:

- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin`
- `firmware.bin`

These files are referenced by `docs/flasher/manifest.json`.
