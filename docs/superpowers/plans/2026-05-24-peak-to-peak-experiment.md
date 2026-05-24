# Peak-To-Peak Experiment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an enabled, rollback-friendly peak-to-peak beat experiment for faster acquisition and more tolerant locked earlobe sensing.

**Architecture:** Keep the single-file Arduino sketch. Add named experiment constants, a peak-to-peak candidate helper, acquisition-score contribution, and serial telemetry without moving `readPulseSensor()` or changing app shell draw behavior.

**Tech Stack:** Arduino/C++ in `PulseSensor_CYD.ino`, PlatformIO `cyd` environment, Python source-inspection guard scripts.

---

### Task 1: Regression Guard

**Files:**
- Create: `tools/check_peak_to_peak_experiment.py`

- [ ] **Step 1: Write the failing guard**

Create a Python source-inspection guard requiring:

```python
from pathlib import Path

source = Path("PulseSensor_CYD.ino").read_text()

required = {
    "#define PEAK_TO_PEAK_EXPERIMENT 1": "experiment switch must be enabled",
    "#define APP_VERSION \"0.4.20-peak2peak\"": "firmware version must identify the experiment",
    "bool isPeakToPeakCandidateBeat": "candidate helper is missing",
    "int peakToPeakScoreForCurrentSignal": "acquisition scoring hook is missing",
    "bool peakToPeakAccepted = PEAK_TO_PEAK_EXPERIMENT && isPeakToPeakCandidateBeat": "read path must use peak-to-peak acceptance",
    '"peak2peak"': "serial accept reason must include peak2peak",
    "p2p=%d": "serial telemetry must include the peak-to-peak score",
    "peakToPeakScore": "runtime score state is missing",
}

missing = [message for token, message in required.items() if token not in source]
if missing:
    raise SystemExit("Peak-to-peak experiment guard failed: " + "; ".join(missing))

print("Peak-to-peak experiment checks passed")
```

- [ ] **Step 2: Run guard and verify RED**

Run: `python3 tools/check_peak_to_peak_experiment.py`

Expected: fails with missing experiment switch/helper messages.

### Task 2: Firmware Experiment

**Files:**
- Modify: `PulseSensor_CYD.ino`
- Modify: `tools/check_app_shell.py`

- [ ] **Step 1: Add constants and state**

Add:

```cpp
#define PEAK_TO_PEAK_EXPERIMENT 1
#define PEAK_TO_PEAK_MIN_RANGE 70
#define PEAK_TO_PEAK_STRONG_RANGE 135
#define PEAK_TO_PEAK_MIN_AMPLITUDE 10
#define PEAK_TO_PEAK_ACQUIRE_MIN_SCORE 5
#define PEAK_TO_PEAK_LOCKED_MIN_SCORE 4
#define PEAK_TO_PEAK_PRELOCK_CADENCE_MIN_STREAK 1
```

Change version to `0.4.20-peak2peak`.

Add `int peakToPeakScore = 0;` near the live sensor state.

- [ ] **Step 2: Add helper prototypes and helpers**

Add prototypes:

```cpp
int peakToPeakScoreForCurrentSignal();
bool isPeakToPeakCandidateBeat(int bpm, int ibi, int amplitude, bool wasLocked);
```

Implement helpers near `isPeakCadenceRecoveryBeat()`:

```cpp
int peakToPeakScoreForCurrentSignal() {
  int liveRange = maxSignal - minSignal;
  int rangeScore = map(constrain(liveRange, PEAK_TO_PEAK_MIN_RANGE, PEAK_TO_PEAK_STRONG_RANGE),
                       PEAK_TO_PEAK_MIN_RANGE, PEAK_TO_PEAK_STRONG_RANGE, 0, 4);
  int amplitudeScore = map(constrain(pulseAmplitude, PEAK_TO_PEAK_MIN_AMPLITUDE, AMPLITUDE_METER_MAX),
                           PEAK_TO_PEAK_MIN_AMPLITUDE, AMPLITUDE_METER_MAX, 0, 3);
  int cleanScore = clippedSampleScore <= 4 ? 2 : (clippedSampleScore <= 18 ? 1 : 0);
  int beatWindowScore = insideBeatWindow ? 1 : 0;
  int score = rangeScore + amplitudeScore + cleanScore + beatWindowScore;
  if (liveRange < PEAK_TO_PEAK_MIN_RANGE && pulseAmplitude < PEAK_TO_PEAK_MIN_AMPLITUDE) score = 0;
  return constrain(score, 0, 10);
}

bool isPeakToPeakCandidateBeat(int bpm, int ibi, int amplitude, bool wasLocked) {
  if (!isPlausibleBeatTiming(bpm, ibi)) return false;
  if (clippedSampleScore > 18) return false;
  int requiredScore = wasLocked ? PEAK_TO_PEAK_LOCKED_MIN_SCORE : PEAK_TO_PEAK_ACQUIRE_MIN_SCORE;
  if (peakToPeakScore < requiredScore) return false;
  if (wasLocked) return isLockedCadenceMatch(ibi);
  if (qualifiedBeatStreak < PEAK_TO_PEAK_PRELOCK_CADENCE_MIN_STREAK) return false;
  return amplitude >= PEAK_TO_PEAK_MIN_AMPLITUDE || (maxSignal - minSignal) >= PEAK_TO_PEAK_MIN_RANGE;
}
```

- [ ] **Step 3: Use helper in `readPulseSensor()`**

Before `maybeRearmDetector()`, update `peakToPeakScore = peakToPeakScoreForCurrentSignal();`.

Inside beat handling, compute:

```cpp
bool peakToPeakAccepted = PEAK_TO_PEAK_EXPERIMENT &&
                          !strictAccepted &&
                          isPeakToPeakCandidateBeat(bpm, ibi, pulseAmplitude, wasLocked);
bool recovered = !strictAccepted && !peakToPeakAccepted && wasLocked &&
                 isPeakCadenceRecoveryBeat(bpm, ibi, pulseAmplitude);
bool accepted = strictAccepted || peakToPeakAccepted || recovered;
```

When accepted, set:

```cpp
lastBeatAcceptReason = strictAccepted ? "strict" : (peakToPeakAccepted ? "peak2peak" : "peak-cadence");
```

- [ ] **Step 4: Add acquisition score contribution and reset behavior**

In `acquisitionScoreForCurrentSignal()`, add a capped contribution:

```cpp
int peakScore = PEAK_TO_PEAK_EXPERIMENT ? min(2, peakToPeakScore / 3) : 0;
int score = rangeScore + amplitudeScore + cleanScore + beatWindowScore + streakScore + peakScore;
```

Reset `peakToPeakScore = 0;` in detector rearm and manual reset paths.

- [ ] **Step 5: Add serial telemetry**

Change serial print format from:

```cpp
"signal=%d amp=%d bpm=%d ibi=%d locked=%d quality=%d range=%d clip=%d qStreak=%d badStreak=%d accept=%s drop=%s\n"
```

to include `p2p=%d` after `quality=%d`, and pass `peakToPeakScore`.

- [ ] **Step 6: Update app shell guard**

Update `tools/check_app_shell.py` so the required peak/cadence token changes from locked-only recovery to the new experimental peak-to-peak path while preserving lock hold checks.

### Task 3: Documentation And Hardware

**Files:**
- Modify: `CHANGELOG.md`
- Modify: `docs/signal-behavior-log.md`

- [ ] **Step 1: Document the experiment**

Add a changelog line for `0.4.20-peak2peak` describing the enabled compile-time experiment, serial `p2p` telemetry, faster acquisition attempt, and locked movement tolerance.

- [ ] **Step 2: Run verification**

Run:

```sh
python3 tools/check_peak_to_peak_experiment.py
python3 tools/check_app_shell.py
git diff --check
PATH=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.venv-pio/bin:$PATH PLATFORMIO_CORE_DIR=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.platformio pio run -e cyd
```

Expected: all pass.

- [ ] **Step 3: Flash and capture serial**

Run:

```sh
PATH=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.venv-pio/bin:$PATH PLATFORMIO_CORE_DIR=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.platformio pio run -e cyd -t upload --upload-port /dev/cu.usbserial-3120
```

Then capture serial for the same earlobe placement and append findings to `docs/signal-behavior-log.md`.
