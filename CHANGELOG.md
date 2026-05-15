# CYD App Launcher — Student Version — Changelog

## Unreleased
- Linked the `Why GPIO 35` note to the standalone `CYD_Analog_Pin_Scanner` diagnostic repo.

## v1.2.0 — 2026-05-15
**Signal Coach teaching aid and quieter startup volume**
- Added Signal Coach states that turn PulseSensorPlayground readings into plain-language feedback: `TOO FLAT`, `HOLD STEADY`, `GOOD WAVE`, `LOCKING`, and `QUALIFIED BEAT`.
- Added a visible pulse amplitude meter and dotted threshold guide tied to `getPulseAmplitude()`, `isInsideBeat()`, and `setThreshold(550)`.
- Updated the README with future teaching-aid wishlist ideas and a clear Avoid For Now section.
- Regenerated dashboard screenshots for the Signal Coach design.
- Lowered the default speaker volume from `2/10` to `1/10`.

## v1.1.0 — 2026-05-14
**Beat feedback, browser flasher prototype, and student-friendly docs**
- Added a qualified-beat chime on the CYD speaker at `GPIO 26`.
- Added touch volume controls in the header with a lower default volume of `2/10`.
- Added a centered animated heart whose outline follows the live trace color.
- Changed the waveform to cyan while searching and white when beats are qualified.
- Replaced timer-driven full dashboard repaints with change-driven redraws to reduce flicker.
- Added generated dashboard screenshots and a script to re-render them from the current design.
- Added an ESP Web Tools browser flasher prototype with matching firmware binary build script.
- Reworked the README into a flash-first, student-friendly guide with wiring, features, troubleshooting, and source-download paths.
- Called out that the firmware is a single Arduino `.ino` sketch for users who want to open it directly in Arduino IDE.
- Updated flashing instructions to require a freshly detected serial port and verified the current build on connected CYD boards.

## v1.0.0 — 2026-05-13
**Known-good one-screen CYD Pulse Dashboard**
- Reduced firmware to one beginner-friendly Arduino `.ino` file.
- Replaced the five-app launcher with a single live PulseSensor dashboard.
- Added live line graph, BPM, IBI, 12-step signal quality meter, and qualified beat status.
- Changed PulseSensor input to `GPIO 35`, matching the connected CYD hardware found with the analog pin scanner.
- Added red rear RGB LED blink/fade on qualified beats only.
- Added automatic detector re-arm when the waveform looks alive but PulseSensorPlayground is not emitting beat events.
- Matched PulseSensorPlayground's ESP32 expectations with `analogReadResolution(10)`.
- Added `flash-cyd.sh` helper for repeatable Arduino CLI builds with the required `TFT_eSPI` CYD compile flags.
- Verified on connected ESP32-2432S028R CYD hardware.

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
