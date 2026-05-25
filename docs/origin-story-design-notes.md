# Origin Story Design Notes

Date: 2026-05-25

Purpose: keep Origin Story animation and soundtrack tweaks easy to review before
future firmware passes. The canonical crawl copy now lives in
`docs/origin-story-crawl.txt`.

## Current Crawl Text

The current firmware and render mock both sync from
`docs/origin-story-crawl.txt`.

## Copy Swap Goal

Current code keeps the crawl copy obvious and beginner-friendly:

- The crawl text lives in one isolated, clearly named place:
  `docs/origin-story-crawl.txt`.
- Keep blank lines as intentional paragraph breaks.
- Let a user prompt Codex with plain copy, such as "replace the Origin Story
  crawl with this text", and have Codex update the text file and run
  `python3 tools/sync_origin_story_crawl.py --write`.
- The sync tool keeps firmware and preview tooling aligned so the mock render
  and flashed CYD cannot drift apart.

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
