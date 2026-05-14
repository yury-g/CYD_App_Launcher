# CYD Pulse Dashboard

![CYD Pulse Dashboard lab banner](docs/readme-banner.svg)

One-screen PulseSensor dashboard for the ESP32 CYD / Cheap Yellow Display.

This software is an educational biofeedback demo and is not intended for medical use.

| Start Here | What You Do | What You Learn |
| --- | --- | --- |
| Flash A Connected CYD | Install the firmware on the board | How code gets onto the ESP32 |
| Wire The PulseSensor | Connect red, black, and purple wires | How the signal reaches GPIO 35 |
| Watch The Screen | Compare search and qualified beat states | How raw pulse data becomes feedback |

## Flash A Connected CYD

Goal: plug in a CYD, flash it, then wire the PulseSensor.

### Option A: Browser Flasher Prototype

| Status | What this means |
| --- | --- |
| Stand-in dummy module | Good for shaping the install flow, not the final public installer yet |
| Firmware files are real | The binary artifacts are generated from this repo |
| Final polish later | We will finish the one-click installer in another session |

A prototype ESP Web Tools flasher lives at:

```text
docs/flasher/index.html
```

To refresh the flasher firmware files:

```bash
bash scripts/build-web-flasher-firmware.sh
```

See `docs/publishing.md` for the eventual publishing flow.

### Option B: Local USB Flash

This is the currently verified flashing path.

1. Plug in one CYD over USB.
2. Detect the serial port fresh:

```bash
arduino-cli board list
```

3. Flash the detected port:

```bash
./flash-cyd.sh /dev/cu.usbserial-3110
```

If your serial port is different, pass the detected port as the first argument. The helper script includes the required `TFT_eSPI` CYD display compile flags and uses a `115200` upload speed.

## Wire The PulseSensor

| PulseSensor Wire | CYD Connection |
| --- | --- |
| Red (+V) | 3.3V on CN1 |
| Black (GND) | GND on P3 or CN1 |
| Purple (Signal) | GPIO 35 on P3 |

Use 3.3V, not 5V. This firmware expects the PulseSensor signal on `GPIO 35`.

## What You Should See

| Searching for signal | Locked on qualified beat |
| --- | --- |
| ![Searching signal screen](docs/screenshots/searching.svg) | ![Locked qualified beat screen](docs/screenshots/locked.svg) |

The display has one screen:

- `SIGNAL SEARCH` while the firmware is looking for reliable beats
- `QUALIFIED BEAT` after the quality meter reaches lock
- A centered animated heart in the header
- Header touch controls for speaker volume, starting at `2/10`
- A live scrolling line graph in the center
- Lower panels for `BPM`, `IBI`, and signal quality
- `R#` in the quality panel shows how many automatic detector re-arms have happened

The waveform and heart outline share the same live trace color: cyan while searching and white when the signal is qualified.

## Feature List

This firmware is tuned for a working CYD hardware setup with:

- Live PulseSensor waveform
- Qualified beat detection
- BPM readout
- IBI readout
- 12-step signal quality meter
- Centered animated heart with a live-trace outline
- Heartbeat tone / sound effect on qualified beats through the CYD speaker
- Touch volume controls in the header
- Rear RGB LED red blink/fade on qualified beats
- Auto re-arm when the waveform looks alive but the detector is not producing beat events

| Feedback Channel | What the student notices |
| --- | --- |
| Screen | Waveform, BPM, IBI, quality, and lock state |
| Light | Rear red LED blinks and fades on qualified beats |
| Sound | A short heartbeat tone plays on qualified beats |
| Touch | Header buttons change the speaker volume |

## Quick Troubleshooting

**No serial port?** Try a different USB cable. Many micro-USB cables are charge-only.

**Flat waveform?** Check that the purple PulseSensor wire is connected to `GPIO 35`.

**Erratic readings?** Use gentle, steady finger pressure and insulate the back of the PulseSensor.

## PulseSensor Playground Tie-Ins

The firmware uses PulseSensorPlayground for beat detection:

- `getLatestSample()` for the waveform
- `sawStartOfBeat()` for beat events
- `getBeatsPerMinute()` for BPM
- `getInterBeatIntervalMs()` for IBI
- `getPulseAmplitude()` as part of signal qualification

The 12-step quality meter rises on qualified beats and falls gently on questionable beats. Lock happens at `10/12`.

### Important ESP32 Detail

PulseSensorPlayground's detector expects 10-bit analog samples, `0..1023`, with idle near `512`. ESP32 defaults to 12-bit analog reads, `0..4095`, so this firmware calls:

```cpp
analogReadResolution(10);
```

That keeps the library's threshold and beat-detection math in the range it expects.

## Low-Level Hardware And Build Notes

The sketch is intentionally kept as one Arduino `.ino` file so beginners can open, read, copy, and modify it in the Arduino IDE.

- **Board:** ESP32-2432S028R CYD
- **Display:** ILI9341 320x240 TFT
- **Sensor:** PulseSensor signal on `GPIO 35`
- **RGB LED:** onboard CYD LED, active-low PWM
- **Speaker:** `GPIO 26`
- **Touch:** XPT2046 controller on HSPI

```text
PULSE_PIN       = 35
BACKLIGHT       = 21
LED_RED_PIN     = 4
LED_GREEN_PIN   = 16
LED_BLUE_PIN    = 17
SPEAKER_PIN     = 26
TOUCH_IRQ       = 36
TOUCH_MISO      = 39
TOUCH_MOSI      = 32
TOUCH_SCLK      = 25
TOUCH_CS        = 33
```

### Why GPIO 35

The sensor pin was found with a small analog pin scanner on the connected CYD. On this unit, the PulseSensor signal was visible on `IO35`, not the originally documented `GPIO 36`.

### Screenshot Renderer

Regenerate the mock screenshots after UI changes with:

```bash
node scripts/render-dashboard-screenshots.mjs
```

## Download The Source Code Instead

If you want to inspect or modify the code instead of flashing a device:

### The Firmware Is One `.ino` File

The Arduino sketch is intentionally kept in one file:

```text
CYD_App_Launcher.ino
```

A learner can download that file, put it inside a folder named `CYD_App_Launcher`, and open it in Arduino IDE.

What the one-file sketch still needs:

- Arduino IDE 2.x
- ESP32 board package
- `TFT_eSPI`
- `PulseSensor Playground`
- `XPT2046_Touchscreen`
- CYD display setup through `TFT_eSPI` configuration or equivalent build flags

Copy-all checklist:

```text
1. Download CYD_App_Launcher.ino.
2. Make a folder named CYD_App_Launcher.
3. Put CYD_App_Launcher.ino inside that folder.
4. Open CYD_App_Launcher.ino in Arduino IDE.
5. Install the ESP32 board package.
6. Install TFT_eSPI, PulseSensor Playground, and XPT2046_Touchscreen.
7. Configure TFT_eSPI for the CYD display.
8. Select an ESP32 board and upload at 115200.
```

### Clone The Whole Repo

For the full project with docs, scripts, screenshots, and flasher files:

```bash
git clone https://github.com/yury-g/CYD_App_Launcher.git
cd CYD_App_Launcher
```

You can also use GitHub's **Code** button and choose **Download ZIP**.

## Release

The first known-good single-screen hardware release is tagged:

```text
v1.0.0
```
