# CYD App Launcher — Student Version

**Five PulseSensor learning applications for the ESP32 CYD (Cheap Yellow Display)**

A teaching-focused firmware that demonstrates heart rate visualization, breathing exercises, relaxation techniques, heart rate variability analysis, and respiratory rate detection using a single sensor.

---

## 🎯 Project Vision

**Original Intent:**  
Create a student-friendly, heavily-commented example codebase that demonstrates how to build multiple interactive biofeedback applications on affordable hardware. This is a learning tool, not a branded product — designed for Parsons students, makers, and educators to understand real-time physiological data visualization.

**What This Is:**
- Educational firmware with 5 complete working apps
- Clean, readable, well-commented Arduino/ESP32 code
- No branding, no marketing copy — just good teaching examples
- Browser-flashable via ESP Web Tools (no Arduino IDE required)

**What This Is NOT:**
- Not PulseSensor Studio (the branded WFE product)
- Not production-ready commercial firmware
- Not meant to replace the PulseSensor_CYD repo (which is the reference implementation)

---

## 🧬 Hardware Requirements

**Board:** ESP32-2432S028R (CYD - "Cheap Yellow Display")  
**Display:** ILI9341 320×240 TFT with XPT2046 resistive touch  
**Sensor:** PulseSensor connected to GPIO 36  
**RGB LED:** Onboard CYD LED (Red=GPIO4, Green=GPIO16, Blue=GPIO17)

**Pinout:**
```
PULSE_PIN       = 36  (Analog input for PulseSensor signal)
TOUCH_CS        = 33  (XPT2046 touch controller chip select)
BACKLIGHT       = 21  (TFT backlight control)
LED_RED_PIN     = 4   (RGB LED red channel, active-low PWM)
LED_GREEN_PIN   = 16  (RGB LED green channel, active-low PWM)
LED_BLUE_PIN    = 17  (RGB LED blue channel, active-low PWM)
```

**Touch Calibration:**
```
X: 200–3800 raw → 0–320 screen
Y: 200–3800 raw → 0–240 screen
```

---

## 📱 The Five Applications

### 0: Heartbeat
Scrolling waveform with auto-scaling, real-time BPM/IBI display, RGB LED flash on beat, thick 4px waveform, cursor line visualization.

### 1: Breathing
Animated sine wave circle that expands/contracts in an 8-second cycle (4s inhale, 4s exhale). Green during inhale, blue during exhale.

### 2: Relaxation
Large BPM number displayed inside a color-shifting circle. Red (high BPM) gradually shifts to teal (low BPM) to encourage relaxation.

### 3: HRV (Heart Rate Variability)
Poincare plot on the left showing IBI(n) vs IBI(n-1). Right side shows RMSSD, SDNN, current IBI, BPM, and sample count.

### 4: BreathFFT
Simplified FFT visualization of IBI data to detect breathing rate. Spectrum bars show frequency components, peak frequency estimates breaths per minute.

**Navigation:** Touch top menu bar to switch between apps. Each app button is 64px wide.

---

## 🧠 Code Architecture & Key Improvements

This firmware incorporates proven patterns from the **WorldFamousElectronics/PulseSensor_CYD** reference implementation:

### RGB LED Control (ESP32 Core 3.x compatible)
```cpp
// Modern ESP32 API (not the old ledcSetup/ledcAttachPin)
ledcAttach(LED_RED_PIN, 5000, 8);  // pin, freq, resolution

// Active-low PWM (CYD hardware characteristic)
int pwmValue = 255 - brightness;
ledcWrite(LED_RED_PIN, pwmValue);
```

### Beat Detection: One-Shot Events
```cpp
// Use sawStartOfBeat() not isInsideBeat()
if (pulseSensor.sawStartOfBeat()) {
  currentBPM = pulseSensor.getBeatsPerMinute();
  ledBrightness = 255;  // Flash LED once per beat
}
```

### Auto-Scaling Waveform
```cpp
void updateMinMax() {
  // Decay toward ADC midpoint (2048) to prevent stuck scaling
  if (millis() - lastDecay > 100) {
    minSignal = min(minSignal + 5, 2048);
    maxSignal = max(maxSignal - 5, 2048);
  }
  minSignal = min(minSignal, currentSignal);
  maxSignal = max(maxSignal, currentSignal);
}
```

### No-Finger Timeout
```cpp
#define NO_BEAT_TIMEOUT 3000  // 3 seconds
if (fingerPresent && (millis() - lastBeatTime > NO_BEAT_TIMEOUT)) {
  fingerPresent = false;
  currentBPM = 0;
  currentIBI = 0;
}
```

### Visual Polish
- **4px thick waveform** instead of single-pixel line
- **Cursor line 4px ahead** of waveform (visual "writing cursor" effect)
- **Heart icon** drawn with two circles + triangle geometry

---

## 🛠️ Build Environment (KNOWN WORKING)

### arduino-cli
**Version:** 1.4.1  
**Install:** `brew install arduino-cli`  
**Location:** `/opt/homebrew/bin/arduino-cli`

### Board Package
**FQBN:** `esp32:esp32:esp32`  
**ESP32 Arduino Core:** 3.3.6  
**Install:** `arduino-cli core install esp32:esp32`

### Required Libraries
```
TFT_eSPI                 2.5.43
XPT2046_Touchscreen      1.4
PulseSensor Playground   2.5.0
```

**Install:**
```bash
arduino-cli lib install "TFT_eSPI"
arduino-cli lib install "XPT2046_Touchscreen"
arduino-cli lib install "PulseSensor Playground"
```

### TFT_eSPI Configuration
**User_Setup.h must be configured for CYD:**
```cpp
#define ILI9341_DRIVER
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_BL   21
#define TOUCH_CS 33
```

Location: `~/Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`

---

## 🔨 Compilation (Current Status)

### Compile Command
```bash
arduino-cli compile \
  --fqbn esp32:esp32:esp32 \
  --export-binaries \
  /Users/mininarwhal/Documents/Arduino/CYD_App_Launcher/
```

### Build Output Location
```
build/esp32.esp32.esp32/CYD_App_Launcher.ino.bin
build/esp32.esp32.esp32/CYD_App_Launcher.ino.bootloader.bin
build/esp32.esp32.esp32/CYD_App_Launcher.ino.partitions.bin
```

### ⚠️ KNOWN ISSUE (As of April 8, 2026)
**Compilation fails** with syntax errors that don't match file content:
- Line 88: "expected constructor before ';'" at `void updateMinMax();`
- Similar errors at lines 118, 243

**Suspected causes:**
1. File encoding issues (hidden characters in .ino file)
2. Path access mismatch between bash container and macOS filesystem
3. Need to rewrite .ino from scratch with clean encoding

**To fix in next session:**
- Option A: Rewrite entire .ino file from scratch (eliminate encoding)
- Option B: Use hexdump to find/remove hidden characters
- Option C: Try different arduino-cli invocation pattern

---

## 📦 ESP Web Tools Flashing (Target Deployment)

### Multi-Part Manifest Pattern
**CRITICAL:** ESP32 requires 4 separate binary files at specific offsets.  
Do NOT use single-bin manifest — it will fail to flash.

**Correct manifest.json structure:**
```json
{
  "name": "CYD App Launcher",
  "version": "1.0.0",
  "builds": [{
    "chipFamily": "ESP32",
    "parts": [
      { "path": "firmware/bootloader.bin", "offset": 4096 },
      { "path": "firmware/partitions.bin", "offset": 32768 },
      { "path": "firmware/boot_app0.bin", "offset": 57344 },
      { "path": "firmware/firmware.bin", "offset": 65536 }
    ]
  }]
}
```

### Where to Get bootloader/partitions/boot_app0
**Source:** WorldFamousElectronics/PulseSensor_CYD repo  
**Path:** `firmware/` folder

These are board-specific binaries that can be reused directly:
```bash
# Clone reference repo to get pre-built system binaries
git clone https://github.com/WorldFamousElectronics/PulseSensor_CYD.git
cp PulseSensor_CYD/firmware/bootloader.bin ./firmware/
cp PulseSensor_CYD/firmware/partitions.bin ./firmware/
cp PulseSensor_CYD/firmware/boot_app0.bin ./firmware/
```

Only `firmware.bin` needs to be compiled from this project's .ino file.

### Web Flasher Deployment
**Planned URL:** `https://yury-g.github.io/CYD_App_Launcher/`  
**Files needed in repo root:**
- `index.html` (ESP Web Tools interface)
- `manifest.json` (multi-part manifest)
- `firmware/` folder with all 4 .bin files

---

## 📚 Reference Materials

### Code Mine Source
**Repo:** [WorldFamousElectronics/PulseSensor_CYD](https://github.com/WorldFamousElectronics/PulseSensor_CYD)  
**Live flasher:** https://worldfamouselectronics.github.io/PulseSensor_CYD/

**Key differences from our version:**
- Signal pin GPIO 35 (we use 36)
- No touch/menu system (single app only)
- Pre-compiled .bin files available
- Proven manifest.json pattern
- Working ESP Web Tools implementation

### Related Repos
- **PulseSensor_CYD_Studio** (parked/inactive) — attempted multi-app WFE product version
- **PulseSensor_CYD** (reference) — single-app production firmware
- **CYD_App_Launcher** (this repo) — student teaching version

### Documentation Read
- Apple Note: "CYD CODE MINE — PulseSensor_CYD Repo Analysis"
- Apple Note: "HANDOFF — CYD App Launcher Debug & Compile — April 8 2026"

---

## 🎓 Teaching Notes

**Pedagogical Intent:**
This firmware is designed to be read and understood by students learning embedded systems, biofeedback, and real-time signal processing. Every function is commented, every algorithm explained.

**Topics demonstrated:**
- Analog sensor reading and signal processing
- Touch UI navigation and state machines
- PWM LED control with active-low hardware
- Auto-scaling algorithms for dynamic range
- Time-based event detection (beat onset, timeouts)
- Statistical calculations (mean, standard deviation, RMSSD)
- Frequency domain analysis (simplified FFT)
- Drawing primitives and geometric construction
- Breathing exercise pacing and biofeedback

**Not covered (intentionally simplified):**
- Advanced FFT libraries (arduinoFFT not used)
- Complex UI frameworks
- WiFi/Bluetooth connectivity
- Data logging or SD card storage
- Multi-core processing

---

## 🚧 Current Development Status

**Version:** v0.2.0 (April 8, 2026)  
**Status:** ⚠️ Code complete, compilation blocked

**What works:**
- ✅ Complete .ino file written (531 lines)
- ✅ All 5 apps implemented
- ✅ Code improvements integrated from reference repo
- ✅ Git repo initialized and pushed to GitHub
- ✅ CHANGELOG.md tracking progress

**What's blocked:**
- ❌ Compilation fails (encoding issue suspected)
- ❌ No .bin files generated yet
- ❌ Web flasher not deployed
- ❌ No hardware testing yet

**Next session must:**
1. Fix compilation error (rewrite .ino with clean encoding)
2. Generate all 4 .bin files
3. Create multi-part manifest.json
4. Copy bootloader/partitions/boot_app0 from reference repo
5. Test flash on actual hardware
6. Deploy web flasher to GitHub Pages
7. Tag v1.0.0

---

## 🔧 Troubleshooting & Lessons Learned

### Common Pitfalls
1. **Single-bin manifest fails** — Must use 4-part manifest with proper offsets
2. **ledcSetup() deprecated** — Use ledcAttach() for ESP32 Core 3.x
3. **isInsideBeat() vs sawStartOfBeat()** — Use saw* for one-shot events
4. **Active-low RGB LED** — CYD hardware inverts brightness (255 = off, 0 = full on)
5. **Touch calibration varies** — Raw values 200-3800 are approximate, may need adjustment

### File Encoding Issue (Current)
**Symptom:** Compiler reports syntax errors at valid function declarations  
**Cause:** Unknown — possibly hidden characters or encoding problem  
**Solution:** Rewrite .ino from scratch with verified clean UTF-8 encoding

### Desktop Commander vs bash_tool
**Lesson:** bash_tool runs in container, can't access Mac filesystem  
**Solution:** Always use desktop-commander tools (start_process, write_file) for Mac operations

---

## 📞 Contact & Attribution

**Author:** Yury Gitman (yury@pulsesensor.com)  
**Organization:** World Famous Electronics LLC / Parsons School of Design  
**Course Context:** PSAM 5320 (Making Wireless Toys) Spring 2026

**License:** MIT (educational use encouraged)  
**Reference Implementation:** [PulseSensor_CYD by WorldFamousElectronics](https://github.com/WorldFamousElectronics/PulseSensor_CYD)

---

## 🎯 Success Criteria (v1.0.0)

- [ ] Compiles without errors
- [ ] All 4 .bin files generated
- [ ] Web flasher live at GitHub Pages
- [ ] Successfully flashes to CYD hardware
- [ ] All 5 apps launch and respond to touch
- [ ] PulseSensor data displays in real-time
- [ ] RGB LED flashes on each heartbeat
- [ ] Touch navigation works between all apps
- [ ] Code is readable and well-commented for students

**Definition of Done:** A student with no prior ESP32 experience can:
1. Visit the GitHub Pages URL
2. Plug in a CYD board via USB
3. Flash the firmware in under 60 seconds
4. Attach a PulseSensor and see their heartbeat
5. Read the .ino file and understand how it works

---

**Last updated:** April 8, 2026  
**Status:** v0.2.0 — Awaiting compilation fix
