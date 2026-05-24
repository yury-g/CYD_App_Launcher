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
- BPM and IBI are still only updated by qualified beats. Grace-held bad beats
  preserve the last trusted BPM/IBI but do not create new readings.
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
