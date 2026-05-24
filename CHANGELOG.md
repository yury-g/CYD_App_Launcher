# CYD App Launcher — Student Version — Changelog

## Unreleased
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
