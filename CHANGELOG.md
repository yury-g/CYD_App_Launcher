# CYD App Launcher — Student Version — Changelog

## v0.2.0 — 2026-04-08
**Complete .ino written (compilation pending)**
- Full CYD_App_Launcher.ino written (531 lines)
- 5 apps implemented: Heartbeat, Breathing, Relaxation, HRV, BreathFFT
- Code improvements from PulseSensor_CYD repo integrated:
  - RGB LED support (GPIO 4/16/17) with ledcAttach API (ESP32 Core 3.x)
  - Auto-scaling waveform with updateMinMax() decay pattern
  - No-finger timeout (3000ms no beat → reset BPM/IBI)
  - sawStartOfBeat() for one-shot beat events
  - Thick 4px waveform + cursor line ahead
  - Heart icon function (2 circles + triangle)
- Touch menu navigation (5 app buttons in top bar)
- Status: INCOMPLETE — compilation error to debug, .bin not yet generated

## v0.1.0 — 2026-04-08
**Repo initialized**
- README placeholder
- git initialized
- Status: INCOMPLETE — .ino not yet written

## Upcoming: v1.0.0
- Compilation errors resolved
- Compiled .bin verified on hardware
- Multi-part manifest.json created
- Web flasher live at yury-g.github.io/CYD_App_Launcher
