# Codex Handoff: PulseSensor CYD

Last updated: 2026-05-25 EDT

This file is intentionally short. The repo no longer needs a brainstorm-heavy
AI memory trail for every development cycle.

## Current Baseline

```text
Repo: https://github.com/yury-g/CYD_App_Launcher
Branch: main
Favorite firmware: 0.4.41-snappy-lock
Diagnostic firmware: 0.4.41-snappy-lock-log
Detailed historical handoff: docs/handoff-2026-05-24-0.4.41-snappy-lock.md
```

Use GitHub as source of truth. Use local Git for speed after fetching.

Do not record board-specific identities in GitHub. Treat CYDs and computers as
interchangeable: detect the current serial port each time and flash that port.

## Working Rules

- Build and test against real CYD hardware whenever firmware behavior matters.
- Keep `readPulseSensor()` first in `loop()`.
- Prefer small firmware changes with a quick build/flash loop.
- Preserve signal behavior before visual polish.
- Use `cyd` for quiet release-style builds.
- Use `cyd_diag` only when raw serial diagnostics are needed.
- Render/mock scripts are optional reference tools for deliberate visual or
  screenshot work.

## Fast Commands

```sh
git fetch launcher
git rev-list --left-right --count HEAD...launcher/main
python3 -m platformio run -e cyd
python3 -m platformio run -e cyd -t upload --upload-port <detected-port>
```

Checks that protect current behavior:

```sh
python3 tools/check_app_shell.py
python3 tools/check_core_polish.py
python3 tools/check_clipping_quality_guard.py
python3 tools/check_signal_diagnostics.py
python3 tools/check_resume_flash_entrypoint.py
python3 tools/check_project.py
```

## What Matters Most

- Real signal quality on `GPIO35`.
- Trustworthy BPM and IBI.
- Clear acquisition vs locked feedback.
- No app, animation, scanner, sound, or drawing path should interfere with
  PulseSensor sampling.

Historical experiment notes remain in `docs/` and Git history, but future work
should not start by replaying all of them.
