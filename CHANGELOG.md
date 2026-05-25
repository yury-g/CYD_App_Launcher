# PulseSensor CyberDeck with the CYD Changelog

This file is the short public history. Detailed signal notes live in
`docs/signal-behavior-log.md`; broader UI and hardware development notes live in
`docs/experiment-log.md`; external audit history lives in `docs/code-audits/`.

## Unreleased

- Renamed the current product story to **PulseSensor CyberDeck with the CYD** so
  the README matches the app-shell firmware on `main`.
- Added external code-audit notes plus the current response/recommendation under
  `docs/code-audits/`.
- Replaced the blocking detector re-arm `delay(8)` with a non-blocking
  pause/resume state.
- Added an on-screen PulseSensor initialization failure message instead of only
  logging the failure to serial.
- Added small source comments for the PulseSensor interrupt setup, the
  signal-first loop rule, and signal-range decay behavior.
- Flashed commit `aebb23a` to the connected CYD on `/dev/cu.usbserial-10` to
  confirm the audit-fix pass still uploads, hard-resets, and streams release
  telemetry on hardware.

## 0.4.41-snappy-lock - 2026-05-24

- Current hardware-tested source firmware on `main`.
- Keeps the single-file Arduino sketch while adding the CyberDeck app shell:
  Pulse dashboard, Settings, Pin Scanner, `Your App Here`, and Origin Story.
- Keeps PulseSensor reading first in `loop()` and preserves the current signal
  math, lock grace, clipping honesty, and shipped peak-to-peak recovery behavior.
- Adds Settings controls for volume, display mode, rotation, LED behavior,
  diagnostics, version/date, memory, and build labels.
- Adds manual guarded Pin Scanner rows for GPIO35, GPIO22, GPIO21, and GPIO27.
- Adds Origin Story crawl text synced from `docs/origin-story-crawl.txt`.
- Adds source metadata helpers and `tools/check_project.py` as the default guard.

## v1.2.0 - 2026-05-15

- Added Signal Coach teaching feedback: `TOO FLAT`, `HOLD STEADY`, `GOOD WAVE`,
  `LOCKING`, and `QUALIFIED BEAT`.
- Added the amplitude meter and dotted `THR 550` guide.
- Lowered startup volume to `1/10`.

## v1.1.0 - 2026-05-14

- Added beat sound on the CYD speaker and touch volume controls.
- Added the centered animated heart and state-colored waveform behavior.
- Added the first ESP Web Tools browser flasher prototype.
- Reworked the README into a flash-first, student-friendly guide.

## v1.0.0 - 2026-05-13

- Established the known-good one-screen CYD Pulse dashboard.
- Switched PulseSensor input to `GPIO 35` for the tested CYD hardware.
- Added BPM, IBI, waveform, signal quality bars, rear LED pulse, and automatic
  detector re-arm.

## v0.2.0 - 2026-04-08

- Built the first complete five-app launcher sketch.
- Added touch menu navigation and early Heartbeat, Breathing, Relaxation, HRV,
  and BreathFFT apps.
- Status at that point was incomplete and not yet hardware-verified.

## v0.1.0 - 2026-04-08

- Started the repository with a placeholder README and initial Git history.
