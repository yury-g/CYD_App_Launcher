# Code Audit 02 - Re-Review After Claimed Update

Date: 2026-05-25  
Reviewer: Claude (Opus 4.7), at Yury's request  
Snapshot reviewed: older public `main` before the current cleanup commit reached GitHub  
Scope: compare Audit 01 findings against the then-visible public repo

## TL;DR As Received

The re-review said the "large code base update" was not visible on public
`main`, so none of the first audit's major issues appeared fixed. It also said
the public story had gotten more confusing because repo metadata and changelog
entries advertised features that the reviewed source did not contain.

## Main Findings

- **The reviewed public branch had not moved.** The audit therefore reported
  `0/26` prior findings fully resolved.
- **Identity drift was worse in that snapshot.** The repo description advertised
  a launcher while the inspected source still looked like a one-screen
  dashboard.
- **The changelog was too aspirational.** Audit 02 called out release/version
  entries that did not match the inspected public source.
- **PlatformIO/tooling references were inconsistent.** The reviewed public tree
  appeared to reference private/local tooling that was not yet committed.
- **PR/App 4 work looked fragile in that moment.** The audit recommended not
  force-merging without clean guards around ADC-capable pins and scan cadence.

## New Findings Worth Tracking

| ID | Original Concern | Current Disposition |
| --- | --- | --- |
| N1 | Aspirational release entries in changelog | Addressed by shortening current changelog and linking detailed logs. |
| N2 | Repo description/README divergence | Partly addressed; current product story is now PulseSensor CyberDeck with CYD ESP-322432S028. |
| N3 | `.pio/` ignored but no `platformio.ini` | Stale; `platformio.ini` is now committed. |
| N4 | Versioning scheme contradiction | Addressed by moving the current source firmware label to `CyberDesk 0.5.0`. |
| N5 | App 4 crash-control history | Mostly addressed by guarded manual Pin Scanner behavior. |
| N6 | No explicit read cadence gate | Defer; PulseSensorPlayground owns the 500 Hz sampler. |
| N7 | Signal-range decay is subtle | Addressed with source comment. |
| N8 | Empty GitHub Releases | Later release/publishing task. |
| N9 | Public changelog references private branches | Partly addressed by shortening public changelog; detailed logs remain in docs. |

## Current Caveat

The strongest claim in this audit, that the update was not pushed, is now stale.
Current `main` includes the app-shell source, PlatformIO config, license, docs,
tools, release hygiene checks, metadata checks, and peak-to-peak recovery rename.
