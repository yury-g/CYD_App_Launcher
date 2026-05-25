# Origin Story Design Notes

Date: 2026-05-25

Purpose: keep Origin Story text, animation, and soundtrack tweaks easy to review
before the next firmware/code pass. This session is notes-only; no firmware code
changes were made.

## Current Crawl Text

The current firmware and render mock both use this crawl text:

```text
EPISODE PPG
A TINY SENSOR
FINDS THE BEAT

From Brooklyn shops
and Parsons classes,
Joel Murphy
and Yury Gitman
built open hardware
heart-rate sensing
for makers.

World Famous
Electronics began
as a Kickstarter
project in 2012,
then kept making
PulseSensor
and teaching it
in public.

OSHWA certified:
Pulse Sensor Amped
UID US000075
August 30, 2017

GitHub repo:
github.com/
WorldFamousElectronics/PulseSensorPlayground

Repo likes (stars):
249 stars, 207 forks
as of May 24, 2026

The sensor shines
green light into
capillary tissue
and watches the
returning brightness.
Each pulse wave
nudges the signal.

Its origin is
delightfully practical:
breadboards,
op-amps, filters,
a phone-style
light sensor,
and a reverse-mount
green LED
made finger placement
better.

Now the signal
lands here,
on a Cheap Yellow
Display:
open, tiny, alive
with code.

Send feature requests,
firmware update ideas,
and wild classroom
wishes.

Thanks for supporting
PulseSensor since 2012.
```

## Copy Swap Goal

Future code should make the crawl copy obvious and beginner-friendly:

- Put the crawl text in one isolated, clearly named place, instead of requiring
  people to hunt through animation code.
- Keep blank lines as intentional paragraph breaks.
- Let a user prompt Codex with plain copy, such as "replace the Origin Story
  crawl with this text", and have Codex update the isolated text block plus the
  line count safely.
- Keep the same source text available to both firmware and preview tooling so
  the mock render and flashed CYD cannot drift apart.
- Add a short comment near the text block explaining that each quoted line is
  one rendered crawl line.

## Animation Placement Goal

The current scrolling text/fade horizon feels too low. In the next code pass,
move the entire Origin Story text animation about 20 percent higher so large
foreground text has more room before it shrinks and disappears near the top.

Implementation intent:

- Apply the shift to the crawl text path and its direct fallback path.
- Keep the header/navigation fixed.
- Keep the starfield/background stable unless the preview shows the text no
  longer feels visually centered.
- Update the render mock so screenshots preview the same 20 percent-higher crawl
  behavior before flashing.

## Soundtrack Tweak Tool Goal

Create a local HTML soundtrack helper for App 3 before changing the firmware
music arrays. It should be friendly to both non-coders and coders:

- First screen: editable step table with frequency, duty/volume, duration, and
  rest rows.
- Playback controls: play, stop, loop, tempo scale, volume scale, and reset.
- Film-score helpers: octave up/down, stretch durations, insert rest, duplicate
  phrase, and transpose by semitone.
- Export panel: firmware-ready arrays for frequencies, duties, durations, step
  count, loop start, and loop length.
- Import panel: paste existing firmware arrays back into the tool for further
  tweaking.
- Notes field: short plain-English description of the intended mood, for
  example "slow heroic fanfare, small speaker, sci-fi crawl".

After the tool sounds right in-browser, copy its exported arrays into the
firmware and test on the CYD speaker.
