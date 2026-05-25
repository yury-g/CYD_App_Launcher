# Start Here Next Chat

Last updated: 2026-05-25 EDT

This is the short resume note for future AI sessions. Keep the default loop fast:
fetch GitHub, build, flash, and verify on real CYD hardware.

## Source Of Truth

```text
Repo: https://github.com/yury-g/CYD_App_Launcher
Branch: main
Current favorite firmware: 0.4.41-snappy-lock
Continuation note: docs/handoff-2026-05-24-0.4.41-snappy-lock.md
```

Do not remember individual CYD boards, serial ports, MAC addresses, or
computer-specific paths in GitHub. Detect the connected serial port at flash
time and pass it with `--upload-port`.

## Default Dev Loop

1. Fetch `launcher/main`.
2. Confirm `APP_VERSION` is the intended firmware.
3. Build with `python3 -m platformio run -e cyd`.
4. Flash the currently connected CYD with `python3 -m platformio run -e cyd -t upload --upload-port <detected-port>`.
5. Use the actual CYD screen and serial output as the source of truth.

Useful checks:

```sh
python3 tools/check_app_shell.py
python3 tools/check_core_polish.py
python3 tools/check_clipping_quality_guard.py
python3 tools/check_signal_diagnostics.py
python3 tools/check_resume_flash_entrypoint.py
```

Run render/mock tools only when deliberately changing visual design or updating
documentation screenshots. They are not part of the default firmware loop.

## Current Product Priorities

- PulseSensor signal quality, BPM, IBI, and qualified beat behavior outrank app
  shell polish.
- `readPulseSensor()` must remain the first meaningful call in `loop()`.
- Keep diagnostics opt-in: `cyd` is the quiet release build; `cyd_diag` is for
  raw CSV logging.
- If signal behavior looks wrong, check placement, wiring, power/ground, and
  serial evidence before changing beat math.

## Current App Order

```text
Pulse dashboard -> Settings -> Pin Scanner -> Your App Here -> Origin Story
```

## Git Sanity

Check whether local and GitHub differ:

```sh
git fetch launcher
git rev-list --left-right --count HEAD...launcher/main
```

Read the output as:

```text
0 0   same place
N 0   local ahead; inspect then push if desired
0 N   GitHub ahead; pull or fast-forward
N M   diverged; inspect before merging
```
