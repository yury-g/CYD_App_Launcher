# Signal Behavior Log

This log tracks signal-reading lessons that should survive future firmware and
UI work. It is intentionally practical: what changed, how it behaved, and
whether it made real CYD pulse sensing better or worse.

## Current Best Rule

Acquire strictly, hold gently.

- Strict acquisition prevents false positives: keep four consecutive qualified
  beats, healthy live range, minimum amplitude, BPM/IBI bounds, and clipping
  rejection.
- Gentle post-lock hold prevents false negatives: once locked, tolerate up to
  two unqualified beat events within 2200 ms before clearing BPM/IBI.
- BPM and IBI are updated by strict qualified beats. Once locked, they may also
  update through the narrow `peak-cadence` recovery path when timing remains
  plausible and the beat-event/range still looks alive. Grace-held rejected
  beats preserve the last trusted BPM/IBI but do not create new readings.
- Peak-to-peak scoring is useful, but it must be bounded by clipping, rolling
  range, and cadence. The 2026-05-24 same-earlobe logs showed that raw peak
  shape can be excellent while detector acquisition still accepts short double
  beats unless pre-lock cadence consistency is enforced.
- Raw `SIG GPIO35`, BPM, IBI, and qualified-beat behavior outrank display polish,
  app switching, Pin Scanner, sounds, and story screens.
- Always record body position and mounting method for hardware validation. Do
  not compare finger, earlobe, wrist, or loose bench readings as if they are the
  same test condition.

## 2026-05-24 - Stopping Point And Placement Clarification

Firmware on device: `0.4.24-front-id`

Commit/ref: `d04c21a` / `good-working-0.4.24-front-id-20260524`

User-visible result:

- After reporting a possible long-run regression where the waveform became
  noisy/jagged and the dashboard could not hold qualified beats, the user later
  reported the same device state was "working like a champ" with better earlobe
  placement.
- No firmware was changed between the regression report and the good behavior.
- Treat this as evidence that body placement/contact can mimic a signal-core
  regression. Do not invent new beat math until the symptom is reproduced with a
  logged stable-contact test.

Research findings from the audit:

- The movement-resilience path was not lost. Current signal-core history still
  includes lock-hold grace (`0.4.17`), locked peak/cadence recovery (`0.4.19`),
  peak-to-peak scoring (`0.4.20`), pre-lock cadence consistency and raw logging
  (`0.4.21`), and clipping/motion-artifact honesty (`0.4.23`).
- `0.4.24-front-id` is a tracking/front-screen identity change only; it does not
  change sampling, clipping guards, acquisition thresholds, BPM/IBI
  qualification, or diagnostic fields.
- `0.4.25-stable-wave` is a drawing-only change: fixed ADC viewport for the
  live trace and no beat circles over the raw waveform. It does not change beat
  qualification.
- A short non-mutating serial watch during the questionable state showed clean
  non-railed input (`clip=0`) with `range` roughly `80..129`, `p2p` roughly
  `3..7`, and occasional `accept=strict`, but `qStreak` did not hold long enough
  for lock. That pattern fits inconsistent contact/cadence more than the old
  rail-to-rail wiring failure.

Next diagnostic if the symptom returns:

- First flash/check internal `main` at `0.4.24-front-id`.
- Record body position and contact method before judging behavior.
- Capture serial or `rawDiag` and compare `range`, `clip`, `p2p`, `qStreak`,
  `badStreak`, `accept`, and `drop`.
- If contact/wiring is uncertain, use App 4 Pin Scanner to compare GPIO35
  against GPIO27. Keep GPIO21/GPIO22 guarded; they are not the dashboard analog
  input candidates.

## 2026-05-24 - Failed Experiment Closure

Branch: `codex/signal-core-polish-publish-prep-20260524`

Verdict:

- Treat this branch as a failed experiment and do not merge it to `main`.
- The work produced useful tools and lessons: raw CSV diagnostics, an offline
  analyzer, time-based clipping decay, motion/artifact guards, pre-lock cadence
  consistency, a quieter release/diagnostic build split, and the
  `0.4.23-clip-guard` false-progress fix.
- The broader peak-to-peak/acquisition experiment did not restore a trustworthy
  live hardware experience. The connected CYD showed weird rail-to-rail waveform
  behavior with `signal` hitting `0`/`1023`, `range=1023`, `clip=100`, and no
  usable BPM/IBI.
- The clipping guard made the firmware honest about the failure by forcing
  `quality=0`, `p2p=0`, no detector re-arm spam, and no accepted BPM/IBI while
  the ADC was railed. It did not fix the underlying rail-to-rail input.

Next baseline:

- Flash current internal `main` and compare the connected CYD against this
  failed branch before doing any more signal math.
- If `main` also rails, investigate physical/electrical causes first: sensor
  pressure and placement, power/ground, connector wiring, GPIO35 behavior, and
  alternate known signal pins such as GPIO27.

## 2026-05-24 - First-Screen Version Identity

Firmware: `0.4.24-front-id` release, `0.4.24-front-id-log` diagnostic

Design:

- The Pulse dashboard now shows `APP_VERSION` and `APP_FIRMWARE_DATE` directly
  under `PulseSensor.com` on the first screen.
- `tools/check_app_shell.py` guards this convention so future experiment
  branches keep the version/date visible during hardware testing.
- This is a tracking change only. It does not change PulseSensor sampling,
  clipping guards, acquisition thresholds, BPM/IBI qualification, or diagnostic
  serial fields.

Hardware preference note:

- After the purple signal wire was re-soldered and `0.4.24-front-id` was
  flashed from rollback tag `good-working-0.4.24-front-id-20260524`, the user
  reported this was their favorite behavior so far with earlobe placement.
- The user specifically liked that, once a beat was detected, the screen made
  the algorithm/BPM/IBI behavior visible enough to watch the math settle while
  the signal stayed usable.
- Keep `0.4.24-front-id` as an important comparison point when judging later
  waveform-drawing experiments.

## 2026-05-24 - Stable Waveform Viewport

Firmware: `0.4.25-stable-wave` release, `0.4.25-stable-wave-log` diagnostic

Problem:

- User observed that the waveform became jagged as soon as beats qualified and
  seemed to change the longer the app stayed on.
- Code inspection found two display-side causes: `signalToGraphY()` mapped the
  trace through the rolling `minSignal`/`maxSignal` window, which can magnify
  small ADC movement as the range tightens, and `drawWaveform()` overlaid large
  beat-effect circles on the raw trace after lock.

Fix:

- The live graph now uses a stable fixed ADC viewport (`250..850`) for drawing.
- Beat-effect circles no longer draw on top of the waveform trace.
- Beat qualification, BPM/IBI freshness, acquisition scoring, clipping guards,
  and serial diagnostics still use the live rolling signal range.

## Test Record Template

Use this shape for future hardware notes:

```text
Date/time:
Firmware:
Commit:
Sensor body position:
Mount/contact method:
Condition:
Duration:
Serial summary:
User-visible behavior:
Verdict:
```

## 2026-05-24 - Clipping Quality Guard

Firmware: `0.4.23-clip-guard` release, `0.4.23-clip-guard-log` diagnostic

Problem:

- After flashing `0.4.22-core-polish`, the user reported the waveform looked
  much worse and the screen was filling with bar-like noise.
- Live release serial confirmed the ADC was genuinely railed/clipped, but also
  exposed a firmware/UI bug: clipped rail-to-rail input could still show
  `quality=10` or `quality=11`, `p2p=6` or `p2p=7`, and repeated detector
  re-arm messages even though BPM/IBI stayed rejected.

Fix:

- Added shared clipping/artifact helpers:
  `signalIsRecentlyClipped()`, `signalRangeIsMotionArtifact()`, and
  `signalLooksCleanForAcquisition()`.
- Acquisition quality and peak-to-peak score now return `0` when recent rail
  clipping or motion-artifact range is present.
- Detector re-arm now requires a clean acquisition signal, not only a large
  live range.
- The dashboard coach can show `ADJUST SENSOR` for clipped/artifact input.

Hardware evidence:

```text
Date/time: 2026-05-24 EDT
Firmware: 0.4.23-clip-guard release
Sensor body position: earlobe, same placement as the reported bad waveform
Condition: release build flashed after the bad-waveform report
Before fix, 0.4.22 release serial:
  - signal=0 / later 1023, amp=1023, range about 995-1023, clip=100
  - quality stayed 10-11, p2p stayed 6-7
  - repeated "Re-arming PulseSensor detector: alive signal without beat event"
After fix, 0.4.23 release serial:
  - 14 s serial window, 29 summary lines, 0 rawDiag CSV rows
  - signal still railed from 0 through 1023, amp=1023, range=1023, clip=100
  - quality=0, p2p=0, locked=0, BPM=0, IBI=0
  - no detector re-arm messages in the capture
```

Verdict:

- The bad waveform is real clipped/railed input, not just a renderer artifact.
- The firmware no longer treats that rail noise as acquisition progress or a
  peak-to-peak candidate. This protects BPM/IBI and removes the false progress
  bars/harmony/re-arm behavior.
- A clean stable-contact pass is still required before main/public release.

## 2026-05-24 - Signal-Core Polish Build Modes

Firmware: `0.4.22-core-polish` release, `0.4.22-core-polish-log` diagnostic

Design:

- Preserved the clean `0.4.21-signal-log` state as rollback branch/tag
  `backup/good-working-0.4.21-signal-log-20260524` and
  `good-working-0.4.21-signal-log-20260524` before cleanup.
- Kept the source as one Arduino `.ino` file for beginner source use.
- Added two PlatformIO modes: `cyd` defaults raw CSV diagnostics off for a
  quieter release candidate, while `cyd_diag` enables 50 Hz `rawDiag` rows and
  keeps the offline analyzer workflow.
- Reduced code around the signal path without changing beat thresholds:
  removed dead top-bar volume/rotate handlers, shared app-frame drawing, shared
  Settings row visibility, grouped accepted/rejected beat decisions in
  `BeatDecision`, and releases the Origin Story sprite when leaving the app or
  rotating the display.

Build sanity:

```text
Release env cyd:      RAM 23764 / 327680 bytes, Flash 378277 / 1310720 bytes
Diagnostic env cyd_diag: RAM 23764 / 327680 bytes, Flash 378313 / 1310720 bytes
```

Hardware evidence:

```text
Date/time: 2026-05-24 EDT
Firmware: 0.4.22-core-polish-log diagnostic, then 0.4.22-core-polish release
Sensor body position: earlobe, same position as the prior smooth/noisy tests
Mount/contact method: same user earlobe placement; exact pressure not recorded
Condition: Codex-side serial capture while user stayed connected
Diagnostic capture A:
  - Path: logs/signal-log-ear-core-polish-60s-20260524.csv
  - Duration: 59.3 s
  - Rows: 2997 rawDiag rows
  - Firmware beat events: 8 accepted, 104 rejected
  - Accepted reasons: 3 strict, 5 peak2peak
  - Firmware accepted median IBI/BPM: 1002 ms / 59.9 BPM for the plausible
    analyzer series
  - Accepted short IBIs below 700 ms: 1, at 646 ms near the transition into
    noisy/clipped behavior
  - Row-level accepted noisy beats: 0
  - Clip rows: 2012
Diagnostic capture B:
  - Path: logs/signal-log-ear-core-polish-180s-20260524.csv
  - Duration before user stopped capture: 122.1 s
  - Rows: 6010 rawDiag rows
  - Firmware beat events: 0 accepted, 369 rejected
  - Clip rows: 6008
  - Locked rows: 0
Release serial quiet check:
  - Flashed final `cyd` release build after diagnostics.
  - 8 s serial window produced 17 summary lines and 0 rawDiag CSV rows.
  - Signal was still saturated during that window (`signal=1023`, `clip=100`),
    and BPM/IBI stayed at 0 while unlocked.
```

Verdict:

- Cleanup candidate is publish-prep safe from a signal-failure standpoint:
  saturated/clipped periods did not create accepted beats, and the release build
  is quiet by default.
- This is not a clean steady-contact accuracy proof, because the available
  `0.4.22` captures became heavily clipped after the early plausible section.
  Keep the `0.4.21-signal-log` clean same-earlobe capture as the best accuracy
  evidence, and require one more stable-contact hardware sanity pass before
  merging to `main` or publishing publicly.

## 2026-05-24 - Raw Signal Log Diagnostics

Firmware: `0.4.21-signal-log`

Design:

- Added `RAW_SIGNAL_DIAGNOSTICS`, enabled on this diagnostic branch, to stream
  `rawDiag` CSV rows at 50 Hz and immediately on beat decisions.
- Added `tools/capture_signal_log.py` to capture serial rows and
  `tools/analyze_signal_log.py` to run an independent raw peak detector against
  the captured waveform.
- Changed clipping score decay from foreground-loop based to time based, so
  display/app redraw speed cannot erase recent rail hits too quickly.
- Added a rolling-range motion-artifact guard for qualified and peak-to-peak
  candidate beats.
- Added pre-lock cadence consistency. The first beat in an acquisition run may
  seed cadence, but later acquisition beats must stay close to the current
  candidate IBI and cannot be shorter than 70% of that IBI.

Evidence:

```text
Date/time: 2026-05-24 EDT
Firmware: 0.4.21-signal-log
Sensor body position: earlobe, same position as the prior noisy/smooth tests
Mount/contact method: same user earlobe placement; exact pressure not recorded
Condition: Codex-side serial capture while user stayed connected
Duration: 44.2 s final acquisition-cadence capture
Serial summary:
  - Final capture path: logs/signal-log-ear-acqcadence-20260524.csv
  - Rows: 2212
  - Firmware beat events: 47 accepted, 0 rejected
  - Independent raw peaks: 47
  - Firmware accepted median IBI/BPM: 918 ms / 65.3 BPM
  - Independent median IBI/BPM: 920 ms / 65.2 BPM
  - Firmware accepted IBI range: 830-988 ms
  - Independent IBI range: 829-988 ms
  - Clip rows: 0
  - Motion/noise windows: 0
```

Comparison against earlier captures from the same investigation:

```text
Initial raw diagnostics: accepted_noisy=6, short_accepted=5, max_range=984
Time-based clip decay: accepted_noisy=4, short_accepted=4, max_range=1015
Motion-range gate only: accepted_noisy=0, short_accepted=40, max_range=162
Acquisition cadence guard: accepted_noisy=0, short_accepted=0, max_range=402
```

Interpretation:

- The smooth-to-noisy behavior appears to include at least two distinct cases:
  real clipping/rail or huge rolling-range movement, and smaller double-detect
  cadence errors during otherwise clean-looking raw signal.
- The old clipping score could decay according to foreground loop speed, so
  fast loops could make recent rail hits look clean before the signal was truly
  stable again.
- The peak-to-peak/motion-range work correctly rejected obvious noisy accepted
  beats, but acquisition could still lock onto short double detections until
  cadence consistency was added before lock.

Verdict:

- Strong diagnostic improvement for this earlobe run, not a public publish
  verdict. Keep `0.4.21-signal-log` internal until the user visually sanity
  checks the CYD and decides whether to keep raw CSV diagnostics enabled or gate
  them off for a normal release build.

## 2026-05-24 - Peak-To-Peak Experiment

Firmware: `0.4.20-peak2peak`

Design:

- The PulseSensor was kept in the same earlobe position during the recent
  signal work, so this experiment targets small earlobe movement/pressure
  changes rather than mixed body-position behavior.
- Added `PEAK_TO_PEAK_EXPERIMENT`, enabled on this branch, to make the broader
  path easy to disable if false positives appear.
- Kept strict beats as the highest-confidence path.
- Added `peakToPeakScore` from live range, detector amplitude, clipping
  cleanliness, and beat-window state.
- Let high-score peak-to-peak candidate beats help first acquisition or continue
  acquisition after one strict beat, then accept them more liberally after lock
  with a wider cadence window while rejecting short movement-blip intervals and
  preserving plausible timing and clipping guardrails.
- Serial telemetry now includes `p2p=...` and can report
  `accept=peak2peak`.

Hardware notes:

```text
Date/time: 2026-05-24 EDT
Firmware: 0.4.20-peak2peak
Commit: 02a5001 Add peak to peak signal experiment 20260524
Sensor body position: earlobe, same position as prior 0.4.19 tests
Mount/contact method: same user earlobe placement as prior run; exact pressure
  not recorded
Condition: serial sanity from Codex side; user visual verdict still needed
Duration: three short windows while tuning peak-to-peak gates
Serial summary:
  - Initial experiment showed `p2p` scoring live, often 7-10, but no
    `accept=peak2peak`; the old cadence gate was still too timid.
  - Wider 45% locked cadence gate produced `accept=peak2peak`, but could accept
    short movement-blip intervals and pull IBI down too far.
  - Tuned gate uses 35% / 180 ms tolerance plus a 70%-of-current-IBI floor.
    This still produced `accept=peak2peak` during locked runs with plausible
    IBI near the existing cadence, such as 954/966 ms after a stable roughly
    900-960 ms locked run, and later 878/832/800/780 ms during another locked
    run.
  - Clipping remained 0 in the final tuned window.
  - Lock still dropped on some later weak/short-interval sections; several of
    those were strict-detector events rather than peak-to-peak events.
User-visible behavior: not recorded by user yet
Verdict: experiment candidate, not publish verdict. The p2p path is now active
  and bounded, but real visual judgment on the CYD is still needed before main
  merge.
```

## 2026-05-24 - Foreground Timing Sanity

Firmware under test: `0.4.19-peak-cadence`

Commit under test:

```text
8824d51 Refresh project memory for peak cadence handoff 20260524-150800-EDT
```

Temporary instrumentation:

- Set `PERF_DIAGNOSTICS` to `1` locally.
- Flashed to `/dev/cu.usbserial-3120`.
- Upload detected ESP32-D0WD-V3 MAC `f4:2d:c9:9d:af:cc`.
- Restored `PERF_DIAGNOSTICS` to `0` after the timing capture.

Observation:

- The code path still calls `readPulseSensor()` as the first meaningful work in
  `loop()`.
- On the live Pulse dashboard, serial diagnostics showed about
  `11747-33674` foreground loops/reads per second, `449-467` changed raw
  samples per second, max read gaps around `15-29 ms`, and the largest draw
  labels were short dashboard redraws around `11-26 ms`.
- The raw serial stream acquired and re-acquired lock during the test, with
  live ranges roughly `118-236`, clipping score `0`, and strict accepted beats
  producing plausible BPM/IBI values.
- A temporary start-in-Origin diagnostic flash showed the heavier story sprite
  renderer costs more foreground time: about `27178-29611` loops/reads per
  second, `231-253` changed raw samples per second, and `origin` redraws around
  `37-39 ms`, with max read gaps up to about `60 ms`.
- PulseSensor Playground keeps the beat flag latched until
  `sawStartOfBeat()` reads it, so these Origin redraw gaps should not drop a
  beat event by themselves. They do reduce foreground raw/serial refresh
  cadence while the user is on Origin Story.

Verdict:

- Current Pulse dashboard timing is a reasonable upgrade candidate: the live
  screen preserves fast foreground reads and near-500 Hz changed-sample cadence
  while keeping `readPulseSensor()` first.
- Origin Story remains the highest-cost non-Pulse screen. It should stay
  second-class: if future hardware use shows sensing feels worse while parked
  on Origin, slow or pause its sprite animation before touching beat math.
- Sensor body position/contact method was not visually confirmed during this
  Codex-side timing run, so this is a foreground timing sanity record, not a
  final body-position hardware verdict.

## 2026-05-24 - Lock Retention Grace

Firmware: `0.4.17-lock-hold-grace`

Rollback anchors:

```text
Branch: backup/pre-lock-hold-grace-20260524
Tag:    backup/pre-lock-hold-grace-20260524
Commit: 460dbac
```

Observation:

- On long runs, the dashboard could acquire signal, then drop lock after roughly
  7-10 beats.
- The waveform could still look usable while BPM/IBI stopped responding.
- The 2026-05-24 lock-hold serial sanity windows were done with the PulseSensor
  on the user's earlobe. The user's finger had stopped giving a usable signal
  even though it worked well the previous day.
- History showed the false-positive tuning made acquisition stricter by resetting
  `qualifiedBeatStreak` on any unqualified beat. That is good before lock, but
  too brittle after lock when normal finger movement creates a brief bad beat.

Change:

- Keep strict four-qualified-beat acquisition.
- Add post-lock grace for two unqualified beat events within 2200 ms.
- Clear lock on grace expiry or no-beat timeout.
- Expand serial telemetry with live range, clipping score, qualified streak,
  unqualified streak, and lock-drop reason.

Verdict:

- Preliminary upgrade candidate from serial sanity after flashing to
  `/dev/cu.usbserial-3120`; still needs user-visible judgement across body
  positions.
- Sensor body position: earlobe. Mount/contact method: hand-held PulseSensor on
  earlobe.
- 60 s steady earlobe window: 121 parsed signal lines, 66.1% locked, `badStreak`
  reached 2, live range 80-141, max clipping score 0, BPM mean 70.9. Drop
  reasons were `none` for 111 lines and `grace expired` for 10 lines.
- 60 s gentle earlobe movement/pressure window: 120 parsed signal lines, 94.2%
  locked, `badStreak` reached 2, live range 80-155, max clipping score 0, BPM
  mean 74.5. Drop reasons were `none` for 116 lines and `grace expired` for 4
  lines.
- The grace path is active and bounded: unqualified beat streak did not exceed
  the planned 2-beat hold. BPM/IBI still came only from qualified readings.

## Test Notes To Keep

- Record sensor body position every time: finger, earlobe, wrist, or other.
- Record mount/contact method: hand-held, clipped, taped, Stabilizer Ring, or
  loose bench contact.
- Test steady contact for at least 60 seconds.
- Test gentle movement and pressure changes after lock.
- Watch serial for `qStreak`, `badStreak`, `drop`, `range`, and `clip`.
- Mark a change as an improvement only if lock survives small movement while
  BPM/IBI remain plausible and still clear when the sensor is removed from the
  body.

## 2026-05-24 - Peak/Cadence Recovery

Firmware: `0.4.19-peak-cadence`

Observation:

- The UI and waveform looked better, and false-positive filtering was doing its
  job.
- With the PulseSensor on the user's earlobe, slight movement could distort the
  valley/trough while the peaks stayed relatively stable and easy to see
  visually.
- Those events can be true positives that are rejected by valley-sensitive
  amplitude/range qualification, causing beat dropouts even though peak-to-peak
  cadence still looks plausible.

Change:

- First acquisition remains strict: four consecutive strictly qualified beats
  are still required before lock.
- After lock, even strictly qualified beat events must stay close to the current
  trusted cadence so short movement blips do not poison BPM/IBI.
- After lock only, a rejected beat can be accepted as `peak-cadence` if BPM/IBI
  timing stays inside normal bounds, the new IBI is close to the last trusted
  IBI, the beat event and live signal movement still look alive, and clipping
  is still low.
- Serial telemetry now includes `accept=strict`, `accept=peak-cadence`,
  `accept=reject`, or `accept=none` so hardware notes can identify whether the
  recovery path is helping or causing trouble.

Verdict:

- Flashed to `/dev/cu.usbserial-3120` and serial sanity confirmed the new
  `accept=peak-cadence` path activates after lock while clipping stays at 0.
- The first sanity window also showed why the locked cadence guard matters:
  strict absolute IBI limits alone can accept short movement-blip intervals.
- Preliminary upgrade candidate, not final proof. One later window still showed
  a `grace expired` drop after two rejected events, so future tuning should
  compare steady earlobe, gentle earlobe movement, and sensor-off behavior
  before publishing.
- Sensor body position for the motivating observation: earlobe. Mount/contact
  method: user-held contact, exact pressure not recorded.
