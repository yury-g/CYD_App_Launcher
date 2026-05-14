#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/cyd-app-launcher-web-build"
SKETCH_DIR="${TMPDIR:-/tmp}/cyd-app-launcher-web-sketch/CYD_App_Launcher"
FIRMWARE_DIR="${ROOT_DIR}/docs/flasher/firmware"
FQBN="esp32:esp32:esp32:UploadSpeed=115200"

TFT_FLAGS=(
  "-DUSER_SETUP_LOADED=1"
  "-DILI9341_DRIVER=1"
  "-DTFT_WIDTH=240"
  "-DTFT_HEIGHT=320"
  "-DTFT_MISO=12"
  "-DTFT_MOSI=13"
  "-DTFT_SCLK=14"
  "-DTFT_CS=15"
  "-DTFT_DC=2"
  "-DTFT_RST=-1"
  "-DTFT_BL=21"
  "-DTFT_BACKLIGHT_ON=HIGH"
  "-DLOAD_GLCD=1"
  "-DSPI_FREQUENCY=40000000"
  "-DSPI_READ_FREQUENCY=16000000"
)

rm -rf "${BUILD_DIR}" "${SKETCH_DIR}"
mkdir -p "${SKETCH_DIR}" "${FIRMWARE_DIR}"
cp "${ROOT_DIR}/CYD_App_Launcher.ino" "${SKETCH_DIR}/"

arduino-cli compile \
  --fqbn "${FQBN}" \
  --build-path "${BUILD_DIR}" \
  --build-property "compiler.cpp.extra_flags=${TFT_FLAGS[*]}" \
  "${SKETCH_DIR}"

BUILD_OPTIONS="${BUILD_DIR}/build.options.json"
ESP32_CORE_DIR="$(node -e '
const fs = require("fs");
const options = JSON.parse(fs.readFileSync(process.argv[1], "utf8"));
const folders = String(options.hardwareFolders || "").split(",").filter(Boolean);
const esp32 = folders.find((path) => /\/esp32\/hardware\/esp32\/[^/]+$/.test(path));
if (!esp32) process.exit(1);
process.stdout.write(esp32);
' "${BUILD_OPTIONS}")"

cp "${BUILD_DIR}/CYD_App_Launcher.ino.bootloader.bin" "${FIRMWARE_DIR}/bootloader.bin"
cp "${BUILD_DIR}/CYD_App_Launcher.ino.partitions.bin" "${FIRMWARE_DIR}/partitions.bin"
cp "${BUILD_DIR}/CYD_App_Launcher.ino.bin" "${FIRMWARE_DIR}/firmware.bin"
cp "${ESP32_CORE_DIR}/tools/partitions/boot_app0.bin" "${FIRMWARE_DIR}/boot_app0.bin"

echo "Wrote ESP Web Tools firmware files to ${FIRMWARE_DIR}:"
ls -lh "${FIRMWARE_DIR}"/*.bin
