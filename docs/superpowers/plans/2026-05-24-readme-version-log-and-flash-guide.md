# README Version Log And Flash Guide Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the repository README useful as a readable project memory log, visual UI tour, and source-flash guide for the current app-shell pause point.

**Architecture:** Keep `README.md` as the approachable front door and `CHANGELOG.md` as the detailed historical log. Add a concise version-history section, clarify that the current app-shell branch is a development pause point, and make the non-Codex PlatformIO flash path explicit for a new computer.

**Tech Stack:** Markdown documentation, existing PlatformIO project, existing render tools, existing app-shell guard.

---

### Task 1: README Major Version Log

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add a readable version-history section near the top**

Add a `## Version History And Major Changes` section after the current horizontal dashboard screenshots. Include short entries for the 2026-05-24 app-shell pause point, v1.2.0, v1.1.0, v1.0.0, v0.2.0, and v0.1.0.

- [ ] **Step 2: Link the deeper changelog**

End the section with a sentence pointing to `CHANGELOG.md` for full detail.

### Task 2: Current UI And Flash Path

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Clarify the current app-shell preview**

Update the existing app-shell preview introduction so it says this branch is the current good development pause point, lists the branch/head commit, and notes that Origin Story copy is placeholder text for a later writing pass.

- [ ] **Step 2: Add a source-flash guide for another computer**

Under `## Flash Your CYD`, add a "Current app-shell preview from source" option before the public web-installer path. Include `git clone`, `git checkout codex/monochrome-ui-treatment-20260524`, `python3 -m pip install --user platformio`, `pio device list`, `pio run -e cyd`, and `pio run -e cyd -t upload --upload-port <detected port>`.

- [ ] **Step 3: Avoid web-installer confusion**

Clarify that the public one-click installer is for the published tutorial firmware unless the checked-in `firmware/` binaries are regenerated from this branch.

### Task 3: Changelog Checkpoint

**Files:**
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Add an unreleased documentation checkpoint**

Add one `Unreleased` bullet noting that the README now keeps a readable major-change log and non-Codex source-flash instructions for the app-shell pause point.

### Task 4: Verification

**Files:**
- Verify: `README.md`
- Verify: `CHANGELOG.md`
- Run: `python3 tools/check_app_shell.py`
- Run: `PATH=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.venv-pio/bin:$PATH PLATFORMIO_CORE_DIR=/Users/mininarwhal/Documents/Codex/2026-05-23/i-have-a-cyd-connected-can/.platformio pio run -e cyd`

- [ ] **Step 1: Inspect Markdown headings**

Run `grep -n "^##\\|^#" README.md CHANGELOG.md` and confirm the new sections are placed logically.

- [ ] **Step 2: Run the app-shell guard**

Expected output: `App shell checks passed`.

- [ ] **Step 3: Run the firmware build**

Expected output includes `[SUCCESS]`.
