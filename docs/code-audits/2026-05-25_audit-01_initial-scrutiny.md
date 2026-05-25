# Code Audit 01 - Initial Scrutiny

Date: 2026-05-25  
Reviewer: Claude (Opus 4.7), at Yury's request  
Snapshot reviewed: older public `main` around the `v1.0.0` era  
Scope: repository identity, docs, classroom fit, firmware structure, hardware assumptions

## TL;DR As Received

The audit said the repo identity was broken: the repo name and description
suggested a five-app launcher, while the inspected code looked like a
single-screen PulseSensor dashboard. It also found that the PulseSensor
integration was strong, but onboarding and public positioning were thin.

## Main Findings

- **Identity mismatch was critical.** The public name/description implied a
  launcher, while the inspected README/code presented a one-screen dashboard.
- **PulseSensor integration was the strongest area.** The audit called out
  correct 10-bit ADC scaling, GPIO35 choice, ESP32 Core 3.x LEDC API usage,
  beat qualification, lock quality, and detector re-arm logic.
- **Classroom onboarding needed work.** Missing or unclear items included
  board/library setup, CYD display setup, supported variants, wiring docs,
  PlatformIO config, troubleshooting, and a visible browser-flash path.
- **Public history was confusing.** The changelog mixed shipped releases,
  abandoned branches, and local experiments in a way that made the current
  state hard to follow.
- **The firmware was useful but had small reliability issues.** The audit
  specifically called out `delay(8)` in detector re-arm, weak handling of
  `pulseSensor.begin()` failure, lack of a signal-first banner, and subtle
  signal-range decay logic.

## Firmware Findings Worth Tracking

| ID | Original Concern | Current Disposition |
| --- | --- | --- |
| C1 | `delay(100)` after `Serial.begin()` | Low priority. |
| C2 | `pulseSensor.begin()` result not surfaced on screen | Fixed after response: init failure now shows on device. |
| C3 | `delay(8)` inside `rearmPulseDetector()` | Fixed after response: re-arm resume is non-blocking. |
| C4 | `updateSignalRange()` min/max decay can look inverted | Addressed with clarifying comment; current guard recenters narrow ranges. |
| C5 | Consider `volatile` for shared globals | Deferred; do not blanket-apply without a targeted concurrency review. |
| C6 | `drawCenteredText()` assumes 6 px font cells | Later polish. |
| C7 | Full graph-frame redraw on waveform wrap | Mostly stale; current waveform uses column background redraw. |
| C8 | Green/blue LED channels | Low priority. |
| C9 | Threshold constants not runtime-tunable | Later classroom tuning feature. |
| C10 | `USE_ARDUINO_INTERRUPTS` needs comment | Fixed after response. |
| C11 | Signal-first loop ordering should be obvious | Fixed after response. |
| C12 | Backlight PWM | Later polish. |

## Recommended Then

The audit recommended resolving identity first, adding display setup docs,
adding a browser flasher link, adding supported-board notes and a license,
fixing the re-arm delay, surfacing init failures, adding PlatformIO/CI, and
eventually adding student extension docs.

## Caveat

This audit reviewed an older public snapshot. Several findings became stale
after the app-shell source, PlatformIO config, docs, tools, license, and release
hygiene checks were pushed to `main`.
