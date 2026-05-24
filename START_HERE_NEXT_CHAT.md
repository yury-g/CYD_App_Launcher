# Start Here Next Chat

Last updated: 2026-05-24 EDT

This file is the first breadcrumb for continuing internal PulseSensor CYD dashboard development in a new Codex chat with no previous chat history.

## Fresh Handoff: 2026-05-24 13:52 EDT

This top section supersedes older branch/path references below. Older notes are preserved as history.

Current local repo path:

```text
/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/CYD_App_Launcher_restore-main-app-shell
```

Current branch:

```text
codex/app4-pin-scanner-perf-safe-20260524
```

Current firmware/UI code commit:

```text
1460be0 Enlarge landscape Settings fallback text 20260524-140900-EDT
```

Latest signal-behavior log commit:

```text
8f341cd Record lock hold serial sanity 20260524-135300-EDT
```

Current firmware version on the attached CYD:

```text
0.4.18-settings-text
```

Connected CYD used for the latest flash:

```text
Port: /dev/cu.usbserial-3120
Detected MAC during latest upload: f4:2d:c9:9d:af:cc
Board: ESP32-D0WD-V3 / ESP32-2432S028 CYD
```

Latest verified commands from this pause point:

```sh
python3 tools/check_app_shell.py
git diff --check
PATH=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.venv-pio/bin:$PATH PLATFORMIO_CORE_DIR=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.platformio pio run -e cyd
PATH=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.venv-pio/bin:$PATH PLATFORMIO_CORE_DIR=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.platformio pio run -e cyd -t upload --upload-port /dev/cu.usbserial-3120
python3 tools/render_settings_mock.py
python3 tools/render_monochrome_mock.py
```

Build memory from PlatformIO:

```text
RAM:   7.3% (23780 / 327680 bytes)
Flash: 28.8% (377781 / 1310720 bytes)
```

Rollback anchors before lock-hold work:

```text
Branch: backup/pre-lock-hold-grace-20260524
Tag:    backup/pre-lock-hold-grace-20260524
Commit: 460dbac
```

Current app order:

```text
Pulse dashboard -> Settings -> App 4 Pin Scanner -> Your App Here -> Origin Story
```

Visual/UI status:

- User confirmed the current UI looks great and is visually ready to publish back to `main`.
- Settings plain data rows now use left-justified labels and right-justified values.
- Settings rows with controls keep two-line label/value text to avoid overlap.
- Settings font was bumped back to size 2 with 40px rows.
- Settings long values that would have fallen back to tiny text now use normal size-2 two-line label/value rows in horizontal rotation; tiny Settings value text remains available only for vertical rotation.
- Four display modes are present: `M DARK`, `M LIGHT`, `C DARK`, and `C LIGHT`.
- App 4 Pin Scanner is manual, starts idle, and only scans the tapped row for GPIO35, GPIO22, GPIO21, or GPIO27.
- Signal-performance pass found and fixed avoidable foreground stalls: the live graph no longer redraws the full frame on waveform wrap, BPM/IBI/SIG panels redraw independently, and opt-in `PERF_DIAGNOSTICS` serial timing remains available in firmware but is off by default.
- Tap-to-reacquire is available on the Pulse dashboard below the navigation/header for the case where a learner sees a good waveform but BPM/IBI/qualified-beat detection is stuck in false negatives.
- `SIG GPIO35` bars now show a 12-step acquisition ladder before lock, and the acquisition harmony uses an 8-note rising palette.
- The live waveform and `SIG GPIO35` panel now share the same state colors: yellow while acquiring, then the locked signal color after signal lock.
- Lock retention now follows "acquire strictly, hold gently": four consecutive qualified beats are still required for acquisition, but an already-locked signal can survive up to two unqualified beat events inside a 2200 ms window. BPM/IBI still update only on qualified beats.
- Expanded serial telemetry now includes live range, clipping score, qualified streak, unqualified streak, and lock-drop reason.
- Two 60-second earlobe serial sanity windows after flashing `0.4.17-lock-hold-grace` showed the lock-hold grace path active and bounded. Window 1 was 66.1% locked with `badStreak` max 2; window 2 was 94.2% locked with `badStreak` max 2. Details live in `docs/signal-behavior-log.md`.
- Signal-first development guidance now lives in `docs/signal-first-architecture.md`.
- Signal behavior lessons and upgrade/downgrade notes now live in `docs/signal-behavior-log.md`.

Pre-main blocker:

- Do not merge to `main` yet.
- Signal-performance code checks and earlobe serial sanity passes have been done, but the user should still do real body-position-specific visual sanity passes before any main merge/public release.
- Next chat should start with hardware sanity on the real CYD and record sensor body position: finger if usable, earlobe if finger remains unreliable. Check raw trace responsiveness, BPM, IBI, qualified-beat lock, app switching, Settings, Pin Scanner idle/active behavior, and Origin Story exit behavior.
- Keep PulseSensor performance first-class. Drawing, display modes, App 4, and Origin Story are secondary to fast raw `SIG GPIO35`, BPM, IBI, and qualified-beat math.

## Current App Shell Branch

Use this branch for the current app shell work:

```text
codex/monochrome-ui-treatment-20260524
```

This branch was created from `codex/settings-app-shell-20260523` at `fa4a5a0` for local display-mode UI treatment experiments. Do not merge to main yet.

Current local working path from the 2026-05-24 continuation:

```text
/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/CYD_App_Launcher_current-clean
```

The branch adds an app shell around the Pulse dashboard:

- App 1 is the Pulse dashboard.
- App 2 is an animated placeholder; App 3 is the `Origin Story` crawl with programmatic fanfare.
- Settings includes Volume, Rotation, Display, WiFi/Bluetooth placeholders, LED Control, color swatches, About, Version, and Firmware date.
- Volume now lives in Settings only.
- Rotation now lives in Settings only; the persistent top rotate button was removed.
- App navigation remains persistent as a three-button previous/next/Settings nav bar, with mode-aware outlines/fills and no top-bar rotate control.
- Display mode cycles through `M DARK`, `M LIGHT`, `C DARK`, and `C LIGHT`.
- Firmware version is `0.4.7-origin-perspective`.
- Firmware date shown in Settings is `2026-05-24`.
- Guard script: `python3 tools/check_app_shell.py`.

2026-05-24 display-mode note: render-only design review files live in `docs/screenshots/display-mode-render/review-20260524-display-modes-v2/` and `docs/screenshots/monochrome-render/`. The approved direction before local hardware test: Settings row labels and values use distinct colors in color modes, `C LIGHT` is a high-contrast color-light treatment, the app nav bar is the compact three-button previous/next/Settings treatment, and the fat heart sits in the header gap between `PulseSensor.com` and the app nav.

2026-05-24 08:56:35 EDT hardware/UI follow-up: startup now defaults to the black-background `C DARK` color display mode instead of `M DARK`. `C LIGHT` inactive app navigation and Settings button cells now use a dark blue high-contrast fill with white text so button backgrounds remain distinct from the white app background. Tracking guard `tools/check_app_shell.py` now asserts both preferences.

2026-05-24 09:06:39 EDT App 3 audio experiment: App 3 now starts an original programmatic "origin crawl" fanfare on entry, using the CYD speaker with a short title sting and looping arpeggio. The tone sequence is generated with `ledcWriteTone()`, respects Settings volume, stops when leaving App 3, and is tracked by `tools/check_app_shell.py`. This is a hardware listening pass, not yet a hardware-approved tag.

2026-05-24 09:15:15 EDT App 3 firmware pass: App 3 no longer renders the bouncing `your app here too` placeholder. It now renders a black starfield and scrolling PulseSensor origin crawl directly in firmware, including the feature-request / firmware-update ask and the since-2012 thank-you. The App 3 fanfare table was extended to a tracked 15-second loop. This was built and flashed for hardware review.

2026-05-24 09:17:17 EDT naming pass: App 3's visible title is now `Origin Story` on-device and in the mockup renderer, keeping the science-fiction crawl theme while making the app name clearer.

2026-05-24 09:22:21 EDT Origin Story hardware pass: the crawl text was too small and direct full-area redraws caused heavy flicker on the CYD. Firmware now uses 2x crawl text, reflowed shorter lines, and an offscreen `TFT_eSprite` for the scrolling content area before pushing each frame to the display. Guard script tracks the enlarged text and sprite rendering.

2026-05-24 09:25:59 EDT Origin Story blank-screen fix: user reported Origin Story became a black screen. Firmware now uses an 8-bit crawl sprite to reduce memory, starts the first large text line inside the visible area immediately, and includes a direct large-text fallback renderer if sprite allocation fails. This version was built and flashed for hardware review.

2026-05-24 09:30:51 EDT Origin Story provenance pass: added Pulse Sensor Amped OSHWA certification details (`US000075`, certified August 30, 2017), the `WorldFamousElectronics/PulseSensor_Amped_Arduino` GitHub repo, and a dated repo-love line showing `249` stars and `207` forks as of May 24, 2026. Firmware version is now `0.4.6-origin-oshw`.

2026-05-24 09:34:48 EDT Origin Story perspective-speed pass: crawl speed was doubled and the renderer now fades/shrinks text toward a horizontal vanishing line at the top of the content area. This build was compiled and flashed to the connected CYD for review.

2026-05-24 hardware note: the app-shell firmware was built and flashed to `/dev/cu.usbserial-3120` on the connected ESP32-D0WD-V3 CYD, MAC `f4:65:0b:a9:f2:e8`. Touch ergonomics fixes changed compact toolbar routing to split adjacent app-nav/rotate hit targets at their midpoints, enlarged the visible app-nav and rotate controls from 22x22 to 44x28, converted Settings to a scrollable large-text row list with bigger row-local touch controls, widened Settings bottom scroll buttons to split the full bottom bar, changed Settings rows to alternating yellow/green Pulse dashboard backgrounds with black text, and redesigned App 1 metric tiles to use the same yellow/green high-contrast language.

## Use This Repo And Branch

Internal development repo:

```text
yury-g/CYD_App_Launcher
```

Active branch:

```text
codex/monochrome-ui-treatment-20260524
```

GitHub draft PR for this continuation:

```text
https://github.com/yury-g/CYD_App_Launcher/pull/1
```

Local working path used on the Mac:

```text
/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/CYD_App_Launcher_current-clean
```

Read this file first, then read `CODEX_HANDOFF.md`.

## Current Best Working State

The CYD was flashed and confirmed working on 2026-05-22 through this code state:

```text
1679ed3 Tune dashboard feedback and portrait layout 20260522-132601-EDT
```

This commit is newer than `11f95a8` and is the latest hardware-tested working source state from the 2026-05-22 CYD session. The older tag named `last-working-20260519-114323-EDT` is still useful as a checkpoint, but the best development base is now the current branch head after the 2026-05-22 feedback/portrait wrap-up.

The user reported the dashboard worked well. The rotate-control builds were flashed and verified on the connected CYD. Serial monitor showed the full orientation cycle earlier in the day:

```text
screenRotation=0
screenRotation=3
screenRotation=2
screenRotation=1
```

The latest feedback/portrait builds were also flashed and exercised on the connected CYD. The user confirmed:

- Rear LED default blue glow is gone.
- Rear LED heartbeat effect is accurate and live.
- Rear LED now has a more dramatic envelope: short peak hold plus gradual fade.
- Rear LED pulses yellow while locking and red once locked.
- SIG quality bars and SIG panel outline use high-visibility yellow while locking and green once locked.
- Muted gray labels were replaced with white for real-CYD readability.
- Portrait layout now uses almost the full vertical screen.
- Portrait BPM/IBI numbers were enlarged.

## Important Repo Map

There are two remotes in the working checkout:

```text
launcher git@github.com:yury-g/CYD_App_Launcher.git
origin   git@github.com:WorldFamousElectronics/PulseSensor_CYD.git
```

Use `launcher` for internal development and experiment history. Do not push active experiments to `origin` unless the user explicitly approves a public release.

The public/customer-facing repo line is useful for release reference, but the hardware-tested working branch is on `launcher`.

Important naming note for future chats: even if a local Git remote is named `origin` and points at `WorldFamousElectronics/PulseSensor_CYD`, do not treat that public repo as the development origin/source of truth. For this project, `yury-g/CYD_App_Launcher` is the development home and memory brain. `WorldFamousElectronics/PulseSensor_CYD` is publish-only for polished customer/user releases, and must not receive pushes unless the user explicitly approves a public release.

Repository metadata note: the `yury-g/CYD_App_Launcher` GitHub description should say `ESP32-2432S028` without a trailing `R`. The connected/tested CYD is documented as `ESP32-2432S028`, and the old `ESP32-2432S028R` label should not be reintroduced unless future hardware testing proves it.

## Hardware-Tested Notes

Connected CYD port used successfully on 2026-05-22:

```text
/dev/cu.usbserial-110
```

Known previous port from earlier sessions:

```text
/dev/cu.usbserial-210
```

Always detect the current port before flashing:

```sh
arduino-cli board list
ls -1 /dev/cu.* /dev/tty.*
```

The current firmware expects:

```text
PulseSensor signal -> GPIO35
Speaker            -> GPIO26
Touch controller   -> XPT2046 on HSPI
Display            -> ILI9341 320x240
```

Raw pin scanning found `GPIO35` and `GPIO27` as the best two PulseSensor signal candidates. Avoid `GPIO22` for the dashboard: a GPIO22 dashboard experiment caused a screen on/off reset loop and was restored back to GPIO35.

## Build And Flash

Use the existing PlatformIO install:

```text
/Users/narwhal2/Library/Python/3.9/bin/pio
```

Build:

```sh
cd /Users/narwhal2/Documents/Codex-CYD/PulseSensor_CYD
/Users/narwhal2/Library/Python/3.9/bin/pio run -e cyd
```

Flash, replacing the port if needed:

```sh
cd /Users/narwhal2/Documents/Codex-CYD/PulseSensor_CYD
/Users/narwhal2/Library/Python/3.9/bin/pio run -e cyd -t upload --upload-port /dev/cu.usbserial-110
```

Serial monitor:

```sh
/Users/narwhal2/Library/Python/3.9/bin/pio device monitor --port /dev/cu.usbserial-110 -b 115200
```

The most recent successful monitor output showed the firmware alive:

```text
signal=0 amp=100 bpm=0 ibi=0 locked=0 quality=0
```

## Known Good Behavior To Preserve

- Default speaker volume is `1`.
- Header says `PulseSensor.com`.
- Header brand text is white.
- Top-right corner has a rotate icon button, with volume controls immediately to its left.
- Rotate cycles through `1 -> 0 -> 3 -> 2 -> 1`.
- Rotations `0` and `2` use a dedicated portrait layout.
- Main graph has a cyan dotted threshold line plus `THR 550`.
- Persistent labels and grid lines are bright enough for the real CYD display.
- Bottom-right signal box is minimal: `SIG GPIO35` plus quality bars.
- Signal acquisition still uses blue for the waveform/searching state.
- SIG quality feedback uses yellow while locking and green once locked.
- Rear LED pulses yellow while locking and red once locked, with a dramatic smooth fade.
- Sound has a rising signal-quality harmony while locking and the normal beat chime after lock.
- Lock false positives are reduced by requiring four consecutive qualified beats, healthy live range, and low recent clipping.
- The red heart has a cyan outline.

The default volume clue lives in `PulseSensor_CYD.ino`:

```cpp
#define VOLUME_START 1
```

## History Map

Release-style `launcher/main` line:

```text
8060bb9 2026-05-12 Save one-screen CYD pulse dashboard
5af30a6 2026-05-12 v1.0.0 known-good hardware release
0bcea12 2026-05-14 Add heartbeat chime and header controls, VOLUME_START 3
537311d 2026-05-14 Polish beat dashboard feedback, VOLUME_START 2
9bae844 2026-05-14 Add Signal Coach teaching feedback, VOLUME_START 1
c22aa0b 2026-05-14 v1.2.0 / launcher/main
```

Internal hardware-tested branch:

```text
3aad11d 2026-05-19 tag last-working-20260519-114323-EDT
3ac3fcd 2026-05-19 stricter false-positive tuning
896af96 2026-05-19 simplified SIG GPIO35 quality box
b64b02a 2026-05-19 white PulseSensor.com header
723edc0 2026-05-19 GPIO22 experiment; caused reset loop
9066535 2026-05-19 restored GPIO35 after GPIO22 reset loop
107b2a4 2026-05-19 current best base; documents GPIO35/GPIO27 finding
109dc67 2026-05-22 added next-chat handoff note
49f0ce3 2026-05-22 added hardware-tested two-way rotate control
66647af 2026-05-22 added four-orientation portrait/landscape layout
58be524 2026-05-22 changed rotate button to a graphic icon
11f95a8 2026-05-22 improved palette contrast and blue/green signal state
214e0f7 2026-05-22 refreshed README screenshots and archived older screenshot design history
ef9a5bd 2026-05-22 updated handoff for rotation/palette screenshot wrap-up
1679ed3 2026-05-22 tuned LED feedback, SIG yellow/green quality colors, portrait layout, white labels, and beat-dot explainer docs
```

## Screenshot Notes

Current horizontal README screenshots live in:

```text
docs/screenshots/searching.svg
docs/screenshots/locked.svg
```

Beat-dot explanation visual added during the 2026-05-22 feedback session:

```text
docs/screenshots/beat-dot-explainer.svg
```

Older screenshots were retired to:

```text
docs/screenshots/history/20260522-before-rotation-palette/
```

## Next Chat Prompt

Paste this into a fresh Codex chat:

```text
Continue internal development of yury-g/CYD_App_Launcher in branch codex/app4-pin-scanner-perf-safe-20260524.

Use local path if available:
/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/CYD_App_Launcher_restore-main-app-shell

Start by reading START_HERE_NEXT_CHAT.md, CODEX_HANDOFF.md, README.md, and CHANGELOG.md. Treat yury-g/CYD_App_Launcher as the internal development home. Do not push to WorldFamousElectronics/PulseSensor_CYD unless I explicitly approve a public release.

Current firmware/UI code commit is a5ec516, firmware version 0.4.12-settings-row-alignment. The current UI looked great on the attached CYD and is visually ready to publish back to main, but do not merge yet. I suspect signal speed / raw SIG GPIO35 responsiveness / BPM / IBI / qualified-beat analysis may have degraded as the app shell, display modes, Settings, and App 4 were added.

First task: investigate signal-performance regression before any main merge. Compare history around 1679ed3, 4691ef1, f664c94, 4b40549, 6ab9693, and a5ec516. Keep PulseSensor performance first-class: fast raw signal, BPM, IBI, and qualified beat math matter more than drawings, app switching, Pin Scanner, display modes, or Origin Story.

Useful checks: confirm readPulseSensor() is still first in loop(), measure loop/draw timing and serial sample cadence, check whether full-screen redraws/app switching/settings/pin scanner code can block PulseSensor Playground updates, and consider reverting or gating any display work that compromises sensing.

Connected CYD for hardware tests:
Port /dev/cu.usbserial-3120
Latest detected upload MAC f4:2d:c9:9d:af:cc

Before UI changes, run visual/check tools:
python3 tools/render_pulse_app_mock.py
python3 tools/render_settings_mock.py
python3 tools/render_monochrome_mock.py
python3 tools/render_display_mode_mock.py
python3 tools/render_app4_pin_scanner_mock.py
python3 tools/render_app3_origin_crawl_mock.py
python3 tools/check_app_shell.py

Build:
PATH=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.venv-pio/bin:$PATH PLATFORMIO_CORE_DIR=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.platformio pio run -e cyd

Flash:
PATH=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.venv-pio/bin:$PATH PLATFORMIO_CORE_DIR=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.platformio pio run -e cyd -t upload --upload-port /dev/cu.usbserial-3120

When the signal path is verified or fixed, make a backup tag/branch for current main, then merge/publish this branch to main only after hardware sanity checks pass.
```
