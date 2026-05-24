# Peak-To-Peak Experiment Design

Date: 2026-05-24

## Goal

Experiment with using peak-to-peak evidence more liberally for the same-position
earlobe case where small user movement or pressure changes distort valley-based
amplitude math while the visible pulse waveform still looks usable.

## Approach

Add a compile-time experiment switch, enabled on this branch, so the firmware can
compare the current `0.4.19-peak-cadence` behavior with a more assertive
peak-to-peak path. The existing strict PulseSensor Playground beat remains the
base event source. The experiment changes how that event is accepted, scored,
and reported.

## Firmware Behavior

- Keep `readPulseSensor()` first in `loop()`.
- Keep `GPIO35`, 10-bit ADC scaling, clipping rejection, and plausible BPM/IBI
  bounds.
- Keep strict beats as the highest-confidence path.
- Add a broader peak-to-peak candidate path that can contribute before lock when
  the waveform has enough live range, enough detector amplitude or range
  movement, low clipping, and plausible timing.
- After lock, allow peak-to-peak candidates more liberally than the current
  locked-only recovery path, while still checking cadence against the current
  trusted IBI.
- Add peak-to-peak contribution to acquisition scoring so the `SIG GPIO35`
  ladder climbs faster when the waveform is alive but strict beat streaking is
  fragile.
- Extend serial telemetry with a small peak-to-peak diagnostic field so hardware
  tests can see when this path is helping or over-accepting.

## Safety Rails

- The experiment stays behind a named compile-time switch.
- Strict accepted beats still report as `accept=strict`.
- Peak-to-peak accepted beats report as `accept=peak2peak`.
- Rejected beats remain rejected and can still drop lock through the existing
  grace/timeout paths.
- Do not tune display code or app shell behavior as part of this experiment.
- If hardware shows false positives or unstable BPM/IBI, disable the switch or
  tighten only the peak-to-peak thresholds before touching the known strict
  pathway.

## Verification

- Add a source-inspection regression check that fails until the peak-to-peak
  switch, helper, serial field, and acquisition scoring hooks exist.
- Run the existing render/check tools before flashing.
- Build with PlatformIO.
- Flash the attached CYD on `/dev/cu.usbserial-3120`.
- Capture serial while the sensor remains on the same earlobe position and
  compare lock speed, `accept=peak2peak` frequency, BPM/IBI plausibility, and
  lock drops against the latest `0.4.19` notes.
