# Codex Handoff: Internal PulseSensor CYD Development

Last updated: 2026-05-22 EDT

This repository branch is the working memory for internal PulseSensor CYD experiments. A future Codex chat with no prior context should start here.

Quick start for future chats: read `START_HERE_NEXT_CHAT.md` first, then this file.

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
codex/finger-coach-dashboard-20260519-111641-EDT
```

The branch should track:

```text
launcher/codex/finger-coach-dashboard-20260519-111641-EDT
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
git push launcher codex/finger-coach-dashboard-20260519-111641-EDT
git push launcher --tags
```

Avoid this until public release is explicitly approved:

```sh
git push origin codex/finger-coach-dashboard-20260519-111641-EDT
git push origin --tags
```

## Fresh Chat Startup Prompt

Use this in a new chat:

```text
We are continuing internal development of the PulseSensor CYD dashboard from GitHub.

Use repo:
yury-g/CYD_App_Launcher

Use branch:
codex/finger-coach-dashboard-20260519-111641-EDT

Read START_HERE_NEXT_CHAT.md and CODEX_HANDOFF.md first. Treat yury-g/CYD_App_Launcher as the memory brain and active internal dev home. Do not push experiments to WorldFamousElectronics/PulseSensor_CYD unless I explicitly approve a public release.

The current hardware-tested code state is 1679ed3, with handoff/docs wrap-up commits after it. Preserve default volume 1, GPIO35 PulseSensor input, the top-right rotate icon button, four-orientation cycle, improved portrait layout, brighter persistent labels/grid lines, blue/cyan waveform acquisition state, yellow locking SIG/LED feedback, green locked SIG feedback, red locked heartbeat LED fade, the minimal SIG GPIO35 quality panel, stricter false-positive tuning, and the one-screen dashboard.

Make small changes, build with PlatformIO, flash to the CYD when asked, and preserve hardware-tested states with timestamped commits/tags on the internal repo.
```
