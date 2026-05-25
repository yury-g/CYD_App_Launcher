# CYD Hardware Experiment Log

Use this file as the broad experiment index. Keep detailed PulseSensor
acquisition and beat-detection notes in `docs/signal-behavior-log.md`, and keep
timing/architecture rules in `docs/signal-first-architecture.md`.

## Current Experiment Navigation

- Signal quality is the top priority. Treat raw `SIG GPIO35`, BPM, IBI, and
  beat qualification as more important than app shell features, display modes,
  Pin Scanner, sounds, or story screens.
- UI experiments should keep screenshots or render previews under
  `docs/screenshots/` so a future chat can see what changed without this chat
  history.
- Hardware experiments should record firmware version, commit, sensor body
  position, contact method, steady-contact behavior, movement behavior, and
  whether the change was an upgrade, downgrade, or still uncertain.
- Reversible lessons matter: keep rejected ideas with the reason they were
  rejected, because some of the useful parts may be reused later.

## 2026-05-25 - Origin Story Design-Only Notes

Session rule: design and notes only; no firmware code changes.

Accepted direction:

- Make Origin Story copy easy to isolate and replace from a plain prompt.
- Preserve the current crawl text in `docs/origin-story-design-notes.md` before
  replacing it.
- Move the Origin Story scrolling text/fade horizon about 20 percent higher in a
  later code pass, keeping the app header fixed.
- Add a local HTML soundtrack helper in a later code pass so the App 3 fanfare
  can be composed, previewed, imported, and exported before flashing the CYD.

## 2026-05-24 — App Shell, Display Modes, Settings, And Signal Regression Pass

Branch: `codex/app4-pin-scanner-perf-safe-20260524`

Latest wrapped firmware at handoff: `0.4.19-peak-cadence`

### UI Work That Looked Good On CYD

- Compact app sequence: Pulse dashboard, Settings, App 4 `Pin Scanner`, `Your
  App Here`, and `Origin Story`.
- Settings owns volume, rotation, display mode, LED, About, firmware/build
  rows, runtime memory, and scroll controls.
- Four display modes: `M DARK`, `M LIGHT`, `C DARK`, and `C LIGHT`.
- Settings horizontal rows keep readable size-2 text for long values by moving
  value text to a second line; tiny value text is reserved for vertical
  rotation only.
- App 4 Pin Scanner starts idle and reads only the selected row.
- Origin Story uses an offscreen sprite/fallback renderer after hardware showed
  text-size, flicker, and blank-screen problems.

### Signal Lessons From This Pass

- Keep `readPulseSensor()` as the first meaningful call in `loop()`.
- Avoid full-screen redraws in normal live Pulse updates. Draw waveform columns
  and changed panels only.
- Acquisition bars are user guidance, not proof of BPM lock.
- The current best detector rule is: acquire strictly, hold gently, then after
  lock allow bounded peak/cadence recovery for valley-distorted true positives.
- Sensor body position matters. Recent validation used the earlobe because the
  finger stopped giving reliable signal even though it worked well the day
  before.

### Current Verdict

- UI is visually ready for a main-merge candidate.
- Signal behavior is an upgrade candidate but still needs body-position sanity
  passes before publishing. One `0.4.19-peak-cadence` serial window showed the
  intended `accept=peak-cadence` recovery; another still showed a bounded
  `grace expired` drop after rejected events.

## 2026-05-24 — 0.4.41 Snappy Lock Handoff

Branch during work: `codex/recover-green-beat-lock-20260524`

Firmware flashed at handoff: `0.4.41-snappy-lock`

Goal:

- Restore the pre-multi-app feel where beat detection is quick and snappy.
- Hold lock through minor movement.
- Recover quickly when the waveform is good but PulseSensor Playground stops
  producing beat events.

What changed:

- Re-arm now retunes the PulseSensor threshold from the current waveform
  midpoint and shows that active threshold on the graph.
- Beat acceptance now trusts the old simple qualified-beat path again instead of
  requiring extra cadence gates for strict acceptance.
- Lock grace is more forgiving: four bad beat events and a 4200 ms window.
- Auto re-arm is faster: 1600 ms quiet detector threshold and 1600 ms cooldown.
- Large waveform range alone no longer blocks re-arm; recent rail clipping still
  blocks it.
- Waveform beat markers now separate accepted beats from raw Playground
  calculations: large filled white dots are accepted, small hollow white dots
  are rejected candidate events.
- The pre-multi-app heart shape was restored while keeping sprite-based
  flicker-free drawing and a white outline.
- Pin Scanner ADC ownership and row-local redraws were kept to avoid the IO35
  crash and scanner flicker.

Verification:

- Guard scripts passed: app shell, clipping quality, signal diagnostics,
  peak-to-peak experiment, and core polish.
- `python3 -m platformio run -e cyd` passed.
- Upload to `/dev/cu.usbserial-10` passed.
- Serial confirmed the new auto re-arm path fired. The short bench/contact read
  was not a stable live body-lock validation.

Continuation note:

- Read `docs/handoff-2026-05-24-0.4.41-snappy-lock.md` before continuing this
  work in a new chat/model.

## 2026-05-19 11:39 EDT — Signal Dashboard / Finger Coach Side Quest

Branch: `codex/finger-coach-dashboard-20260519-111641-EDT`

Tested on connected ESP32-2432S028 CYD hardware.

## 2026-05-19 13:04:51 EDT — Best Dual PulseSensor Pin Candidates

Raw pin-scanner testing on the connected ESP32-2432S028 CYD showed `GPIO35` and `GPIO27` as the best two PulseSensor signal candidates. `GPIO22` also showed usable raw signal, but a dashboard experiment using `GPIO22` as the PulseSensor input caused the screen to turn on/off in a reset loop. Use `GPIO35` and `GPIO27` for the first two-PulseSensor dashboard experiment.

### Tried

- Auto-expanding Signal Dashboard when the PulseSensor signal was not yet qualified.
- Finger Coach guidance states such as place finger, press firmer, press lighter, hold steady, and signal acquired.
- A 5-step Playground lock game with heart progress.
- Visual math strip for amplitude, live range, min/max, threshold, quality, and clipping.
- Friendly sound cues to gamify signal acquisition.

### IRL Result

The expanded dashboard did not improve the experience on the actual CYD display. It felt too busy, and the active bottom panel introduced visible flicker and over-drawing artifacts.

### Kept

- Cyan dotted threshold line on the graph.
- `THR 550` label on the graph.

### Rejected For Now

- Full Signal Dashboard screen.
- Finger Coach copy/states.
- Lock-game bottom panel.
- Dense visual math strip.
- Coaching nudge sounds.

### Current Direction

Return to the original one-screen dashboard: waveform, BPM, IBI, and compact signal panel. Keep the threshold affordance from the experiment, and use only a small signal-quality harmony tied to the bottom-right `SIGNAL` quality progress.
