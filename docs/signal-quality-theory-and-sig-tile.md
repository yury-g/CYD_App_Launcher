# Signal Quality Theory And SIG GPIO35 Tile

This project treats signal quality as a physical setup problem first and a code
problem second. A beautiful waveform can still fail beat detection if the sensor
is moving, pressed too hard, too loose, pointed at light/noise, or resting on a
body spot that does not repeat cleanly.

## Working Theory

- Raw `GPIO35` waveform means the CYD sees analog movement.
- A PulseSensor Playground beat event means the library thinks a beat started.
- A qualified beat means this firmware trusts that event enough to update
  BPM/IBI.
- Lock means enough trusted beats arrived in a row.
- The `SIG GPIO35` tile should guide the human toward stable contact. It should
  not claim that a pulse is proven just because the line looks alive.

## Practice From Hardware

Movement and resting spot make a large difference in real use. The same firmware
can feel broken with a loose finger, then work well after the sensor settles or
moves to a better contact point. Treat these as test variables:

- body position: fingertip, earlobe, wrist, bench/no finger;
- pressure: too light, stable/gentle, too hard;
- motion: still, cable tugging, sensor sliding, hand tremor;
- light and electrical noise: sensor facing room light, floating input, USB
  cable/ground changes;
- time since boot/reflash: the detector may need a few clean beats to settle.

## SIG GPIO35 Translation

The bottom-right `SIG GPIO35` tile is a contact-confidence ladder:

- Low/empty bars: not enough usable signal yet.
- Rising yellow bars: raw signal looks promising, but BPM/IBI are not trusted.
- Locked green/cyan state: recent beats are trusted enough to show BPM/IBI.
- Drop/reset: contact moved, clipped, became too flat, or beats stopped arriving.

The tile should blend physical and algorithmic evidence:

- live range: is anything moving on GPIO35?
- amplitude: is the movement strong enough?
- clipping: is the signal slamming 0 or 1023?
- steadiness over time: is the contact stable, not just momentarily pretty?
- qualified beat streak: are trusted beats repeating?
- recent drop reason: why did the firmware stop trusting the signal?

Design rule: the tile teaches "getting closer to trustworthy beat detection,"
not "this is definitely a pulse."

## Dev Rule

When signal behavior feels inconsistent, do not start with math changes. First
record firmware version, body placement, contact method, no-finger behavior,
steady-finger behavior, and whether the version label on screen matches the
build that was flashed.
