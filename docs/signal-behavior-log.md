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
- Raw `SIG GPIO35`, BPM, IBI, and qualified-beat behavior outrank display polish,
  app switching, Pin Scanner, sounds, and story screens.
- Always record body position and mounting method for hardware validation. Do
  not compare finger, earlobe, wrist, or loose bench readings as if they are the
  same test condition.

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
Commit: pending implementation commit
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
