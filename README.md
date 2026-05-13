# CYD Pulse Dashboard

One-screen PulseSensor dashboard for the ESP32 CYD / Cheap Yellow Display.

This firmware is tuned for a working CYD hardware setup with:

- Live PulseSensor waveform
- Qualified beat detection
- BPM readout
- IBI readout
- 12-step signal quality meter
- Rear RGB LED red blink/fade on qualified beats
- Auto re-arm when the waveform looks alive but the detector is not producing beat events

The sketch is intentionally kept as one Arduino `.ino` file so beginners can open, read, copy, and modify it in the Arduino IDE.

## Hardware

**Board:** ESP32-2432S028R CYD
**Display:** ILI9341 320x240 TFT
**Sensor:** PulseSensor signal on `GPIO 35`
**RGB LED:** onboard CYD LED, active-low PWM

```text
PULSE_PIN       = 35
BACKLIGHT       = 21
LED_RED_PIN     = 4
LED_GREEN_PIN   = 16
LED_BLUE_PIN    = 17
```

## Why GPIO 35

The sensor pin was found with a small analog pin scanner on the connected CYD. On this unit, the PulseSensor signal was visible on `IO35`, not the originally documented `GPIO 36`.

## Important ESP32 Detail

PulseSensorPlayground's detector expects 10-bit analog samples, `0..1023`, with idle near `512`. ESP32 defaults to 12-bit analog reads, `0..4095`, so this firmware calls:

```cpp
analogReadResolution(10);
```

That keeps the library's threshold and beat-detection math in the range it expects.

## Current UI

The display has one screen:

- `SEARCHING / SIGNAL SEARCH` while the firmware is looking for reliable beats
- `LOCKED / QUALIFIED BEAT` after the quality meter reaches lock
- A live scrolling line graph in the center
- Lower panels for `BPM`, `IBI`, and signal quality
- `R#` in the quality panel shows how many automatic detector re-arms have happened

### Screenshots

These mock screenshots match the current `v1.0.0` firmware layout.

| Searching for signal | Locked on qualified beat |
| --- | --- |
| ![Searching signal screen](docs/screenshots/searching.svg) | ![Locked qualified beat screen](docs/screenshots/locked.svg) |

## Beat Qualification

The firmware uses PulseSensorPlayground for beat detection:

- `getLatestSample()` for the waveform
- `sawStartOfBeat()` for beat events
- `getBeatsPerMinute()` for BPM
- `getInterBeatIntervalMs()` for IBI
- `getPulseAmplitude()` as part of signal qualification

The 12-step quality meter rises on qualified beats and falls gently on questionable beats. Lock happens at `10/12`.

## Flashing

The helper script includes the required `TFT_eSPI` CYD display compile flags.

```bash
./flash-cyd.sh /dev/cu.usbserial-3120
```

If your serial port is different, pass it as the first argument.

## Release

The first known-good single-screen hardware release is tagged:

```text
v1.0.0
```

This software is an educational biofeedback demo and is not intended for medical use.
