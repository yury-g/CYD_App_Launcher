# Start Here Next Chat

Last updated: 2026-05-24 EDT

This file is the first breadcrumb for continuing internal PulseSensor CYD dashboard development in a new Codex chat with no previous chat history.

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
- App 2 and App 3 are animated placeholders.
- Settings includes Volume, Rotation, Display, WiFi/Bluetooth placeholders, LED Control, color swatches, About, Version, and Firmware date.
- Volume now lives in Settings only.
- Rotation now lives in Settings only; the persistent top rotate button was removed.
- App navigation remains persistent as a three-button previous/next/Settings nav bar, with mode-aware outlines/fills and no top-bar rotate control.
- Display mode cycles through `M DARK`, `M LIGHT`, `C DARK`, and `C LIGHT`.
- Firmware version is `0.4.4-origin-readable`.
- Firmware date shown in Settings is `2026-05-24`.
- Guard script: `python3 tools/check_app_shell.py`.

2026-05-24 display-mode note: render-only design review files live in `docs/screenshots/display-mode-render/review-20260524-display-modes-v2/` and `docs/screenshots/monochrome-render/`. The approved direction before local hardware test: Settings row labels and values use distinct colors in color modes, `C LIGHT` is a high-contrast color-light treatment, the app nav bar is the compact three-button previous/next/Settings treatment, and the fat heart sits in the header gap between `PulseSensor.com` and the app nav.

2026-05-24 08:56:35 EDT hardware/UI follow-up: startup now defaults to the black-background `C DARK` color display mode instead of `M DARK`. `C LIGHT` inactive app navigation and Settings button cells now use a dark blue high-contrast fill with white text so button backgrounds remain distinct from the white app background. Tracking guard `tools/check_app_shell.py` now asserts both preferences.

2026-05-24 09:06:39 EDT App 3 audio experiment: App 3 now starts an original programmatic "origin crawl" fanfare on entry, using the CYD speaker with a short title sting and looping arpeggio. The tone sequence is generated with `ledcWriteTone()`, respects Settings volume, stops when leaving App 3, and is tracked by `tools/check_app_shell.py`. This is a hardware listening pass, not yet a hardware-approved tag.

2026-05-24 09:15:15 EDT App 3 firmware pass: App 3 no longer renders the bouncing `your app here too` placeholder. It now renders a black starfield and scrolling PulseSensor origin crawl directly in firmware, including the feature-request / firmware-update ask and the since-2012 thank-you. The App 3 fanfare table was extended to a tracked 15-second loop. This was built and flashed for hardware review.

2026-05-24 09:17:17 EDT naming pass: App 3's visible title is now `Origin Story` on-device and in the mockup renderer, keeping the science-fiction crawl theme while making the app name clearer.

2026-05-24 09:22:21 EDT Origin Story hardware pass: the crawl text was too small and direct full-area redraws caused heavy flicker on the CYD. Firmware now uses 2x crawl text, reflowed shorter lines, and an offscreen `TFT_eSprite` for the scrolling content area before pushing each frame to the display. Guard script tracks the enlarged text and sprite rendering.

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
We are continuing internal development of the CYD app shell branch.

Use repo yury-g/CYD_App_Launcher, branch codex/settings-app-shell-20260523.

Start by reading START_HERE_NEXT_CHAT.md and CODEX_HANDOFF.md. Treat yury-g/CYD_App_Launcher as the internal development home. Do not push experiments to WorldFamousElectronics/PulseSensor_CYD unless I explicitly approve a public release.

The current app-shell code state started from 7654183 and was continued on 2026-05-24. It was flashed successfully to /dev/cu.usbserial-3120, MAC f4:65:0b:a9:f2:e8. Preserve default volume 1, GPIO35 PulseSensor input, the persistent rotate icon button, four-orientation rotation cycle, Settings-only volume, app navigation attached to rotate, app placeholder pages, Settings firmware date 2026-05-24, midpoint-split touch targets for the compact toolbar, enlarged 44x28 app-nav/rotate controls, the scrollable large-text Settings list, full-width split Settings scroll buttons, yellow/green staggered Settings row backgrounds, and yellow/green App 1 metric tiles with black text.

Make small changes, run python3 tools/check_app_shell.py, build with PlatformIO, flash to the CYD when asked, then preserve hardware-tested states with timestamped commits/tags on the internal repo.
```
