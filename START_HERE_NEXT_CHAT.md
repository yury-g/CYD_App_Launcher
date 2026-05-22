# Start Here Next Chat

Last updated: 2026-05-22 EDT

This file is the first breadcrumb for continuing internal PulseSensor CYD dashboard development in a new Codex chat with no previous chat history.

## Use This Repo And Branch

Internal development repo:

```text
yury-g/CYD_App_Launcher
```

Active branch:

```text
codex/finger-coach-dashboard-20260519-111641-EDT
```

Local working path used on the Mac:

```text
/Users/narwhal2/Documents/Codex-CYD/PulseSensor_CYD
```

Read this file first, then read `CODEX_HANDOFF.md`.

## Current Best Working State

The CYD was flashed and confirmed working on 2026-05-22 through this code state:

```text
11f95a8 Improve dashboard palette contrast 20260522
```

That commit is newer than the tag named `last-working-20260519-114323-EDT`. The `last-working` tag is still useful as a checkpoint, but the best development base is the current branch head after the 2026-05-22 wrap-up.

The user reported the dashboard worked well. The rotate-control builds were flashed and verified on the connected CYD. Serial monitor showed the full orientation cycle:

```text
screenRotation=0
screenRotation=3
screenRotation=2
screenRotation=1
```

The latest palette build was also flashed and exercised through acquisition and locked states.

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
- Signal acquisition uses blue; locked/acquired signal uses green.
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
```

## Screenshot Notes

Current horizontal README screenshots live in:

```text
docs/screenshots/searching.svg
docs/screenshots/locked.svg
```

Older screenshots were retired to:

```text
docs/screenshots/history/20260522-before-rotation-palette/
```

## Next Chat Prompt

Paste this into a fresh Codex chat:

```text
We are continuing internal development of the PulseSensor CYD dashboard.

Use repo yury-g/CYD_App_Launcher, branch codex/finger-coach-dashboard-20260519-111641-EDT.

Start by reading START_HERE_NEXT_CHAT.md and CODEX_HANDOFF.md. Treat yury-g/CYD_App_Launcher as the internal development home. Do not push experiments to WorldFamousElectronics/PulseSensor_CYD unless I explicitly approve a public release.

The current best hardware-tested code state is 11f95a8, with screenshot/docs updates after it. It was flashed successfully on 2026-05-22 to /dev/cu.usbserial-110. Preserve default volume 1, GPIO35 PulseSensor input, the top-right rotate icon button, four-orientation rotation cycle, portrait layout, blue acquisition state, green locked state, brighter persistent labels/grid lines, the minimal SIG GPIO35 quality panel, stricter false-positive tuning, and the one-screen dashboard.

Make small changes, build with PlatformIO, flash to the CYD when asked, then preserve hardware-tested states with timestamped commits/tags on launcher.
```
