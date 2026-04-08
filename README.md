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

## 📸 Visual Overview

### App Screenshots & UI Layout

```
┌─────────────────────────────────────────────────────────┐
│  BEAT │ BREATH │ RELAX │  HRV  │  FFT  │  (Menu Bar)   │
├═════════════════════════════════════════════════════════┤
│                                                         │
│  APP 0: HEARTBEAT VISUALIZER                            │
│  ┌───────────────────────────────────────────────────┐  │
│  │ ▁▂▃▄▅▆▇█▇▆▅▄▃▂▁▂▃▄▅▆▇█  (scrolling waveform)     │  │
│  │                      ●  (beat flash dot)          │  │
│  │                                                    │  │
│  └───────────────────────────────────────────────────┘  │
│  BPM: 72      IBI: 833ms                         ♥      │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│  APP 1: BREATHING TRAINER                               │
│                                                         │
│                      ◯                                  │
│                    ◯   ◯                                │
│                  ◯  IN   ◯  (expanding circle)          │
│                    ◯   ◯                                │
│                      ◯                                  │
│                                                         │
│  BPM: 68                                                │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│  APP 2: RELAXATION                                      │
│                                                         │
│                    ┌─────┐                              │
│                    │  72 │  (large BPM in color circle) │
│                    └─────┘                              │
│                                                         │
│  [RED ========●======= TEAL]  (stress spectrum bar)     │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│  APP 3: HRV DASHBOARD                                   │
│  ┌──────────────┐  │  RMSSD: 42ms                       │
│  │  •  •        │  │  SDNN:  58ms                       │
│  │    • •  •    │  │  IBI:   847ms                      │
│  │  •    •   •  │  │  BPM:   71                         │
│  │    •  •      │  │  N:     42 beats                   │
│  └──────────────┘  │                                    │
│  (Poincaré plot)   │  (metrics panel)                   │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│  APP 4: BREATH FFT                                      │
│                                                         │
│  ▌ ▌ ▌ █ ▌ ▌ ▌ ▌  (frequency spectrum bars)            │
│  │ │ │ │ │ │ │ │                                       │
│  └─┴─┴─┴─┴─┴─┴─┘                                       │
│      ↑ peak                                             │
│                                                         │
│  Breathing: 7.2/min                                     │
└─────────────────────────────────────────────────────────┘
```

### System Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                       CYD HARDWARE                           │
│  ┌────────────┐  ┌────────────┐  ┌──────────────┐           │
│  │ ILI9341    │  │ XPT2046    │  │ ESP32        │           │
│  │ 320x240    │  │ Touch      │  │ 240MHz       │           │
│  │ TFT        │  │ Controller │  │ WiFi+BT      │           │
│  └────────────┘  └────────────┘  └──────────────┘           │
│         ↓                ↓               ↓                   │
│  ┌───────────────────────────────────────────────┐           │
│  │          TFT_eSPI + XPT2046 Libraries         │           │
│  └───────────────────────────────────────────────┘           │
└──────────────────────────────────────────────────────────────┘
                           ↓
┌──────────────────────────────────────────────────────────────┐
│              CYD_APP_LAUNCHER.INO (Main Loop)                │
│  ┌──────────────────────────────────────────────┐            │
│  │  setup() → init hardware, draw menu          │            │
│  │  loop()  → read touch, read sensor, route    │            │
│  └──────────────────────────────────────────────┘            │
│                           ↓                                  │
│  ┌──────────────────────────────────────────────┐            │
│  │            APP ROUTER (switch/case)           │            │
│  │  case 0: runHeartbeat()                      │            │
│  │  case 1: runBreathing()                      │            │
│  │  case 2: runRelaxation()                     │            │
│  │  case 3: runHRV()                            │            │
│  │  case 4: runBreathFFT()                      │            │
│  └──────────────────────────────────────────────┘            │
└──────────────────────────────────────────────────────────────┘
                           ↓
┌──────────────────────────────────────────────────────────────┐
│              PULSESENSOR (GPIO 36)                           │
│  ┌──────────────────────────────────────────────┐            │
│  │  PulseSensor Playground Library              │            │
│  │  - Analog read @ ~500Hz                      │            │
│  │  - Beat detection algorithm                  │            │
│  │  - BPM / IBI calculation                     │            │
│  └──────────────────────────────────────────────┘            │
└──────────────────────────────────────────────────────────────┘
```

### File Structure

```
CYD_App_Launcher/
├── CYD_App_Launcher.ino   ← Main firmware (531 lines)
│   ├── Pin definitions
│   ├── Global state variables
│   ├── setup() - Initialize all hardware
│   ├── loop() - Touch handling + app routing
│   ├── RGB LED functions (ledcAttach API)
│   ├── Auto-scaling waveform helpers
│   ├── Drawing helper functions
│   └── Five app functions:
│       ├── runHeartbeat()   - Scrolling waveform
│       ├── runBreathing()   - Breathing pacer
│       ├── runRelaxation()  - Color biofeedback
│       ├── runHRV()         - Poincaré + metrics
│       └── runBreathFFT()   - RSA breathing detection
│
├── README.md              ← This file
├── CHANGELOG.md           ← Version history
├── .gitignore
│
├── build/                 ← Created by arduino-cli compile
│   └── esp32.esp32.esp32/
│       ├── CYD_App_Launcher.ino.bin
│       ├── CYD_App_Launcher.ino.bootloader.bin
│       └── CYD_App_Launcher.ino.partitions.bin
│
└── firmware/              ← For ESP Web Tools deployment
    ├── bootloader.bin     (from PulseSensor_CYD repo)
    ├── partitions.bin     (from PulseSensor_CYD repo)
    ├── boot_app0.bin      (from PulseSensor_CYD repo)
    └── firmware.bin       (compiled from this project)
```

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
<div align="center">

# 📱 Visual App Gallery

*What you'll see on the 320×240 CYD display*

</div>

## App 0: Heartbeat Visualizer

<div align="center">

<svg width="320" height="240" viewBox="0 0 320 240" xmlns="http://www.w3.org/2000/svg">
  <!-- CYD Screen Background -->
  <rect width="320" height="240" fill="#000"/>
  
  <!-- Menu Bar -->
  <rect x="0" y="0" width="64" height="30" fill="#E63946"/>
  <text x="10" y="20" fill="#fff" font-size="10" font-family="monospace">BEAT</text>
  <rect x="64" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="70" y="20" fill="#999" font-size="10" font-family="monospace">BREATH</text>
  <rect x="128" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="135" y="20" fill="#999" font-size="10" font-family="monospace">RELAX</text>
  <rect x="192" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="202" y="20" fill="#999" font-size="10" font-family="monospace">HRV</text>
  <rect x="256" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="265" y="20" fill="#999" font-size="10" font-family="monospace">FFT</text>
  
  <!-- Waveform Area -->
  <rect x="0" y="40" width="320" height="140" fill="#0a0a0a"/>
  
  <!-- Scrolling Waveform (cyan wave) -->
  <path d="M0,120 Q10,100 20,110 T40,120 T60,110 T80,130 T100,100 T120,140 T140,90 T160,130 T180,110 T200,125 T220,105 T240,120 T260,115 T280,125 T300,110 T320,120" 
        stroke="#07FF" stroke-width="4" fill="none"/>
  
  <!-- Beat Flash Dot -->
  <circle cx="280" cy="90" r="8" fill="#E63946"/>
  
  <!-- Grid Lines -->
  <line x1="0" y1="90" x2="320" y2="90" stroke="#1a1a1a" stroke-width="1"/>
  <line x1="0" y1="130" x2="320" y2="130" stroke="#1a1a1a" stroke-width="1"/>
  
  <!-- BPM Display -->
  <text x="10" y="210" fill="#fff" font-size="18" font-family="monospace">BPM: 72</text>
  <text x="160" y="210" fill="#fff" font-size="18" font-family="monospace">IBI: 833</text>
  
  <!-- Heart Icon -->
  <circle cx="288" cy="215" r="8" fill="#E63946"/>
  <circle cx="304" cy="215" r="8" fill="#E63946"/>
  <path d="M 280,219 L 296,233 L 312,219" fill="#E63946"/>
</svg>

**Heartbeat Visualizer** — Scrolling waveform at ~60 FPS with auto-scaling. Red dot flashes on each beat. RGB LED mirrors the flash. BPM and IBI update in real-time.

</div>

---

## App 1: Breathing Trainer

<div align="center">

<svg width="320" height="240" viewBox="0 0 320 240" xmlns="http://www.w3.org/2000/svg">
  <rect width="320" height="240" fill="#000"/>
  
  <!-- Menu Bar (inactive) -->
  <rect x="0" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="10" y="20" fill="#999" font-size="10" font-family="monospace">BEAT</text>
  <rect x="64" y="0" width="64" height="30" fill="#22D3EE"/>
  <text x="70" y="20" fill="#000" font-size="10" font-family="monospace">BREATH</text>
  <rect x="128" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="135" y="20" fill="#999" font-size="10" font-family="monospace">RELAX</text>
  <rect x="192" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="202" y="20" fill="#999" font-size="10" font-family="monospace">HRV</text>
  <rect x="256" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="265" y="20" fill="#999" font-size="10" font-family="monospace">FFT</text>
  
  <!-- Breathing Circle (expanding, cyan) -->
  <circle cx="160" cy="120" r="60" fill="none" stroke="#22D3EE" stroke-width="3"/>
  <circle cx="160" cy="120" r="50" fill="none" stroke="#22D3EE" stroke-width="2" opacity="0.5"/>
  <circle cx="160" cy="120" r="40" fill="none" stroke="#22D3EE" stroke-width="2" opacity="0.3"/>
  
  <!-- Inhale Text -->
  <text x="128" y="125" fill="#22D3EE" font-size="20" font-family="monospace" font-weight="bold">INHALE</text>
  
  <!-- BPM Display -->
  <text x="10" y="220" fill="#999" font-size="14" font-family="monospace">BPM: 68</text>
</svg>

**Breathing Trainer** — Circle expands (4s inhale) and contracts (4s exhale) in an 8-second cycle. Paces you at 7.5 breaths/min for relaxation. BPM shown as you calm down.

</div>

---

## App 2: Relaxation Mode

<div align="center">

<svg width="320" height="240" viewBox="0 0 320 240" xmlns="http://www.w3.org/2000/svg">
  <rect width="320" height="240" fill="#000"/>
  
  <!-- Menu Bar -->
  <rect x="0" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="10" y="20" fill="#999" font-size="10" font-family="monospace">BEAT</text>
  <rect x="64" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="70" y="20" fill="#999" font-size="10" font-family="monospace">BREATH</text>
  <rect x="128" y="0" width="64" height="30" fill="#4ADE80"/>
  <text x="135" y="20" fill="#000" font-size="10" font-family="monospace">RELAX</text>
  <rect x="192" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="202" y="20" fill="#999" font-size="10" font-family="monospace">HRV</text>
  <rect x="256" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="265" y="20" fill="#999" font-size="10" font-family="monospace">FFT</text>
  
  <!-- Color Circle (gradient from red to teal, showing ~72 BPM = mid-range) -->
  <circle cx="160" cy="110" r="60" fill="url(#relaxGrad)"/>
  <defs>
    <linearGradient id="relaxGrad" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" style="stop-color:#FFA500;stop-opacity:1" />
      <stop offset="100%" style="stop-color:#4ADE80;stop-opacity:1" />
    </linearGradient>
  </defs>
  
  <!-- BPM Number -->
  <text x="132" y="125" fill="#000" font-size="36" font-family="monospace" font-weight="bold">72</text>
  
  <!-- Stress Spectrum Bar -->
  <rect x="40" y="190" width="240" height="12" rx="6" fill="#1a1a1a"/>
  <rect x="40" y="190" width="120" height="12" rx="6" fill="url(#specGrad)"/>
  <circle cx="160" cy="196" r="8" fill="#FFA500" stroke="#000" stroke-width="2"/>
  <defs>
    <linearGradient id="specGrad">
      <stop offset="0%" style="stop-color:#E63946"/>
      <stop offset="100%" style="stop-color:#22D3EE"/>
    </linearGradient>
  </defs>
  <text x="40" y="218" fill="#E63946" font-size="10" font-family="monospace">STRESS</text>
  <text x="232" y="218" fill="#22D3EE" font-size="10" font-family="monospace">RELAXED</text>
</svg>

**Relaxation Mode** — Large BPM inside a color-changing circle. Circle shifts from red (stressed) → orange → green → teal (relaxed) as heart rate drops. Live biofeedback loop.

</div>

---

## App 3: HRV Dashboard

<div align="center">

<svg width="320" height="240" viewBox="0 0 320 240" xmlns="http://www.w3.org/2000/svg">
  <rect width="320" height="240" fill="#000"/>
  
  <!-- Menu Bar -->
  <rect x="0" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="10" y="20" fill="#999" font-size="10" font-family="monospace">BEAT</text>
  <rect x="64" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="70" y="20" fill="#999" font-size="10" font-family="monospace">BREATH</text>
  <rect x="128" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="135" y="20" fill="#999" font-size="10" font-family="monospace">RELAX</text>
  <rect x="192" y="0" width="64" height="30" fill="#FACC15"/>
  <text x="202" y="20" fill="#000" font-size="10" font-family="monospace">HRV</text>
  <rect x="256" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="265" y="20" fill="#999" font-size="10" font-family="monospace">FFT</text>
  
  <!-- Poincaré Plot (left) -->
  <rect x="10" y="45" width="140" height="140" fill="#0a0a0a" stroke="#2a2a2a" stroke-width="1"/>
  <text x="12" y="56" fill="#666" font-size="8" font-family="monospace">IBI(n+1)</text>
  
  <!-- Scatter points (Poincaré) -->
  <circle cx="50" cy="100" r="2" fill="#07FF"/>
  <circle cx="70" cy="110" r="2" fill="#07FF"/>
  <circle cx="90" cy="95" r="2" fill="#07FF"/>
  <circle cx="65" cy="120" r="2" fill="#07FF"/>
  <circle cx="80" cy="105" r="2" fill="#07FF"/>
  <circle cx="100" cy="115" r="2" fill="#07FF"/>
  <circle cx="55" cy="90" r="2" fill="#07FF"/>
  <circle cx="75" cy="125" r="2" fill="#07FF"/>
  <circle cx="85" cy="100" r="2" fill="#07FF"/>
  <circle cx="95" cy="108" r="2" fill="#07FF"/>
  <circle cx="105" cy="112" r="2" fill="#07FF"/>
  <circle cx="60" cy="103" r="2" fill="#07FF"/>
  
  <!-- Metrics Panel (right) -->
  <text x="165" y="65" fill="#22D3EE" font-size="11" font-family="monospace">RMSSD</text>
  <text x="240" y="65" fill="#22D3EE" font-size="16" font-family="monospace" font-weight="bold">42</text>
  <text x="280" y="65" fill="#666" font-size="10" font-family="monospace">ms</text>
  
  <text x="165" y="90" fill="#FACC15" font-size="11" font-family="monospace">SDNN</text>
  <text x="240" y="90" fill="#FACC15" font-size="16" font-family="monospace" font-weight="bold">58</text>
  <text x="280" y="90" fill="#666" font-size="10" font-family="monospace">ms</text>
  
  <text x="165" y="115" fill="#E63946" font-size="11" font-family="monospace">IBI</text>
  <text x="230" y="115" fill="#E63946" font-size="16" font-family="monospace" font-weight="bold">847</text>
  <text x="280" y="115" fill="#666" font-size="10" font-family="monospace">ms</text>
  
  <text x="165" y="140" fill="#4ADE80" font-size="11" font-family="monospace">BPM</text>
  <text x="240" y="140" fill="#4ADE80" font-size="16" font-family="monospace" font-weight="bold">71</text>
  
  <text x="165" y="170" fill="#666" font-size="10" font-family="monospace">42 beats logged</text>
</svg>

**HRV Dashboard** — Poincaré plot (left) shows each IBI vs. the next. Wide scatter = good variability. Metrics panel (right) shows RMSSD, SDNN, current IBI, and BPM. Needs ~10 beats to populate.

</div>

---

## App 4: Breath FFT

<div align="center">

<svg width="320" height="240" viewBox="0 0 320 240" xmlns="http://www.w3.org/2000/svg">
  <rect width="320" height="240" fill="#000"/>
  
  <!-- Menu Bar -->
  <rect x="0" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="10" y="20" fill="#999" font-size="10" font-family="monospace">BEAT</text>
  <rect x="64" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="70" y="20" fill="#999" font-size="10" font-family="monospace">BREATH</text>
  <rect x="128" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="135" y="20" fill="#999" font-size="10" font-family="monospace">RELAX</text>
  <rect x="192" y="0" width="64" height="30" fill="#2a2a2a"/>
  <text x="202" y="20" fill="#999" font-size="10" font-family="monospace">HRV</text>
  <rect x="256" y="0" width="64" height="30" fill="#a855f7"/>
  <text x="265" y="20" fill="#fff" font-size="10" font-family="monospace">FFT</text>
  
  <!-- FFT Spectrum Bars -->
  <rect x="20" y="120" width="16" height="20" fill="#07FF"/>
  <rect x="40" y="110" width="16" height="30" fill="#07FF"/>
  <rect x="60" y="100" width="16" height="40" fill="#07FF"/>
  <rect x="80" y="80" width="16" height="60" fill="#E63946"/>
  <rect x="100" y="95" width="16" height="45" fill="#07FF"/>
  <rect x="120" y="105" width="16" height="35" fill="#07FF"/>
  <rect x="140" y="115" width="16" height="25" fill="#07FF"/>
  <rect x="160" y="120" width="16" height="20" fill="#07FF"/>
  <rect x="180" y="125" width="16" height="15" fill="#07FF"/>
  <rect x="200" y="128" width="16" height="12" fill="#07FF"/>
  <rect x="220" y="130" width="16" height="10" fill="#07FF"/>
  <rect x="240" y="132" width="16" height="8" fill="#07FF"/>
  <rect x="260" y="134" width="16" height="6" fill="#07FF"/>
  <rect x="280" y="135" width="16" height="5" fill="#07FF"/>
  
  <!-- Peak Marker -->
  <text x="82" y="75" fill="#E63946" font-size="12" font-family="monospace">↑ peak</text>
  
  <!-- Breathing Rate Display -->
  <text x="60" y="170" fill="#fff" font-size="16" font-family="monospace">Breathing: 7.2/min</text>
  
  <!-- Grid Lines -->
  <line x1="20" y1="140" x2="300" y2="140" stroke="#2a2a2a" stroke-width="1"/>
</svg>

**Breath FFT** — Measures your actual breathing rate from heart rate variability (RSA). Spectrum bars show frequency components of IBI data. Peak frequency (red bar) indicates detected breathing rate. Needs ~16 beats (~15 sec) to lock on.

</div>

---

<div align="center">

## 🏗️ System Architecture (Visual)

</div>

```
┌─────────────────────────────────────────────────────────────────┐
│                     ⚡ ESP32-2432S028R (CYD)                    │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────────────┐ │
│  │  ILI9341    │  │  XPT2046     │  │  ESP32 (240MHz)        │ │
│  │  320×240    │  │  Touch       │  │  WiFi + Bluetooth      │ │
│  │  TFT LCD    │  │  Resistive   │  │  Dual Core             │ │
│  └──────┬──────┘  └──────┬───────┘  └────────┬───────────────┘ │
│         │                │                   │                  │
│         │                │                   │                  │
