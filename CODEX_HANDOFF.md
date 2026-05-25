# Codex Handoff: Internal PulseSensor CYD Development

Last updated: 2026-05-24 EDT

This repository branch is the working memory for internal PulseSensor CYD experiments. A future Codex chat with no prior context should start here.

Quick start for future chats: read `START_HERE_NEXT_CHAT.md` first, then this file.

## Fresh Handoff: 2026-05-24 0.4.41 Snappy Lock On Main

This top section supersedes older branch/path references below. Older notes are
preserved as history and may describe rollback or failed-experiment states.

Current GitHub source of truth:

```text
https://github.com/yury-g/CYD_App_Launcher
Branch: main
Firmware: 0.4.41-snappy-lock
Diagnostic firmware: 0.4.41-snappy-lock-log
Continuation note: docs/handoff-2026-05-24-0.4.41-snappy-lock.md
```

The latest work restored a snappier detection strategy:

- Strict accepted beats now trust the simple qualified-beat path again.
- Auto re-arm retunes PulseSensor Playground's threshold to the current waveform
  midpoint and retries faster.
- Locked state is held through four bad beat events and a 4200 ms gap.
- The graph marker dots expose the distinction between Playground detection and
  app acceptance.
- The heart uses the pre-multi-app shape with the current sprite/no-flicker
  renderer and white outline.

The latest build was flashed to `/dev/cu.usbserial-10`. Serial confirmed the
new re-arm path fired, but the final short bench/contact read was not a stable
live body-lock proof.

## Fresh Handoff: 2026-05-24 Failed Signal-Core Experiment Closure

This section supersedes older branch/path references below. Older notes remain as useful history.

## Latest Stopping Point: 2026-05-24 No More Development Today

The user wants to stop development here and resume from the internal GitHub repo
later.

Next time, point the AI at:

```text
https://github.com/yury-g/CYD_App_Launcher
```

Then ask it to flash the connected CYD with the current favorite build already
on the device:

```text
Firmware: 0.4.24-front-id
Firmware commit: d04c21a Add first screen version identity 20260524
Ref: good-working-0.4.24-front-id-20260524
Branch: main
```

No more firmware/UI development is requested before that first flash/sanity
check.

Latest hardware/user note:

- The apparent signal degradation/no-lock edge case cleared without a code
  change once earlobe placement improved.
- The user reported the current device state was "working like a champ" and
  suspected the earlier issue was user placement.
- Keep `0.4.24-front-id` as the preferred first flash and comparison point.
- Keep the regression research notes for later: if the symptom returns, capture
  serial evidence first and compare placement/wiring/GPIO35/GPIO27 before
  changing beat math.

Current local path:

```text
/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/CYD_App_Launcher_restore-main-app-shell
```

Current branch:

```text
codex/signal-core-polish-publish-prep-20260524
```

Current firmware/UI code commit:

```text
HEAD Add first screen version identity 20260524
```

Latest signal-behavior log commit:

```text
HEAD Add first screen version identity 20260524
```

Current firmware version:

```text
0.4.24-front-id
```

Latest hardware status:

- This branch is closed as a failed experiment and should not merge to `main`.
- It produced useful diagnostics and one honest UI guard, but it did not restore a trustworthy live pulse experience on hardware.
- The weird waveform behavior was real: the attached CYD showed rail-to-rail `SIG GPIO35` input with `signal` hitting `0`/`1023`, `range=1023`, `clip=100`, and unusable BPM/IBI.
- `0.4.23-clip-guard` only made the failure visible and safe: while railed, serial showed `quality=0`, `p2p=0`, `locked=0`, `BPM=0`, `IBI=0`, `rawDiag=0`, and no re-arm spam.
- `0.4.24-front-id` keeps the same signal behavior and adds `APP_VERSION` plus `APP_FIRMWARE_DATE` directly under `PulseSensor.com` on the first Pulse dashboard screen for fast hardware/version tracking.
- Next action is to flash current internal `main` (`0.4.24-front-id`) and
  compare. If it fails again, investigate physical/electrical causes before
  touching beat math.
- Built with PlatformIO in release env `cyd` and diagnostic env `cyd_diag`.
- Release build defaults raw CSV diagnostics off and reports `0.4.24-front-id`; diagnostic build enables 50 Hz `rawDiag` CSV and reports `0.4.24-front-id-log`.
- Upload target remains `/dev/cu.usbserial-3120`; latest known attached CYD MAC is `f4:2d:c9:9d:af:cc`.
- User reported the `0.4.22` waveform looked much worse and filled the screen with bar-like noise. Release serial confirmed the ADC was genuinely saturated/clipped, but exposed a UI/control bug: `quality=10/11`, `p2p=6/7`, and detector re-arm messages could appear while `clip=100`.
- `0.4.23-clip-guard` fixes that: clipped or motion-artifact input forces acquisition quality and peak-to-peak score to `0`, blocks detector re-arm, and shows `ADJUST SENSOR`. A 14 s release serial window after flashing showed `rawDiag=0`, `quality=0`, `p2p=0`, `locked=0`, `BPM=0`, `IBI=0`, and no re-arm messages while the ADC remained railed.
- Flashed `cyd_diag` and captured `logs/signal-log-ear-core-polish-60s-20260524.csv`: 59.3 s, 2997 rows, 8 accepted firmware beat events, 104 rejected, 0 row-level accepted noisy beats, and 1 accepted short IBI of 646 ms near the transition into clipped/noisy behavior.
- The longer diagnostic capture `logs/signal-log-ear-core-polish-180s-20260524.csv` was stopped at the user's request after 122.1 s. It was almost entirely saturated/clipped: 6010 rows, 6008 clip rows, 369 rejected beat events, and 0 accepted beats. This is good safety behavior but not a clean steady-contact accuracy proof.
- Flashed the final `cyd` release build afterward. An 8 s serial check produced 17 normal summary lines and 0 `rawDiag` CSV rows; the signal was still saturated (`signal=1023`, `clip=100`), so BPM/IBI stayed at 0 while unlocked.
- PlatformIO release memory is `RAM 7.3% (23764 / 327680)` and `Flash 28.9% (378349 / 1310720)`. Diagnostic memory is `RAM 7.3% (23764 / 327680)` and `Flash 28.9% (378397 / 1310720)`.
- The cleanest same-earlobe accuracy evidence remains the previous `0.4.21-signal-log` capture: 44.2 s, 47 firmware accepted beats, 47 independent raw peaks, firmware median IBI/BPM `918 ms / 65.3`, independent median IBI/BPM `920 ms / 65.2`, zero clip rows, zero motion/noise windows, and zero accepted short IBIs below 700 ms.
- User confirmed the current UI looks great and is visually ready to publish back to `main`.

Do not merge this branch to `main`. Use it as a diagnostic record and rollback source only.

Performance priority:

- PulseSensor `SIG GPIO35`, BPM, IBI, and qualified-beat math are first-class.
- Display modes, app switching, App 4 Pin Scanner, Origin Story, and visual polish are second-class if they interfere with sensing.
- Preserve `readPulseSensor()` as the first meaningful work in `loop()`.
- If signal behavior still feels off on hardware, re-enable `PERF_DIAGNOSTICS`, compare serial loop/read/draw timing, and keep reducing display work before touching beat math.
- Keep `RAW_SIGNAL_DIAGNOSTICS` as an internal diagnostic switch. Use `cyd` for quiet release-candidate builds and `cyd_diag` for raw CSV logging.
- Disable `PEAK_TO_PEAK_EXPERIMENT` first if the new peak-to-peak behavior creates false positives or unstable BPM/IBI. Roll back the lock-hold experiment with branch/tag `backup/pre-lock-hold-grace-20260524` only if the broader lock-hold behavior feels worse.
- Always record sensor body position and mount/contact method in hardware notes. The `0.4.17-lock-hold-grace` serial sanity windows were earlobe tests because finger contact had stopped giving usable readings even though it worked well the previous day.

Commits to compare:

```text
1679ed3 Tune dashboard feedback and portrait layout 20260522-132601-EDT
4691ef1 Merge app shell preview into main
f664c94 Tune mono SIG bars 20260524-121410-EDT
4b40549 Add perf-safe App 4 scanner 20260524-122839-EDT
6ab9693 Add Settings build memory row 20260524-123742-EDT
a5ec516 Align Settings row values 20260524-125120-EDT
```

Current app order:

```text
Pulse dashboard -> Settings -> App 4 Pin Scanner -> Your App Here -> Origin Story
```

Current UI specifics:

- Four display modes: `M DARK`, `M LIGHT`, `C DARK`, `C LIGHT`.
- Settings-only icon rotation.
- Settings plain rows use left-label/right-value alignment.
- Settings control rows keep a second value line to avoid control overlap.
- Settings long values that would have fallen back to tiny text now use normal size-2 two-line label/value rows in horizontal rotation; tiny Settings value text remains available only for vertical rotation.
- Settings includes runtime memory and build memory rows.
- App 4 Pin Scanner starts idle, scans only one tapped row, and is limited to GPIO35, GPIO22, GPIO21, and GPIO27.
- `Your App Here` stays second-to-last.
- `Origin Story` stays last.
- Signal-performance pass: `readPulseSensor()` still starts `loop()`, PulseSensor Playground remains on its ESP32 500 Hz timer interrupt, the live graph wrap no longer redraws the whole graph frame, BPM/IBI/SIG panels redraw independently, and `PERF_DIAGNOSTICS` is available but disabled by default.
- Tap-to-reacquire is available on the Pulse dashboard below the navigation/header for the case where a learner sees a good waveform but BPM/IBI/qualified-beat detection is stuck in false negatives.
- `SIG GPIO35` bars now show a 12-step acquisition ladder before lock, and the acquisition harmony uses an 8-note rising palette.
- The live waveform and `SIG GPIO35` panel now share the same state colors: yellow while acquiring, then the locked signal color after signal lock.
- Lock retention now follows "acquire strictly, hold gently": four consecutive qualified beats are still required for acquisition, but an already-locked signal can survive up to two rejected beat events inside a 2200 ms window.
- Locked beat detection now adds a peak/cadence recovery path for the user-observed earlobe case where slight movement distorts the valley while peaks remain visually stable. After lock, both strict and recovered beats must stay close to the current cadence before updating BPM/IBI.
- `0.4.20-peak2peak` adds an enabled peak-to-peak experiment. It scores live peak-to-peak waveform movement, can let high-score peak-to-peak candidate beats help acquisition, and can accept bounded peak-to-peak candidates while locked. The tuned window uses a wider cadence tolerance than strict recovery but rejects short movement-blip intervals below 70% of the current IBI.
- Expanded serial telemetry now includes live range, clipping score, qualified streak, unqualified streak, `p2p` score, beat accept reason, and lock-drop reason.
- `0.4.23-clip-guard` fixes the bad-waveform/false-progress regression: clipped rail noise no longer fills acquisition quality bars, no longer raises peak-to-peak score, no longer re-arms the detector, and shows `ADJUST SENSOR` while preserving BPM/IBI rejection.
- `0.4.22-core-polish` preserves the single-file Arduino source, removes dead top-bar volume/rotate paths, shares app-frame/header and Settings row helpers, groups beat acceptance in `BeatDecision` without changing thresholds, releases the Origin Story sprite on app exit/rotation, and adds `tools/check_core_polish.py`.
- `0.4.21-signal-log` adds raw CSV diagnostics, `tools/capture_signal_log.py`, `tools/analyze_signal_log.py`, `tools/check_signal_diagnostics.py`, time-based clipping-score decay, a rolling-range motion-artifact guard, and pre-lock cadence consistency. It directly addresses the same-earlobe smooth/noisy investigation: clipping and huge rolling range caused some degraded readings, and clean raw signal could still produce short/double acquisition beats until cadence was checked before lock.
- Serial sanity after flashing `0.4.20-peak2peak` on the same earlobe position confirmed `accept=peak2peak` events during locked runs with clipping at 0. The tuned path looked bounded in serial, but later weak/short-interval sections still dropped lock, so this remains an experiment candidate rather than a publish verdict.
- Serial sanity after flashing `0.4.19-peak-cadence` confirmed `accept=peak-cadence` events during locked runs with clipping at 0. One later window still showed a `grace expired` drop after two rejected events, so this is an upgrade candidate, not a final publish verdict.
- Two 60-second earlobe serial sanity windows after flashing `0.4.17-lock-hold-grace` showed the lock-hold grace path active and bounded. Window 1 was 66.1% locked with `badStreak` max 2; window 2 was 94.2% locked with `badStreak` max 2. Details live in `docs/signal-behavior-log.md`.
- Signal-first development guidance now lives in `docs/signal-first-architecture.md`.
- Signal behavior lessons and upgrade/downgrade notes now live in `docs/signal-behavior-log.md`.

## Current App Shell Status

Active app-shell branch:

```text
codex/monochrome-ui-treatment-20260524
```

GitHub draft PR for this continuation:

```text
https://github.com/yury-g/CYD_App_Launcher/pull/1
```

This branch was created from `codex/settings-app-shell-20260523` at `fa4a5a0` for display-mode UI treatment experiments. Do not merge to main yet.

Current app-shell branch head before the 2026-05-24 touch ergonomics continuation:

```text
7654183 Match app navigation button size to rotate
```

The current branch wraps the one-screen Pulse dashboard in a small app shell:

- App 1: Pulse dashboard.
- App 2: bouncing `your app here`.
- App 3: `Origin Story` crawl with programmatic fanfare.
- Settings: Volume, Rotation, Display, WiFi placeholder, Bluetooth placeholder, LED Control, LED color swatches, About, Version, and Firmware date.
- Volume is Settings-only and no longer appears in the top toolbar.
- Rotation is Settings-only and no longer appears as a persistent top-toolbar button.
- App navigation remains persistent as a three-button previous/next/Settings nav bar, with mode-aware outlines/fills and no top-toolbar rotate button.
- Firmware version for this branch: `0.4.7-origin-perspective`.
- Firmware date shown in Settings: `2026-05-24`.
- Guard script: `python3 tools/check_app_shell.py`.

2026-05-24 display-mode continuation: added Settings `Display` cycling through `M DARK`, `M LIGHT`, `C DARK`, and `C LIGHT`. `M DARK`/`M LIGHT` are true monochrome treatments. `C DARK` preserves the main dark color family, and `C LIGHT` is a high-contrast light color treatment with blue values/search trace, amber searching accents, cyan/teal locked outlines, and green locked SIG bars. The top nav bar was also changed to the compact three-button previous/next/Settings layout, with rotation moved fully into Settings and the freed header space used for the centered fat heart. Render review files live in `docs/screenshots/display-mode-render/review-20260524-display-modes-v2/`; monochrome black/white review files live in `docs/screenshots/monochrome-render/`.

2026-05-24 08:56:35 EDT display-mode hardware follow-up: changed startup default to the black-background `C DARK` mode, bumped Settings version text to `0.4.1-color-dark-start`, and gave `C LIGHT` inactive app-nav/Settings button cells a dark blue fill with white text so the cells contrast against the white app background. `tools/check_app_shell.py` now tracks the default mode and light-mode nav-fill requirements.

2026-05-24 09:06:39 EDT App 3 audio experiment: added an original programmatic CYD speaker fanfare for App 3's PulseSensor origin crawl direction. The sequence is intentionally not a Star Wars melody: it uses a short low-to-high title sting and a looping digital arpeggio, generated by `ledcWriteTone()`, scaled by Settings volume, and stopped when leaving App 3. `tools/check_app_shell.py` now tracks the App 3 fanfare hooks.

2026-05-24 09:15:15 EDT App 3 firmware pass: replaced the on-device App 3 bouncing placeholder with the firmware-rendered PulseSensor origin crawl over a black starfield. The crawl includes the feature-request / firmware-update ask and thanks supporters since 2012. The App 3 fanfare was expanded from a short loop to a tracked 15-second programmatic loop. Guard script now asserts the real App 3 crawl renderer and 15-second loop tracking.

2026-05-24 09:17:17 EDT naming pass: App 3 is now visibly titled `Origin Story` on-device and in the mockup renderer, preserving the science-fiction crawl direction while making the app identity clearer.

2026-05-24 09:22:21 EDT Origin Story hardware pass: user reported the scrolling text was much too small and the page flickered heavily. The crawl now uses 2x text with shorter CYD-width lines and renders the animated content area into a `TFT_eSprite`, then pushes the composed sprite to the display to avoid direct clear/redraw flicker. This is flashed for hardware review.

2026-05-24 09:25:59 EDT Origin Story blank-screen fix: user reported the Origin Story page became totally black. Likely causes were the first large line starting below the visible content area and/or full-size sprite allocation failure. Firmware now uses an 8-bit sprite, starts text visibly on the first frame, and falls back to direct large-text rendering if sprite allocation fails.

2026-05-24 09:30:51 EDT Origin Story provenance pass: the crawl now includes Pulse Sensor Amped OSHWA registration `US000075`, certification date August 30, 2017, the `WorldFamousElectronics/PulseSensor_Amped_Arduino` GitHub repo, and a dated repo-love note showing `249` stars and `207` forks as of May 24, 2026.

2026-05-24 09:34:48 EDT Origin Story perspective-speed pass: the crawl speed is doubled with `APP3_CRAWL_SPEED_MS 52`, and crawl lines now use a perspective renderer that fades and shrinks them toward a horizontal origin line. This build was compiled and flashed to the connected CYD for review.

2026-05-24 continuation: the compact toolbar hit testing was tuned for real CYD touch ergonomics. App nav and rotate still look grouped, but their padded tap regions are now split at the midpoint between adjacent button centers, preventing the previous overlap where taps near rotate could be claimed by Settings or taps near nav boundaries could choose the earlier button. The same midpoint approach is used for the Settings color swatches.

Follow-up hardware pass: the visible app-nav and rotate controls were enlarged from 22x22 to 44x28 for easier large-finger touch. Landscape moves the heart left to avoid the wider toolbar; portrait uses a taller header so the toolbar, title, coach text, and heart do not collide.

Second follow-up hardware pass: Settings page rows now use larger `SETTINGS_TEXT_SIZE 2` text and scroll through a row list. The volume, rotation, LED, color swatch, and scroll controls use larger row-local touch targets, so each Settings control should be easier to hit without repeated attempts.

Visual pass: rendered Settings mocks before flashing and changed the row treatment to alternating Pulse dashboard yellow and green backgrounds with black text. This replaces the weak gray separators and should make row boundaries obvious on the real CYD.

App 1 visual pass: added `tools/render_pulse_app_mock.py` and rendered the Pulse dashboard before changing firmware. The bottom BPM, IBI, and SIG panels now use the same high-contrast yellow/green tile language with black text, while the live graph remains dark for waveform contrast.

Settings scroll touch pass: bottom Settings scroll buttons now split the full bottom bar width. The up button starts at the left edge and the down button fills the right half, leaving only the normal app-button gap between them.

Connected CYD used for this continuation:

```text
Port: /dev/cu.usbserial-3120
MAC:  f4:65:0b:a9:f2:e8
Board: ESP32-D0WD-V3 / ESP32-2432S028 CYD
```

## GitHub Repositories

- Internal development home: `yury-g/CYD_App_Launcher`
- Public/customer-facing repo: `WorldFamousElectronics/PulseSensor_CYD`
- Do not push active experiments to `WorldFamousElectronics/PulseSensor_CYD` until the user explicitly says the work is ready for public release.

Important naming note: `yury-g/CYD_App_Launcher` is the development origin/source of truth for this ongoing work. `WorldFamousElectronics/PulseSensor_CYD` is the public publishing destination for polished customer/user releases only. If a local checkout has a remote named `origin` pointing at `WorldFamousElectronics/PulseSensor_CYD`, do not use that remote for development pushes unless the user explicitly approves a public release.

Repository metadata note: the GitHub description for `yury-g/CYD_App_Launcher` was corrected on 2026-05-22 to say `ESP32-2432S028` without a trailing `R`. Keep the board label as `ESP32-2432S028` in repo metadata, README, and code comments unless future hardware testing proves otherwise.

Local remotes used in the original workspace:

```text
launcher git@github.com:yury-g/CYD_App_Launcher.git
origin   git@github.com:WorldFamousElectronics/PulseSensor_CYD.git
```

The active dev branch is:

```text
codex/settings-app-shell-20260523
```

The branch should track:

```text
origin/codex/settings-app-shell-20260523
```

## Current Known State

### Hardware Finding: Dual PulseSensor Pins

2026-05-19 13:04:51 EDT: on the connected ESP32-2432S028 CYD, raw connector scanning showed `GPIO35` and `GPIO27` as the best two PulseSensor signal candidates. `GPIO22` also showed usable signal in the raw scanner, but an earlier dashboard build using `GPIO22` as the PulseSensor input caused a screen on/off reset loop, so prefer `GPIO35` and `GPIO27` for the first two-sensor dashboard experiment.

Planned two-sensor mapping:

```text
PulseSensor A signal -> GPIO35
PulseSensor B signal -> GPIO27
Power for sensors    -> 3.3V
Ground for sensors   -> GND
```

Current working HEAD before this handoff note:

```text
1679ed3 Tune dashboard feedback and portrait layout 20260522-132601-EDT
```

2026-05-22 EDT: the latest code state was built and flashed successfully to the connected CYD on `/dev/cu.usbserial-110`. The serial monitor showed live firmware output and earlier confirmed the full orientation cycle with `screenRotation=0`, `screenRotation=3`, `screenRotation=2`, and `screenRotation=1`. The feedback/portrait builds were exercised on hardware with user-visible confirmation.

Latest user-confirmed hardware behavior:

- Rear RGB LED is off by default; the previous unwanted blue default glow is gone.
- Rear LED pulses yellow while locking and red after lock.
- Rear LED pulse is beat-accurate but more dramatic: short peak hold and gradual fade.
- SIG panel outline and bars use high-visibility yellow while locking and green when locked.
- Gray/dim labels were replaced with white for CYD readability.
- Portrait layout uses the lower screen area, with taller graph/panels and larger BPM/IBI numbers.
- Beat-dot explainer visual lives at `docs/screenshots/beat-dot-explainer.svg`.

Important local/GitHub tags on the internal repo:

```text
last-working-20260519-114323-EDT
false-positive-tune-20260519-114323-EDT
signal-box-minimal-20260519-114712-EDT
header-white-20260519-120416-EDT
restore-gpio35-after-gpio22-loop-20260519-124815-EDT
dual-pulse-pin-finding-20260519-130451-EDT
rotate-control-20260522-121644-EDT
last-working-20260522-132601-EDT
```

Important: the tag named `last-working-20260519-114323-EDT` is a useful older checkpoint, but the best current development base is the current branch head after the 2026-05-22 feedback/portrait wrap-up. The latest hardware-flashed code commit before handoff/docs cleanup is `1679ed3`.

These refs were intentionally removed from the public repo after being accidentally pushed there:

```text
codex/finger-coach-dashboard-20260519-111641-EDT
codex/light-blue-screen-redraw-20260519-105951-EDT
last-working-20260519-114323-EDT
false-positive-tune-20260519-114323-EDT
signal-box-minimal-20260519-114712-EDT
header-white-20260519-120416-EDT
```

Keep development and experiment history on `yury-g/CYD_App_Launcher` for now.

## Product Direction

Keep the original one-screen PulseSensor CYD dashboard.

Do not bring back the rejected expanded Signal Dashboard / Finger Coach UI. It looked too busy on the real CYD and caused flicker/overdrawing.

What is currently kept:

- Header says `PulseSensor.com`.
- Header brand text is white.
- Top-right corner has a rotate icon button, with volume controls immediately to its left.
- The rotate button cycles through `1 -> 0 -> 3 -> 2 -> 1`.
- Rotations `0` and `2` use a dedicated portrait layout.
- Main graph has a cyan dotted threshold line plus `THR 550`.
- Bottom-right signal box is minimal: `SIG GPIO35` plus quality bars only.
- Persistent labels and grid lines use a brighter palette for real-CYD legibility.
- Waveform/searching acquisition is blue/cyan.
- SIG quality feedback is yellow while locking and green once locked.
- Rear LED is off by default, pulses yellow while locking, and pulses red after lock.
- Rear LED uses a more dramatic beat-accurate envelope: short peak hold and smooth gradual fade.
- Sound has a rising signal-quality harmony while locking and the normal beat chime after lock.
- Default speaker volume starts at `1` via `#define VOLUME_START 1`.
- Portrait layout uses nearly the full vertical screen and has larger BPM/IBI numbers.
- Formerly muted/dim labels are white for CYD readability.
- Lock false positives were reduced by requiring:
  - 4 consecutive qualified beats
  - healthy live signal range
  - low recent clipping
- The red heart has a cyan outline.

## Hardware-Tested Workflow

User preference: make small changes, build, flash, let the user test IRL, then save timestamped commits/tags for hardware-tested states.

PlatformIO path:

```text
/Users/narwhal2/Library/Python/3.9/bin/pio
```

Build:

```sh
cd /Users/narwhal2/Documents/Codex-CYD/PulseSensor_CYD
/Users/narwhal2/Library/Python/3.9/bin/pio run -e cyd
```

Flash:

```sh
cd /Users/narwhal2/Documents/Codex-CYD/PulseSensor_CYD
/Users/narwhal2/Library/Python/3.9/bin/pio run -e cyd -t upload
```

Connected board port used:

```text
/dev/cu.usbserial-110
```

Earlier sessions used `/dev/cu.usbserial-210`; always detect the current port before flashing.

## Screenshot History

The current README screenshots are regenerated horizontal SVG recreations of the latest palette and rotate-icon UI:

```text
docs/screenshots/searching.svg
docs/screenshots/locked.svg
```

Beat-dot explanation visual:

```text
docs/screenshots/beat-dot-explainer.svg
```

The older pre-rotation/pre-palette screenshots were retired for design-version history:

```text
docs/screenshots/history/20260522-before-rotation-palette/
```

## GitHub Auth

GitHub CLI was logged in as `yury-g` during the source session:

```sh
gh auth status
```

Expected:

```text
Logged in to github.com account yury-g
Token scopes include: gist, read:org, repo
```

SSH auth also worked:

```sh
ssh -T git@github.com
```

Expected:

```text
Hi yury-g! You've successfully authenticated, but GitHub does not provide shell access.
```

## Safe Push Commands

For internal dev pushes:

```sh
git push origin codex/settings-app-shell-20260523
git push origin --tags
```

No public/customer-facing remote is configured in the current 2026-05-24 app-shell clone. Add one only when a public release is explicitly approved:

```sh
git remote add public git@github.com:WorldFamousElectronics/PulseSensor_CYD.git
```

## Fresh Chat Startup Prompt

Use this in a new chat:

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
