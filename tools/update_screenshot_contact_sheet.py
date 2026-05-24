from pathlib import Path

SCREENSHOT_DIR = Path("docs/screenshots")
README = SCREENSHOT_DIR / "README.md"
IMAGE_SUFFIXES = {".png", ".svg"}


def image_paths():
    return sorted(
        path
        for path in SCREENSHOT_DIR.rglob("*")
        if path.is_file() and path.suffix.lower() in IMAGE_SUFFIXES
    )


def group_label(path):
    parent = path.parent.relative_to(SCREENSHOT_DIR)
    return "Root" if str(parent) == "." else str(parent)


def alt_text(path):
    stem = path.relative_to(SCREENSHOT_DIR).with_suffix("")
    return str(stem).replace("/", " / ").replace("-", " ")


paths = image_paths()
lines = [
    "# Screenshot Contact Sheet",
    "",
    "This folder README is a flat contact sheet for every checked-in `.png` and `.svg` file under `docs/screenshots/`.",
    "",
    "When screenshots are added or regenerated, run `python3 tools/update_screenshot_contact_sheet.py` so the GitHub folder view stays useful.",
    "",
    f"Total images: `{len(paths)}`",
    "",
]

current_group = None
for path in paths:
    relative = path.relative_to(SCREENSHOT_DIR)
    group = group_label(path)
    if group != current_group:
        lines.append(f"## {group}")
        lines.append("")
        current_group = group

    lines.append(f"### `{relative}`")
    lines.append("")
    lines.append(
        f'<a href="{relative}"><img src="{relative}" alt="{alt_text(path)}" width="320"></a>'
    )
    lines.append("")

README.write_text("\n".join(lines), encoding="utf-8")
print(f"Wrote {README} with {len(paths)} images")
