# CYD App Launcher — Student Version — Changelog

## Unreleased
- 2026-05-24 clipping quality guard: fixed the saturated/noisy-signal regression where rail-to-rail ADC input could still show near-full acquisition bars, peak-to-peak score, detector re-arm messages, and acquisition harmony even though beat math correctly rejected it; clipped or motion-artifact input now forces acquisition quality and peak-to-peak score to zero, blocks detector re-arm, and shows `ADJUST SENSOR`; firmware version is `0.4.23-clip-guard`.
- 2026-05-24 signal-core polish pass: preserved and pushed the known-good `0.4.21-signal-log` state to the internal repo with rollback branch/tag `backup/good-working-0.4.21-signal-log-20260524` and `good-working-0.4.21-signal-log-20260524`, kept the source as one beginner-friendly `.ino`, added release/diagnostic PlatformIO modes (`cyd` quiet by default, `cyd_diag` raw CSV logging), removed dead top-bar volume/rotate paths, shared app-frame/header and Settings row helpers, grouped beat acceptance in `BeatDecision` without changing thresholds, added Origin Story sprite release on exit/rotation, and regenerated release binaries; firmware version is `0.4.22-core-polish`.
- 2026-05-24 signal-log diagnostics pass: added 50 Hz raw CSV serial diagnostics plus local capture/analyze/check tools, confirmed an independent raw peak detector can match the firmware's beat stream on the same earlobe position, changed clipping decay from foreground-loop based to time based, rejected huge rolling-range motion artifacts, and added pre-lock cadence consistency so acquisition no longer accepts short/double movement beats; firmware version is `0.4.21-signal-log`.
- 2026-05-24 peak-to-peak experiment pass: added an enabled `PEAK_TO_PEAK_EXPERIMENT` switch that scores live peak-to-peak waveform movement, lets high-score peak-to-peak candidate beats help first acquisition or continue acquisition after an initial strict beat, accepts peak-to-peak candidates with a wider cadence window during locked earlobe runs while rejecting short movement-blip intervals and preserving plausible timing/clipping guardrails, and reports serial `p2p` plus `accept=peak2peak`; firmware version is `0.4.20-peak2peak`.
- 2026-05-24 peak/cadence recovery pass: kept first acquisition strict, but added locked-only cadence guarding and peak/cadence acceptance for slight movement cases where the valley/amplitude math distorts while beat timing remains plausible; serial telemetry now reports `accept=strict`, `accept=peak-cadence`, or `accept=reject` for versioned hardware notes; firmware version is `0.4.19-peak-cadence`.
- 2026-05-24 Settings landscape small-text pass: kept the tiny Settings value fallback only for vertical/portrait display rotation, and changed horizontal Settings rows that would have shrunk to tiny text into larger two-line label/value rows; firmware version is `0.4.18-settings-text`.
- 2026-05-24 lock-retention grace pass: added rollback branch/tag `backup/pre-lock-hold-grace-20260524`, kept strict four-qualified-beat acquisition and existing beat qualification thresholds, but made already-locked signal state tolerate up to two unqualified beat events within a 2200 ms grace window before dropping lock; expanded serial telemetry with live range, clipping score, qualified/unqualified streaks, and drop reason for long-run hardware sanity checks; firmware version is `0.4.17-lock-hold-grace`.
- 2026-05-24 matched waveform/SIG color pass: changed the live waveform to use the same acquisition/lock color function as the `SIG GPIO35` panel, so the graph line is yellow while acquiring and switches to the same locked color as the SIG box after signal lock; added a guard check to keep future UI work from reintroducing a separate waveform acquisition palette; firmware version is `0.4.16-matched-sig-wave`.
- 2026-05-24 signal-acquisition ladder pass: changed the compact `SIG GPIO35` bars from four coarse `0/3/6/9/12` jumps into a continuous 12-step acquisition score based on live range, amplitude, clipping cleanliness, detector beat-window state, and qualified-beat streak, while keeping BPM/IBI lock strict; expanded the acquisition harmony to an 8-note, 4-step rising phrase; firmware version is `0.4.15-acquisition-ladder`.
- 2026-05-24 tap-to-reacquire signal pass: added a Pulse dashboard touch gesture below the navigation/header that manually re-arms PulseSensor Playground, clears local signal acquisition state, redraws the dashboard, and gives learners a direct recovery path when the raw waveform looks good but BPM/IBI/qualified-beat detection is stuck in false negatives; added `docs/signal-first-architecture.md` and guard checks for the signal-first architecture; firmware version is `0.4.14-tap-reacquire`.
- 2026-05-24 signal-performance safety pass: confirmed `readPulseSensor()` remains first in `loop()`, added opt-in serial perf diagnostics for loop/read/draw timing, removed the periodic full graph-frame redraw from the live waveform wrap path, split BPM/IBI/SIG panel redraws so qualified beats no longer redraw all panels unnecessarily, and bumped firmware version to `0.4.13-signal-perf-safe`.
- 2026-05-24 app-shell handoff before main publish: documented the current UI-ready branch state, added a next-chat signal-performance investigation blocker before any `main` merge, and called out that raw `SIG GPIO35`, BPM, IBI, and qualified-beat analysis stay higher priority than app switching, display modes, Pin Scanner, and Origin Story.
- 2026-05-24 Settings row-alignment pass: bumped Settings row text back up one size, restored 40px rows, changed plain data rows to left-label/right-value alignment with automatic smaller right-aligned values for long strings, kept two-line text only for rows with right-side controls, and bumped firmware version to `0.4.12-settings-row-alignment`.
- 2026-05-24 Settings build-memory layout pass: added a Build row showing PlatformIO RAM and Flash usage, changed plain Settings rows to single-line `Label: value` text that uses the full row, kept two-line text only on rows with right-side controls, and bumped firmware version to `0.4.11-settings-build-memory`.
- 2026-05-24 App 4 performance-safe pin scanner pass: added a manual Pin Scanner screen after Settings with only GPIO35, GPIO22, GPIO21, and GPIO27, left scanning idle until one row is tapped, kept `readPulseSensor()` first in `loop()`, kept the ADC on the PulseSensor 10-bit scale, added a Settings memory row with used/free size and free percentage, switched the rotation control to an icon-only button, and reduced Settings row text to fit more rows; firmware version is `0.4.10-perf-safe-pin-scanner`.
- 2026-05-24 monochrome SIG quality-bar polish: removed inactive SIG GPIO quality-bar background segments from monochrome modes in firmware and matching render mockups while leaving color-mode inactive bar backgrounds intact; firmware version is `0.4.9-mono-quality-bars`.
- 2026-05-24 app-switcher waveform-priority pass: stopped live Pulse dashboard status changes from redrawing the full graph frame, added a small coach-status redraw instead, and guarded the app-shell check so `readPulseSensor()` remains first in `loop()`; firmware version is `0.4.8-waveform-priority`.
- 2026-05-24 app-shell pause documentation: expanded the README into a readable major-change log, added the current branch/head as the good app-shell pause point, included Origin Story render previews, and documented a non-Codex PlatformIO source-flash path for another computer.
- 2026-05-24 09:34:48 EDT Origin Story hardware trial: doubled the crawl speed, added a horizontal vanishing line, and made crawl rows shrink to 1x text and fade into the black background as they approach the top horizon; firmware version is `0.4.7-origin-perspective`.
- 2026-05-24 09:30:51 EDT Origin Story provenance pass: added OSHWA certification details for Pulse Sensor Amped (`US000075`, certified August 30, 2017), the `WorldFamousElectronics/PulseSensor_Amped_Arduino` GitHub repo, and current repo-love stats (`249` stars, `207` forks as of May 24, 2026); firmware version is `0.4.6-origin-oshw`.
- 2026-05-24 09:25:59 EDT Origin Story blank-screen fix: changed the crawl sprite to lower-memory 8-bit color, made the first crawl line visible immediately, and added a direct large-text fallback renderer if sprite allocation fails; firmware version is `0.4.5-origin-visible`.
- 2026-05-24 09:22:21 EDT Origin Story hardware pass: enlarged the scrolling crawl text to 2x, reflowed the copy into CYD-width lines, moved the animated crawl into an offscreen `TFT_eSprite` to reduce direct clear/redraw flicker, and bumped firmware version to `0.4.4-origin-readable`.
- 2026-05-24 09:17:17 EDT App 3 naming pass: changed the on-device and mockup title from `APP 3 ORIGIN CRAWL` to `Origin Story` to fit the science-fiction app theme.
- 2026-05-24 09:15:15 EDT App 3 origin-crawl firmware pass: replaced App 3's bouncing placeholder on-device with a black starfield and scrolling PulseSensor origin crawl, extended the original CYD speaker fanfare to a tracked 15-second loop, and bumped firmware version to `0.4.3-app3-crawl`.
- 2026-05-24 09:08:58 EDT App 3 origin-crawl mockup: added a closing ask for feature requests, firmware update ideas, and classroom wishes, plus a thank-you for supporting PulseSensor since 2012.
- 2026-05-24 09:06:39 EDT App 3 origin-crawl branch: added an original programmatic CYD speaker fanfare for App 3, with a short heroic title sting and looping digital arpeggio scaled by Settings volume, and bumped firmware version to `0.4.2-app3-fanfare` for hardware listening.
- 2026-05-24 08:56:35 EDT monochrome UI branch: changed the startup default to the black-background `C DARK` display mode, bumped firmware version to `0.4.1-color-dark-start`, and gave `C LIGHT` inactive navigation/button cells a dark blue high-contrast fill with white text so they no longer disappear into the white app background.
- 2026-05-24 monochrome UI branch: added firmware display modes `M DARK`, `M LIGHT`, `C DARK`, and `C LIGHT` under Settings `Display`, moved rotation to Settings-only, removed the top-bar rotate button, and bumped firmware version to `0.4.0-display-modes` for local hardware color-scheme testing.
- 2026-05-24 monochrome UI branch: updated the persistent app nav bar to the three-button previous/next/Settings layout, with mode-aware monochrome/color outlines and no top-bar rotate control, leaving more header room for the centered fat heart.
- 2026-05-24 monochrome UI branch: expanded the README with an internal visual design-memory section showing the display-mode contact sheet, Settings picker variants, Pulse searching/locked variants, and true black/white exploration images.
- 2026-05-24 monochrome UI branch: added render-only review tools for black/white and display-mode layouts, including cache-busted full-panel PNGs for Settings/searching/locked mode comparison.
- 2026-05-24 app-shell branch: widened Settings bottom scroll buttons so up/down split the full bottom bar width for easier finger navigation.
- 2026-05-24 app-shell branch: visually rendered and redesigned App 1 Pulse dashboard metric tiles to use the same yellow/green high-contrast language with black text.
- 2026-05-24 app-shell branch: changed Settings rows to alternating Pulse dashboard yellow/green backgrounds with black text for stronger row separation on the CYD.
- 2026-05-24 app-shell branch: enlarged Settings page text, converted Settings to a scrollable row list, and gave Settings controls larger row-local touch targets for easier finger taps.
- 2026-05-24 app-shell branch: enlarged the visible app navigation and persistent rotate controls from 22x22 to 44x28 for easier finger touch on the CYD.
- 2026-05-24 app-shell branch: tuned compact toolbar touch ergonomics so app navigation and persistent rotate controls use midpoint-split hit targets instead of overlapping padded rectangles; Settings color swatches use the same nearest-swatch approach.
- Linked the `Why GPIO 35` note to the standalone `CYD_Analog_Pin_Scanner` diagnostic repo.
- 2026-05-19 11:39 EDT hardware experiment: tried expanded Signal Dashboard / Finger Coach UI with lock-game visuals and coaching sounds on branch `codex/finger-coach-dashboard-20260519-111641-EDT`; rejected for IRL use because the CYD display felt too busy and the active bottom panel flickered / over-drew. Kept only the useful graph threshold affordance: cyan dotted threshold line plus `THR 550` label.
- Restored the working one-screen dashboard layout and added a small signal-quality harmony tied to bottom-right `SIGNAL` quality progress.
- 2026-05-19 11:43 EDT checkpoint: saved tag `last-working-20260519-114323-EDT` before false-positive tuning. Re-applied the useful detector-side lesson from the side quest without restoring the rejected UI: lock now requires four consecutive qualified beats, healthy live range, and low recent clipping.
- 2026-05-19 11:47 EDT: simplified the bottom-right `SIGNAL` box to `SIG GPIO35` plus quality bars only, keeping the signal-quality harmony behavior for progress feedback.

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
- Verified on connected ESP32-2432S028 CYD hardware.

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
