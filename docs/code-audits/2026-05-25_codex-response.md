# Codex Response To 2026-05-25 Audits

Date: 2026-05-25  
Current source firmware: `0.4.41-snappy-lock`
Product name to use now: **PulseSensor CyberDeck with the CYD**

## Short Recommendation

The audits are useful, but they are old snapshots. Treat them as a map of risks,
not as a live bug list. The current repo already fixed or invalidated several
claims: `LICENSE`, `platformio.ini`, tools, docs, browser-flash clarity, source
metadata checks, and shipped peak-to-peak recovery naming are now present on
`main`.

## Do Now

- Keep the public story aligned around **PulseSensor CyberDeck with the CYD**:
  Pulse dashboard, Settings, Pin Scanner, `Your App Here`, and Origin Story.
- Keep the README/changelog short enough for humans; move long branch history to
  docs.
- Remove blocking detector re-arm delay.
- Surface PulseSensor initialization failure on the CYD screen.
- Add small source comments for signal-first loop order, interrupt setup, and
  subtle signal-range decay.

## Do Later

- Add GitHub Actions build checks.
- Add a GitHub Release with attached binaries.
- Refresh web-installer binaries only during an intentional browser-flasher
  release.
- Add `docs/HACK.md` with student extension recipes.
- Add a supported-boards note for ILI9341 vs ST7789/S3 variants.
- Add a wiring/photo/hero asset pass.
- Consider Arduino IDE `User_Setup.h` helper docs if students are not using the
  PlatformIO path.
- Consider `tft.textWidth()` polish, backlight PWM, and runtime threshold tuning.

## Ignore Or Deprioritize

- Audit 02's "large update not pushed" premise. It is obsolete.
- "No LICENSE/platformio/tools/docs." Obsolete.
- "No app shell." Obsolete for the current `main`.
- Blanket `volatile` changes. Do a targeted concurrency review before touching
  signal state qualifiers.
- A formal `App struct` registry. Nice later, but not needed to keep the current
  CyberDeck understandable.
- Green/blue LED channel cleanup. Low value until those channels are needed.
- `delay(100)` after serial setup. Tiny cleanup, not worth derailing current
  signal and docs work.
